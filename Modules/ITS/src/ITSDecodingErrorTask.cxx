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
/// \file   ITSDecodingErrorTask.cxx
/// \author Zhen Zhang
///

#include "ITS/ITSDecodingErrorTask.h"
#include "QualityControl/QcInfoLogger.h"
#include "ITSMFTReconstruction/DecodingStat.h"

#include <DPLUtils/RawParser.h>
#include <DPLUtils/DPLRawParser.h>
#include <iostream>

using namespace o2::framework;
using namespace o2::itsmft;
using namespace o2::header;

namespace o2::quality_control_modules::its
{

ITSDecodingErrorTask::ITSDecodingErrorTask()
  : TaskInterface()
{
}

ITSDecodingErrorTask::~ITSDecodingErrorTask()
{
  delete mDecoder;
  delete mLinkErrorPlots;
  delete mChipErrorPlots;
  delete mLinkErrorVsFeeid;
  delete mChipErrorVsFeeid;
  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    int maxlink = ilayer < NLayerIB ? 3 : 2;
    for (int istave = 0; istave < NStaves[ilayer]; istave++) {
      for (int ilink = 0; ilink < maxlink; ilink++) {
        delete[] mLinkErrorCount[ilayer][istave][ilink];
        delete[] mChipErrorCount[ilayer][istave][ilink];
      }

      delete[] mLinkErrorCount[ilayer][istave];
      delete[] mChipErrorCount[ilayer][istave];
    }
    delete[] mLinkErrorCount[ilayer];
    delete[] mChipErrorCount[ilayer];
  }
  delete[] mLinkErrorCount;
  delete[] mChipErrorCount;

  // delete mInfoCanvas;
}

void ITSDecodingErrorTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "Initializing the ITSDecodingErrorTask" << ENDM;
  getParameters();
  createDecodingPlots();
  setPlotsFormat();

  mDecoder = new o2::itsmft::RawPixelDecoder<o2::itsmft::ChipMappingITS>();
  mDecoder->init();
  mDecoder->setNThreads(mNThreads);
  mDecoder->setFormat(GBTLink::NewFormat);               // Using RDHv6 (NewFormat)
  mDecoder->setUserDataOrigin(header::DataOrigin("DS")); // set user data origin in dpl
  mDecoder->setUserDataDescription(header::DataDescription("RAWDATA0"));

  mLinkErrorCount = new int***[NLayer];
  mChipErrorCount = new int***[NLayer];
  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    mLinkErrorCount[ilayer] = new int**[NStaves[ilayer]];
    mChipErrorCount[ilayer] = new int**[NStaves[ilayer]];
    int maxlink = ilayer < NLayerIB ? 3 : 2;
    for (int istave = 0; istave < NStaves[ilayer]; istave++) {
      mLinkErrorCount[ilayer][istave] = new int*[maxlink];
      mChipErrorCount[ilayer][istave] = new int*[maxlink];
      // for link statistic
      for (int ilink = 0; ilink < maxlink; ilink++) {
        mLinkErrorCount[ilayer][istave][ilink] = new int[o2::itsmft::GBTLinkDecodingStat::NErrorsDefined];
        mChipErrorCount[ilayer][istave][ilink] = new int[o2::itsmft::ChipStat::NErrorsDefined];
        for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
          mLinkErrorCount[ilayer][istave][ilink][ierror] = 0;
        }
        for (int ierror = 0; ierror < o2::itsmft::ChipStat::NErrorsDefined; ierror++) {
          mChipErrorCount[ilayer][istave][ilink][ierror] = 0;
        }
      }
    }
  }
}

