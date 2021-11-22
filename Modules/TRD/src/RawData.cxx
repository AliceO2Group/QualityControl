// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   RawData.cxx
/// \author Sean Murray
///

#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"

#include "QualityControl/QcInfoLogger.h"
#include "TRD/RawData.h"
#include "DataFormatsTRD/RawDataStats.h"
#include "DataFormatsTRD/Digit.h"
#include "DataFormatsTRD/Tracklet64.h"
#include "DataFormatsTRD/TriggerRecord.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <gsl/span>

using namespace o2::trd;

namespace o2::quality_control_modules::trd
{

RawData::~RawData()
{
}

void RawData::buildHistograms()
{
  std::array<std::string, 10> linkerrortitles = { "Count of Link had no errors during tf",
                                                  "Count of # times Linkerrors 0x1 seen per tf",
                                                  "Count of # time Linkerrors 0x2 seen per tf",
                                                  "Count of any Linkerror seen during tf",
                                                  "Link was seen with no data (empty) in a tf",
                                                  "Link was seen with data during a tf",
                                                  "Links seen with corrupted data during tf",
                                                  "Links seen with out corrupted data during tf",
                                                  "Accepted Data volume on link",
                                                  "Rejected Data volume on link" };
  std::array<std::string, 60> parsingerrortitle = {
    "TRDParsingNoError",
    "TRDParsingUnrecognisedVersion",
    "TRDParsingBadDigt",
    "TRDParsingBadTracklet",
    "TRDParsingDigitEndMarkerWrongState",
    "TRDParsingDigitMCMHeaderSanityCheckFailure",
    "TRDParsingDigitROBDecreasing",
    "TRDParsingDigitMCMNotIncreasing",
    "TRDParsingDigitADCMaskMismatch",
    "TRDParsingDigitADCMaskAdvanceToEnd",
    "TRDParsingDigitMCMHeaderBypassButStateMCMHeader",
    "TRDParsingDigitEndMarkerStateButReadingMCMADCData",
    "TRDParsingDigitADCChannel21",
    "TRDParsingDigitADCChannelGT22",
    "TRDParsingDigitGT10ADCs",
    "TRDParsingDigitSanityCheck",
    "TRDParsingDigitExcessTimeBins",
    "TRDParsingDigitParsingExitInWrongState",
    "TRDParsingDigitStackMismatch",
    "TRDParsingDigitLayerMismatch",
    "TRDParsingDigitSectorMismatch",
    "TRDParsingTrackletCRUPaddingWhileParsingTracklets",
    "TRDParsingTrackletBit11NotSetInTrackletHCHeader",
    "TRDParsingTrackletHCHeaderSanityCheckFailure",
    "TRDParsingTrackletMCMHeaderSanityCheckFailure",
    "TRDParsingTrackletMCMHeaderButParsingMCMData",
    "TRDParsingTrackletStateMCMHeaderButParsingMCMData",
    "TRDParsingTrackletTrackletCountGTThatDeclaredInMCMHeader",
    "TRDParsingTrackletInvalidTrackletCount",
    "TRDParsingTrackletPadRowIncreaseError",
    "TRDParsingTrackletColIncreaseError",
    "TRDParsingTrackletNoTrackletEndMarker",
    "TRDParsingTrackletExitingNoTrackletEndMarker",
    "TRDParsingDigitHeaderCountGT3",
    "TRDParsingDigitHeaderWrong1",
    "TRDParsingDigitHeaderWrong2",
    "TRDParsingDigitHeaderWrong3",
    "TRDParsingDigitHeaderWrong4",
    "TRDParsingDigitDataStillOnLink",
    "TRDParsingTrackletIgnoringDataTillEndMarker",
    "TRDLastParsingError"
  };

  mDataAcceptance = new TH1F("dataacceptance", "Data Accepted and Rejected", 2, 0, 2);
  getObjectsManager()->startPublishing(mDataAcceptance);
  mTimeFrameTime = new TH1F("timeframetime", "Time taken per time frame", 10000, 0, 10000);
  getObjectsManager()->startPublishing(mTimeFrameTime);
  mTrackletParsingTime = new TH1F("tracklettime", "Time taken per tracklet block", 1000, 0, 1000);
  getObjectsManager()->startPublishing(mTrackletParsingTime);
  mDigitParsingTime = new TH1F("digittime", "Time taken per digit block", 1000, 0, 1000);
  getObjectsManager()->startPublishing(mDigitParsingTime);
  mDataVersions = new TH1F("dataversions", "Data versions major.minor seen in data (half chamber header required)", 65000, 0, 65000);
  getObjectsManager()->startPublishing(mDataVersions);
  mDataVersionsMajor = new TH1F("dataversionsmajor", "Data versions major seen in the data (half chamber header required)", 256, 0, 256);
  getObjectsManager()->startPublishing(mDataVersionsMajor);
  mParsingErrors = new TH1F("parseerrors", "Parsing Errors seen in data", 256, 0, 256);
  getObjectsManager()->startPublishing(mParsingErrors);

  mTotalChargevsTimeBin = new TH1F("totalchargevstimebin", "Total Charge vs Timebin", 30, 0, 30);
  getObjectsManager()->startPublishing(mTotalChargevsTimeBin);
  mDataVolumePerHalfSector = new TH2F("datavolumeperhalfsector", "Event size per half chamber, from parsing", 1080, 0, 1080, 1000, 0, 1000);
  getObjectsManager()->startPublishing(mDataVolumePerHalfSector);
  getObjectsManager()->setDefaultDrawOptions("datavolumeperhalfsector", "COLZ");
  mDataVolumePerHalfSectorCru = new TH2F("datavolumeperhalfsectorcru", "Event size per half chamber, from cru header", 1080, 0, 1080, 1000, 0, 1000);
  getObjectsManager()->startPublishing(mDataVolumePerHalfSectorCru);
  getObjectsManager()->setDefaultDrawOptions("datavolumeperhalfsectorcru", "COLZ");
  int count = 0;
  for (int count = 0; count < o2::trd::TRDLastParsingError; ++count) {
    std::string label = fmt::format("parsingerrors_{0}", count);
    std::string title = parsingerrortitle[count];
    TH2F* h = new TH2F(label.c_str(), title.c_str(), 36, 0, 36, 30, 0, 30);
    mParsingErrors2d[count] = h;
    getObjectsManager()->startPublishing(h);
    getObjectsManager()->setDefaultDrawOptions(h->GetName(), "COLZ");
  }
  count = 0;
  for (int count = 0; count < 10; ++count) {
    std::string label = fmt::format("linkstatus_{0}", count);
    std::string title = linkerrortitles[count];
    TH2F* h = new TH2F(label.c_str(), title.c_str(), 36, 0, 36, 30, 0, 30);
    mLinkErrors[count] = h;
    getObjectsManager()->startPublishing(h);
    getObjectsManager()->setDefaultDrawOptions(h->GetName(), "COLZ");
  }
  mDataAcceptance->GetXaxis()->SetTitle("Type");
  mDataAcceptance->GetYaxis()->SetTitle("MBytes");
  mTimeFrameTime->GetXaxis()->SetTitle("Time taken in ms");
  mTimeFrameTime->GetYaxis()->SetTitle("Counts");

  mTrackletParsingTime->GetXaxis()->SetTitle("Time taken in #mus");
  mTrackletParsingTime->GetYaxis()->SetTitle("Counts");
  mDigitParsingTime->GetXaxis()->SetTitle("Time taken in #mus");
  mDigitParsingTime->GetYaxis()->SetTitle("Counts");
  mDigitParsingTime->GetYaxis()->SetTitle("Counts");
  mDataVersions->GetXaxis()->SetTitle("Version");
  mDataVersions->GetYaxis()->SetTitle("Counts");
  mDataVersionsMajor->GetYaxis()->SetTitle("Counts");
  mDataVersionsMajor->GetXaxis()->SetTitle("Version major");
  mParsingErrors->GetYaxis()->SetTitle("Counts");
  mParsingErrors->GetXaxis()->SetTitle("Error Types");
  mDataVolumePerHalfSector->GetXaxis()->SetTitle("half chamber");
  mDataVolumePerHalfSectorCru->GetXaxis()->SetTitle("half chamber");
  mDataVolumePerHalfSector->GetYaxis()->SetTitle("Data Volume [kB/event]");
  mDataVolumePerHalfSectorCru->GetYaxis()->SetTitle("Data Volume [kB/event]");
  for (int i = 0; i < o2::trd::TRDLastParsingError; ++i) {
    std::string label = fmt::format("{0}_{1}", parsingerrortitle[i], i);
    mParsingErrors->GetXaxis()->SetBinLabel(i + 1, label.c_str());
  }
  for (int count = 0; count < o2::trd::TRDLastParsingError; ++count) {
    TH2F* h = mParsingErrors2d[count];
    h->GetXaxis()->SetTitle("Sector*2 + side");
    h->GetXaxis()->CenterTitle(kTRUE);
    h->GetYaxis()->SetTitle("Stack_Layer");
    h->GetYaxis()->CenterTitle(kTRUE);
    for (int s = 0; s < o2::trd::constants::NSTACK; ++s) {
      for (int l = 0; l < o2::trd::constants::NLAYER; ++l) {
        std::string label = fmt::format("{0}_{1}", s, l);
        int pos = s * o2::trd::constants::NLAYER + l + 1;
        h->GetYaxis()->SetBinLabel(pos, label.c_str());
      }
    }
    for (int sm = 0; sm < o2::trd::constants::NSECTOR; ++sm) {
      for (int side = 0; side < 2; ++side) {
        std::string label = fmt::format("{0}.{1}", sm, side);
        int pos = sm * 2 + side + 1;
        h->GetXaxis()->SetBinLabel(pos, label.c_str());
      }
    }
  }
  for (int count = 0; count < 10; ++count) {
    TH2F* h = mLinkErrors[count];
    h->GetXaxis()->SetTitle("Sector*2 + side");
    h->GetXaxis()->CenterTitle(kTRUE);
    h->GetYaxis()->SetTitle("Stack_Layer");
    h->GetYaxis()->CenterTitle(kTRUE);
    for (int s = 0; s < o2::trd::constants::NSTACK; ++s) {
      for (int l = 0; l < o2::trd::constants::NLAYER; ++l) {
        std::string label = fmt::format("{0}.{1}", s, l);
        int pos = s * o2::trd::constants::NLAYER + l + 1;
        h->GetYaxis()->SetBinLabel(pos, label.c_str());
      }
    }
    for (int sm = 0; sm < o2::trd::constants::NSECTOR; ++sm) {
      for (int side = 0; side < 2; ++side) {
        std::string label = fmt::format("{0}_{1}", sm, side);
        int pos = sm * 2 + side + 1;
        h->GetXaxis()->SetBinLabel(pos, label.c_str());
      }
    }
  }

  for (Int_t det = 0; det < 540; ++det) {
    fClusterChamberAmplitude[det] = new TH1F(Form("clustramplitude_%d", det), "", 300, -0.5, 299.5);
  }
}

void RawData::initialize(o2::framework::InitContext& /*ctx*/)
{

  ILOG(Info, Support) << "initialize TRD RawData QC " << ENDM;

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  //if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
  //  ILOG(Info, Devel) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  // } commented out for now I will come back to this.

  buildHistograms();
  ILOG(Info, Support) << "TRD RawData QC histograms built" << ENDM;

  // ILOG(Warning, Support) << "Metadata could not be added to " << mHistogram->GetName() << ENDM;
}

void RawData::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  resetHistograms();
}

