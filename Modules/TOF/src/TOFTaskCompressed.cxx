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
/// \file   TOFTaskCompressed.cxx
/// \author Nicolo' Jacazio
///

// ROOT includes
#include <TCanvas.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TH1I.h>
#include <TH2I.h>

// O2 includes
#include "TOFCompression/RawDataFrame.h"
#include "DataFormatsTOF/CompressedDataFormat.h"
#include "Headers/RAWDataHeader.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TOF/TOFTaskCompressed.h"

namespace o2::quality_control_modules::tof
{

Int_t TOFTaskCompressed::fgNbinsMultiplicity = 2000;       /// Number of bins in multiplicity plot
Int_t TOFTaskCompressed::fgRangeMinMultiplicity = 0;       /// Min range in multiplicity plot
Int_t TOFTaskCompressed::fgRangeMaxMultiplicity = 1000;    /// Max range in multiplicity plot
Int_t TOFTaskCompressed::fgNbinsTime = 250;                /// Number of bins in time plot
const Float_t TOFTaskCompressed::fgkNbinsWidthTime = 2.44; /// Width of bins in time plot
Float_t TOFTaskCompressed::fgRangeMinTime = 0.0;           /// Range min in time plot
Float_t TOFTaskCompressed::fgRangeMaxTime = 620.0;         /// Range max in time plot
Int_t TOFTaskCompressed::fgCutNmaxFiredMacropad = 50;      /// Cut on max number of fired macropad
const Int_t TOFTaskCompressed::fgkFiredMacropadLimit = 50; /// Limit on cut on number of fired macropad

TOFTaskCompressed::TOFTaskCompressed() : TaskInterface(),
                                         mHisto(nullptr),
                                         mTime(nullptr),
                                         mTOT(nullptr),
                                         mIndexE(nullptr),
                                         mSlotEnableMask(nullptr),
                                         mDiagnostic(nullptr)
{
}

TOFTaskCompressed::~TOFTaskCompressed()
{
  mHisto.reset();
  mTime.reset();
  mTOT.reset();
  mIndexE.reset();
  mSlotEnableMask.reset();
  mDiagnostic.reset();
}

void TOFTaskCompressed::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize TOFTaskCompressed" << ENDM;

  mHisto.reset(new TH1F("hHisto", "hHisto", 1000, 0., 1000.));
  getObjectsManager()->startPublishing(mHisto.get());
  mTime.reset(new TH1F("hTime", "hTime;time (24.4 ps)", 2097152, 0., 2097152.));
  getObjectsManager()->startPublishing(mTime.get());
  mTOT.reset(new TH1F("hTOT", "hTOT;ToT (48.8 ps)", 2048, 0., 2048.));
  getObjectsManager()->startPublishing(mTOT.get());
  mIndexE.reset(new TH1F("hIndexE", "hIndexE;index EO", 172800, 0., 172800.));
  getObjectsManager()->startPublishing(mIndexE.get());
  mSlotEnableMask.reset(new TH2F("hSlotEnableMask", "hSlotEnableMask;crate;slot", 72, 0., 72., 12, 1., 13.));
  getObjectsManager()->startPublishing(mSlotEnableMask.get());
  mDiagnostic.reset(new TH2F("hDiagnostic", "hDiagnostic;crate;slot", 72, 0., 72., 12, 1., 13.));
  getObjectsManager()->startPublishing(mDiagnostic.get());
}

void TOFTaskCompressed::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
  mHisto->Reset();
  mTime->Reset();
  mTOT->Reset();
  mIndexE->Reset();
  mSlotEnableMask->Reset();
  mDiagnostic->Reset();
}