void ITSDecodingErrorTask::createDecodingPlots()
{


  mLinkErrorVsFeeid = new TH2I("General/LinkErrorVsFeeid", "GBTLink errors per FeeId", (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), 0, (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), o2::itsmft::GBTLinkDecodingStat::NErrorsDefined, 0.5, o2::itsmft::GBTLinkDecodingStat::NErrorsDefined + 0.5);
  mLinkErrorVsFeeid->SetMinimum(0);
  mLinkErrorVsFeeid->SetStats(0);
  getObjectsManager()->startPublishing(mLinkErrorVsFeeid);

  mChipErrorVsFeeid = new TH2I("General/ChipErrorVsFeeid", "Chip decoding errors per FeeId", (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), 0, (3 * StaveBoundary[3]) + (2 * (StaveBoundary[7] - StaveBoundary[3])), o2::itsmft::ChipStat::NErrorsDefined, 0.5, o2::itsmft::ChipStat::NErrorsDefined + 0.5);
  mChipErrorVsFeeid->SetMinimum(0);
  mChipErrorVsFeeid->SetStats(0);
  getObjectsManager()->startPublishing(mChipErrorVsFeeid);

  mLinkErrorPlots = new TH1D("General/LinkErrorPlots", "GBTLink decoding Errors", o2::itsmft::GBTLinkDecodingStat::NErrorsDefined, 0.5, o2::itsmft::GBTLinkDecodingStat::NErrorsDefined + 0.5);
  mLinkErrorPlots->SetMinimum(0);
  mLinkErrorPlots->SetFillColor(kRed);
  getObjectsManager()->startPublishing(mLinkErrorPlots); // mLinkErrorPlots

  mChipErrorPlots = new TH1D("General/ChipErrorPlots", "Chip Decoding Errors", o2::itsmft::ChipStat::NErrorsDefined, 0.5, o2::itsmft::ChipStat::NErrorsDefined + 0.5);
  mChipErrorPlots->SetMinimum(0);
  mChipErrorPlots->SetFillColor(kRed);
  getObjectsManager()->startPublishing(mChipErrorPlots); // mChipErrorPlots

}

void ITSDecodingErrorTask::setAxisTitle(TH1* object, const char* xTitle, const char* yTitle)
{
  object->GetXaxis()->SetTitle(xTitle);
  object->GetYaxis()->SetTitle(yTitle);
}


void ITSDecodingErrorTask::setPlotsFormat()
{

  if (mLinkErrorVsFeeid) {
    setAxisTitle(mLinkErrorVsFeeid, "FeeID", "Error ID");
  }
  if (mChipErrorVsFeeid) {
    setAxisTitle(mChipErrorVsFeeid, "FeeID", "Error ID");
  }

  if (mLinkErrorPlots) {
    setAxisTitle(mLinkErrorPlots, "LinkError ID", "Counts");
  }
  if (mChipErrorPlots) {
    setAxisTitle(mChipErrorPlots, "ChipError ID", "Counts");
  }

}

void ITSDecodingErrorTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity : " << activity.mId << ENDM;
  mRunNumber = activity.mId;
}

void ITSDecodingErrorTask::startOfCycle() { ILOG(Info, Support) << "startOfCycle" << ENDM; }

void ITSDecodingErrorTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // set timer
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();

  std::vector<InputSpec> rawDataFilter{ InputSpec{ "", ConcreteDataTypeMatcher{ "DS", "RAWDATA0" }, Lifetime::Timeframe } };

  rawDataFilter.push_back(InputSpec{ "", ConcreteDataTypeMatcher{ "ITS", "RAWDATA" }, Lifetime::Timeframe });
  DPLRawParser parser(ctx.inputs(), rawDataFilter);

  mDecoder->startNewTF(ctx.inputs());
  mDecoder->setDecodeNextAuto(true);
  

  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {

    auto const* rdh = it.get_if<o2::header::RAWDataHeaderV6>();
    // Decoding data format (RDHv6)
    int istave = (int)(rdh->feeId & 0x00ff);
    int ilink = (int)((rdh->feeId & 0x0f00) >> 8);
    int ilayer = (int)((rdh->feeId & 0xf000) >> 12);
    int ifee = 3 * StaveBoundary[ilayer] - (StaveBoundary[ilayer] - StaveBoundary[NLayerIB]) * (ilayer >= NLayerIB) + istave * (3 - (ilayer >= NLayerIB)) + ilink;
    int memorysize = (int)(rdh->memorySize);
    int headersize = (int)(rdh->headerSize);

    bool clockEvt = false;
    auto const* DecoderTmp = mDecoder;
    int RUid = StaveBoundary[ilayer] + istave;
    const o2::itsmft::RUDecodeData* RUdecode = DecoderTmp->getRUDecode(RUid);
    if (!RUdecode) {
      continue;
    }

    auto linkErrors = ctx.inputs().get<gsl::span<o2::itsmft::GBTLinkDecodingStat>>("linkerrors");
    auto detErrors = ctx.inputs().get<gsl::span<o2::itsmft::ChipError>>("decerrors");
    for (const auto& le : linkErrors) {
      for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
        if (linkErrors.statistics.errorCounts[ierror] <= 0) {
	  continue;
        }
        mLinkErrorCount[ilayer][istave][ilink][ierror]=linkErrors.statistics.errorCounts[ierror];
        if (mLinkErrorVsFeeid && (mLinkErrorCount[ilayer][istave][ilink][ierror] != 0)) {
          mLinkErrorVsFeeid->SetBinContent(ifee + 1, ierror + 1, mLinkErrorCount[ilayer][istave][ilink][ierror]);
        }
      }
    }
    for (const auto& de : decErrors) {
      LOGP(info, "err in FeeID:{:#x} Chip:{} ErrorBits {:#b}, {} errors in TF", de.getFEEID(), de.getChipID(), de.errors, de.nerrors);
      for (int ierror = 0; ierror < o2::itsmft::ChipStat::NErrorsDefined; ierror++) {
        if (de.chipStat.errorCounts[ierror] <= 0) {
          continue;
        }
        mChipErrorCount[ilayer][istave][ilink][ierror] = de.chipStat.errorCounts[ierror];
        if (mChipErrorVsFeeid && (mChipErrorCount[ilayer][istave][ilink][ierror] != 0)) {
          mChipErrorVsFeeid->SetBinContent(ifee + 1, ierror + 1, mChipErrorCount[ilayer][istave][ilink][ierror]);
        }
      }
    }
  }

  for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
    int feeLinkError = mLinkErrorVsFeeid->Integral(1, mLinkErrorVsFeeid->GetXaxis()->GetNbins(), ierror + 1, ierror + 1);
    mLinkErrorPlots->SetBinContent(ierror + 1, feeLinkError);
  }
  for (int ierror = 0; ierror < o2::itsmft::ChipStat::NErrorsDefined; ierror++) {
    int feeChipError = mChipErrorVsFeeid->Integral(1, mChipErrorVsFeeid->GetXaxis()->GetNbins(), ierror + 1, ierror + 1);
    mChipErrorPlots->SetBinContent(ierror + 1, feeChipError);
  }

  // Filling histograms: loop over mStatusFlagNumber[ilayer][istave][ilane][iflag]


  mTimeFrameId = ctx.inputs().get<int>("G");

  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

void ITSDecodingErrorTask::getParameters()
{
  mNPayloadSizeBins = std::stoi(mCustomParameters["NPayloadSizeBins"]);
}


void ITSDecodingErrorTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void ITSDecodingErrorTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ITSDecodingErrorTask::resetGeneralPlots()
{
  mLinkErrorVsFeeid->Reset();
  mChipErrorVsFeeid->Reset();
  mLinkErrorPlots->Reset();
  mChipErrorPlots->Reset();
}

void ITSDecodingErrorTask::reset()
{
  resetGeneralPlots();
  mDecoder->clearStat();
  ILOG(Info, Support) << "Reset" << ENDM;
}

} // namespace o2::quality_control_modules::its