void RawData::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void RawData::monitorData(o2::framework::ProcessingContext& ctx)
{

  auto rawdatastats = ctx.inputs().get<o2::trd::TRDDataCountersPerTimeFrame>("rawstats");
  auto digits = ctx.inputs().get<gsl::span<o2::trd::Digit>>("digits");
  auto tracklets = ctx.inputs().get<gsl::span<o2::trd::Tracklet64>>("tracklets");
  auto triggerrecords = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");

  //RAWSTATS is the RawDataStats class which is essentially histograms.
  //loop through all the bits and pieces and fill the histograms

  std::array<uint16_t, o2::trd::constants::MAXCHAMBER * 2> eventsize{};
  // triggerrecords is a span of our triggers in the respective time frame
  int lastmcm = 0;
  uint64_t digitcount = 0;
  uint64_t trackletcount = 0;
  for (auto& trigger : triggerrecords) {
    uint64_t numtracklets = trigger.getNumberOfTracklets();
    uint64_t numdigits = trigger.getNumberOfDigits();
    uint64_t digitend = trigger.getFirstDigit() + trigger.getNumberOfDigits();
    uint64_t trackletend = trigger.getFirstTracklet() + trigger.getNumberOfTracklets();
    for (int tracklet = trigger.getFirstTracklet(); tracklet < trackletend; ++tracklet) {
      eventsize[tracklets[tracklet].getHCID()]++;
      if (lastmcm != tracklets[tracklet].getMCM()) {
        eventsize[tracklets[tracklet].getHCID()]++; // include the mcmheader in the data volume calculation
        lastmcm = tracklets[tracklet].getMCM();
      }
    }
    for (int digit = trigger.getFirstDigit(); digit < digitend; ++digit) {
      eventsize[digits[digit].getHCId()] += 12;
    }
  }
  //data per event per link.
  for (int hcid = 0; hcid < 1080; ++hcid) {
    if (eventsize[hcid] > 0) {
      mDataVolumePerHalfSector->Fill(hcid, eventsize[hcid] / 256); // conver 32bit words to bytes then to kb    *4/1024=/256
    }
  }
  for (int error; error < o2::trd::TRDLastParsingError; ++error) {
    mParsingErrors->AddBinContent(error, rawdatastats.mParsingErrors[error]);
    //std::cout << "Increment parsing errors 1d at : " << rawdatastats.mParsingErrors[error] << " for error:" << error<< std::endl;
    for (int sm = 0; sm < o2::trd::constants::NSECTOR * 2 - 1; ++sm) {
      for (int stacklayer = 0; stacklayer < 30; ++stacklayer) {
        int specoffset = sm * 30 * o2::trd::TRDLastParsingError + stacklayer * o2::trd::TRDLastParsingError + error;
        if (rawdatastats.mParsingErrorsByLink[specoffset] > 0) {
          mParsingErrors2d[error]->SetBinContent(sm, stacklayer, mParsingErrors2d[error]->GetBinContent(sm, stacklayer) + rawdatastats.mParsingErrorsByLink[specoffset]);
        }
      }
    }
  }
  //linkstatus:
  for (int sm = 0; sm < o2::trd::constants::NSECTOR * 2 - 1; ++sm) {
    for (int stacklayer = 0; stacklayer < 30; ++stacklayer) {
      int specoffset = sm * 30 + stacklayer;
      if (rawdatastats.mLinkErrorFlag[specoffset] == 0) { //"Count of Link had no errors during tf",
        mLinkErrors[0]->Fill(sm, stacklayer);
      }
      if (rawdatastats.mLinkErrorFlag[specoffset] & 0x1) { //"Count of # times Linkerrors 0x1 seen per tf",
        mLinkErrors[1]->Fill(sm, stacklayer, rawdatastats.mLinkErrorFlag[specoffset]);
      }
      if (rawdatastats.mLinkErrorFlag[specoffset] & 0x2) { //"Count of # time Linkerrors 0x2 seen per tf",
        mLinkErrors[2]->Fill(sm, stacklayer, rawdatastats.mLinkErrorFlag[specoffset]);
      }
      if (rawdatastats.mLinkErrorFlag[specoffset] != 0) { //"Count of any Linkerror seen during tf",
        mLinkErrors[3]->Fill(sm, stacklayer, rawdatastats.mLinkErrorFlag[specoffset]);
      }
      if (rawdatastats.mLinkWordsRejected[specoffset] + rawdatastats.mLinkWordsRead[specoffset] == 0) { //LinkWords[specoffset]==0){//"Link was seen with no data (empty) in a tf",
        mLinkErrors[4]->Fill(sm, stacklayer);                                                           //,rawdatastats.mLinkNoData[specoffset]);
      } else {
        if (rawdatastats.mLinkWords[specoffset] > 0) {
          mDataVolumePerHalfSectorCru->Fill(sm, stacklayer, rawdatastats.mLinkWords[specoffset] / 32); // 32 because convert 256bit into bytes and then to kb, *32/1024 == /32
        }
      }
      if (rawdatastats.mLinkWordsRead[specoffset] > 0) { //"Link was seen with data during a tf",
        mLinkErrors[5]->Fill(sm, stacklayer, rawdatastats.mLinkWordsRead[specoffset]);
      }
      if (rawdatastats.mLinkWordsRejected[specoffset] > 0) { //"Links seen with corrupted data during tf"
        mLinkErrors[6]->Fill(sm, stacklayer);
      }
      if (rawdatastats.mLinkWordsRejected[specoffset] < 20) { //"Links seen with out corrupted data during tf", "",
        mLinkErrors[7]->Fill(sm, stacklayer);
      }
      if (rawdatastats.mLinkWordsRead[specoffset] != 0) { //"Accepted Data volume on link",
        mLinkErrors[8]->Fill(sm, stacklayer, rawdatastats.mLinkWordsRead[specoffset]);
        mDataAcceptance->AddAt(((float)rawdatastats.mLinkWordsRead[specoffset]) / 1024.0 / 256.0, 0);
      }
      if (rawdatastats.mLinkWordsRejected[specoffset] != 0) { // "Rejected Data volume on link"};
        mLinkErrors[9]->Fill(sm, stacklayer, rawdatastats.mLinkWordsRejected[specoffset]);
        mDataAcceptance->AddAt(((float)rawdatastats.mLinkWordsRead[specoffset]) / 1024.0 / 256.0, 1);
      }
    }
  }
  //time graphs
  mTimeFrameTime->Fill(rawdatastats.mTimeTaken);
  mDigitParsingTime->Fill(rawdatastats.mTimeTakenForDigits);
  mTrackletParsingTime->Fill(rawdatastats.mTimeTakenForTracklets);

  for (int i = 0; i < rawdatastats.mDataFormatRead.size(); ++i) {
    mDataVersionsMajor->Fill(i, rawdatastats.mDataFormatRead[i]);
  }
  LOG(info) << "for TF : D:" << digitcount << " T:" << trackletcount;
}

void RawData::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void RawData::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void RawData::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  resetHistograms();
}
void RawData::resetHistograms()
{
  ILOG(Info, Support) << "Resetting the histogram " << ENDM;
  for (auto hist : mLinkErrors) {
    hist->Reset();
  }
  for (auto hist : mParsingErrors2d) {
    hist->Reset();
  }
  mTimeFrameTime->Reset();
  mTrackletParsingTime->Reset();
  mDigitParsingTime->Reset();
  mDataVersions->Reset();
  mDataVersionsMajor->Reset();
  mParsingErrors->Reset();
  mDataVolumePerHalfSector->Reset();
  mDataVolumePerHalfSectorCru->Reset();
  mTotalChargevsTimeBin->Reset();
  //TODO come back and change the drawing of these and labels
}

} // namespace o2::quality_control_modules::trd