void TOFTaskCompressed::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void TOFTaskCompressed::monitorData(o2::framework::ProcessingContext& ctx)
{
    LOG(INFO) << "Monitoring in the TOFCompressed Task "<<ENDM;

//     TH1F *hasd = nullptr;
// hasd->GetName();
  /** check status **/
  //   if (mStatus) {
  //     ctx.services().get<ControlService>().readyToQuit(QuitRequest::Me);
  //     return;
  //   }

  /** receive input **/
  auto dataFrame = ctx.inputs().get<o2::tof::RawDataFrame*>("dataframe");
  auto pointer = dataFrame->mBuffer;
  /** process input **/
  while (pointer < (dataFrame->mBuffer + dataFrame->mSize)) {
    auto rdh = reinterpret_cast<o2::header::RAWDataHeader*>(pointer);

    /** RDH close detected **/
    if (rdh->stop) {
      // #ifdef VERBOSE
      //       std::cout << "--- RDH close detected" << std::endl;
      //       o2::utils::HBFUtils::printRDH(*rdh);
      // #endif
      pointer += rdh->offsetToNext;
      continue;
    }

    // #ifdef VERBOSE
    //     std::cout << "--- RDH open detected" << std::endl;
    //     o2::utils::HBFUtils::printRDH(*rdh);
    // #endif

    pointer += rdh->headerSize;

    while (pointer < (reinterpret_cast<char*>(rdh) + rdh->memorySize)) {

      auto word = reinterpret_cast<uint32_t*>(pointer);
      if ((*word & 0x80000000) != 0x80000000) {
        printf(" %08x [ERROR] \n ", *(uint32_t*)pointer);
        return;
      }

      /** crate header detected **/
      auto crateHeader = reinterpret_cast<o2::tof::compressed::CrateHeader_t*>(pointer);
      // #ifdef VERBOSE
      //       printf(" %08x CrateHeader          (drmID=%d) \n ", *(uint32_t*)pointer, crateHeader->drmID);
      // #endif
      for (int ibit = 0; ibit < 11; ++ibit)
        if (crateHeader->slotEnableMask & (1 << ibit))
          mSlotEnableMask->Fill(crateHeader->drmID, ibit + 2);
      pointer += 4;

      /** crate orbit expected **/
      auto crateOrbit = reinterpret_cast<o2::tof::compressed::CrateOrbit_t*>(pointer);
      // #ifdef VERBOSE
      //       printf(" %08x CrateOrbit           (orbit=0x%08x) \n ", *(uint32_t*)pointer, crateOrbit->orbitID);
      // #endif
      pointer += 4;

      while (true) {
        word = reinterpret_cast<uint32_t*>(pointer);

        /** crate trailer detected **/
        if (*word & 0x80000000) {
          auto crateTrailer = reinterpret_cast<o2::tof::compressed::CrateTrailer_t*>(pointer);
          // #ifdef VERBOSE
          // 	  printf(" %08x CrateTrailer         (numberOfDiagnostics=%d) \n ", *(uint32_t*)pointer, crateTrailer->numberOfDiagnostics);
          // #endif
          pointer += 4;

          /** loop over diagnostics **/
          for (int i = 0; i < crateTrailer->numberOfDiagnostics; ++i) {
            auto diagnostic = reinterpret_cast<o2::tof::compressed::Diagnostic_t*>(pointer);
            // #ifdef VERBOSE
            // 	    printf(" %08x Diagnostic           (slotId=%d) \n ", *(uint32_t*)pointer, diagnostic->slotID);
            // #endif
            mDiagnostic->Fill(crateHeader->drmID, diagnostic->slotID);
            pointer += 4;
          }

          break;
        }

        /** frame header detected **/
        auto frameHeader = reinterpret_cast<o2::tof::compressed::FrameHeader_t*>(pointer);
        // #ifdef VERBOSE
        // 	printf(" %08x FrameHeader          (numberOfHits=%d) \n ", *(uint32_t*)pointer, frameHeader->numberOfHits);
        // #endif
        mHisto->Fill(frameHeader->numberOfHits);
        pointer += 4;

        /** loop over hits **/
        for (int i = 0; i < frameHeader->numberOfHits; ++i) {
          auto packedHit = reinterpret_cast<o2::tof::compressed::PackedHit_t*>(pointer);
          // #ifdef VERBOSE
          // 	  printf(" %08x PackedHit            (tdcID=%d) \n ", *(uint32_t*)pointer, packedHit->tdcID);
          // #endif
          auto indexE = packedHit->channel +
                        8 * packedHit->tdcID +
                        120 * packedHit->chain +
                        240 * (frameHeader->trmID - 3) +
                        2400 * crateHeader->drmID;
          int time = packedHit->time;
          time += (frameHeader->frameID << 13);

          mIndexE->Fill(indexE);
          mTime->Fill(time);
          mTOT->Fill(packedHit->tot);
          pointer += 4;
        }
      }
    }

    pointer = reinterpret_cast<char*>(rdh) + rdh->offsetToNext;
  }
}

void TOFTaskCompressed::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
}

void TOFTaskCompressed::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void TOFTaskCompressed::reset()
{
  // clean all the monitor objects here

  ILOG(Info) << "Resetting the histogram" << ENDM;
  mHisto->Reset();
  mTime->Reset();
  mTOT->Reset();
  mIndexE->Reset();
  mSlotEnableMask->Reset();
  mDiagnostic->Reset();
}

} // namespace o2::quality_control_modules::tof
