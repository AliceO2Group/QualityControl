// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   BasicReadoutHeaderQcTask.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
///

// ROOT
#include <TH1.h>
#include <TH2.h>
// O2
#include <DPLUtils/RawParser.h>
#include <DPLUtils/DPLRawParser.h>
// #include <Framework/InputRecord.h> // needed??
// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/BasicReadoutHeaderQcTask.h"

using namespace o2::framework;
using namespace o2::header;

namespace o2::quality_control_modules::mft
{

BasicReadoutHeaderQcTask::~BasicReadoutHeaderQcTask()
{
  /*
    not needed for unique pointers
  */
}

void BasicReadoutHeaderQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize BasicReadoutHeaderQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  // Defining summary histogram
  //==============================================
  mMFT_SummaryLaneStatus_H = std::make_unique<TH1F>("mMFT_SummaryLaneStatus_H", "mMFT_SummaryLaneStatus_H", 5, -0.5, 4.5);
  getObjectsManager()->startPublishing(mMFT_SummaryLaneStatus_H.get());
  mMFT_SummaryLaneStatus_H->GetXaxis()->SetBinLabel(1, "Missing data");
  mMFT_SummaryLaneStatus_H->GetXaxis()->SetBinLabel(2, "Warning");
  mMFT_SummaryLaneStatus_H->GetXaxis()->SetBinLabel(3, "Error");
  mMFT_SummaryLaneStatus_H->GetXaxis()->SetBinLabel(4, "Fault");
  mMFT_SummaryLaneStatus_H->GetXaxis()->SetBinLabel(5, "#Entries");
  mMFT_SummaryLaneStatus_H->SetStats(0);
  //  Defining individual histograms
  //==============================================
  generateRUidMap(); // create a map of possible FEEid to
  // histo name defined by half, disk, face, zone
  int zone = -1;
  int plane = -1;
  int disc = -1;
  int half = -1;
  for (int i = 0; i < kmaxRUid; i++) { // loop over potential IDs
    if (RUidMap[i] == -1)
      continue;                             // skip not asigned IDs
    unpackRUid(i, zone, plane, disc, half); // get geo info
    // define histogram
    auto RU_H = std::make_unique<TH2F>(
      Form("IndividualLaneStatus/h%d-d%d-f%d-z%d", half, disc, plane, zone),
      Form("h%d-d%d-f%d-z%d", half, disc, plane, zone),
      25, -0.5, 24.5, // lanes
      4, -0.5, 3.5);  // status
    RU_H->GetYaxis()->SetBinLabel(1, "OK");
    RU_H->GetYaxis()->SetBinLabel(2, "Warning");
    RU_H->GetYaxis()->SetBinLabel(3, "Error");
    RU_H->GetYaxis()->SetBinLabel(4, "Fault");
    RU_H->SetXTitle("Lane");
    RU_H->SetStats(0);
    // push the histo into the vector of histograms and publish it
    mMFT_IndividualLaneStatus_vH.push_back(std::move(RU_H));
    getObjectsManager()->startPublishing(mMFT_IndividualLaneStatus_vH[RUidMap[i]].get());
  } // end-potential IDs
}

void BasicReadoutHeaderQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  mMFT_SummaryLaneStatus_H->Reset();
  for (int i = 0; i < knRU; i++) {
    mMFT_IndividualLaneStatus_vH[i]->Reset();
  }
}

void BasicReadoutHeaderQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void BasicReadoutHeaderQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the input
  DPLRawParser parser(ctx.inputs());

  // loop over input
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    // get the header
    auto const* rdh = it.get_if<o2::header::RAWDataHeaderV6>();
    // get detector field
    uint64_t SummaryLaneStatus = rdh->detectorField;
    // fill histogram bin with #entries
    mMFT_SummaryLaneStatus_H->Fill(4);
    // fill status if set
    if (SummaryLaneStatus & (1 << 0))
      mMFT_SummaryLaneStatus_H->Fill(0); // missing data
    if (SummaryLaneStatus & (1 << 1))
      mMFT_SummaryLaneStatus_H->Fill(1); // warning
    if (SummaryLaneStatus & (1 << 2))
      mMFT_SummaryLaneStatus_H->Fill(2); // error
    if (SummaryLaneStatus & (1 << 3))
      mMFT_SummaryLaneStatus_H->Fill(3); // fault
    if (rdh->stop && it.size()) {
      auto const* ddw = reinterpret_cast<const MFTDDW*>(it.data());
      uint16_t ddw_id = ddw->indexWord.indexBits.id;
      if (ddw_id == 0xE4) { // it is a diagnostic data word
        uint64_t ddw_laneStatus = ddw->laneWord.laneBits.laneStatus;
        uint16_t rdh_feeID = rdh->feeId;
        int RUid = (rdh_feeID & 127); // look only at the rightmost 7 bits
        // check the status of each lane
        for (int i = 0; i < 25; i++) {
          // get the two bits corresponding to the lane i
          int laneStatus = ((ddw_laneStatus >> (i * 2)) & (3));
          // fill the status in the histogram
          mMFT_IndividualLaneStatus_vH[RUidMap[RUid]]->Fill(i, laneStatus);
        }
      } // end if is a DDW
    }   // end if rdh->stop

  } // end loop over input
}

void BasicReadoutHeaderQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void BasicReadoutHeaderQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void BasicReadoutHeaderQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mMFT_SummaryLaneStatus_H->Reset();
  for (int i = 0; i < knRU; i++) {
    mMFT_IndividualLaneStatus_vH[i]->Reset();
  }
}

void BasicReadoutHeaderQcTask::unpackRUid(int RUid, int& zone, int& plane, int& disc, int& half)
// unpacks RU ID into geometry information needed to name histograms
{
  zone = (RUid & 3);         // first and second bits from right to left
  plane = ((RUid >> 2) & 1); // third bit from right to left
  disc = ((RUid >> 3) & 7);  // fourth to sixth bits from right to left
  half = ((RUid >> 6) & 1);  // seventh bits from right to left
}

void BasicReadoutHeaderQcTask::generateRUidMap()
// maps RUid into an index from 0 to 79 to be used
// to access the vector of histograms
{
  // space to store geometrical info
  int zone = -1;
  int plane = -1;
  int disc = -1;
  int half = -1;

  // loop over potential IDs
  int idx = 0; // count good IDs
  for (int i = 0; i < kmaxRUid; i++) {
    RUidMap[i] = -1; // -1 means no RU has such an ID
    unpackRUid(i, zone, plane, disc, half);
    if (zone > 3)
      continue;
    if (plane > 1)
      continue;
    if (disc > 4)
      continue;
    if (half > 1)
      continue;
    RUidMap[i] = idx;
    idx++;
  }
}

} // namespace o2::quality_control_modules::mft
