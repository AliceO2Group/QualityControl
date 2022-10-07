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
#include <Framework/InputRecord.h>
#include <ITSMFTReconstruction/ChipMappingITS.h>

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
  delete mLinkErrorPlots;
  delete mChipErrorPlots;
  delete mLinkErrorVsFeeid;
  delete mChipErrorVsFeeid;
  for (int ifee = 0; ifee < NFees; ifee++) {
    delete[] mLinkErrorCount[ifee];
    delete[] mChipErrorCount[ifee];
  }
  delete[] mLinkErrorCount;
  delete[] mChipErrorCount;
}

void ITSDecodingErrorTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "Initializing the ITSDecodingErrorTask" << ENDM;
  getParameters();
  createDecodingPlots();
  setPlotsFormat();
  mLinkErrorCount = new int*[NFees];
  mChipErrorCount = new int*[NFees];
  for (int ifee = 0; ifee < NFees; ifee++) {
    // for link statistic
    mLinkErrorCount[ifee] = new int[o2::itsmft::GBTLinkDecodingStat::NErrorsDefined];
    mChipErrorCount[ifee] = new int[o2::itsmft::ChipStat::NErrorsDefined];
    for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
      mLinkErrorCount[ifee][ierror] = 0;
    }
    for (int ierror = 0; ierror < o2::itsmft::ChipStat::NErrorsDefined; ierror++) {
      mChipErrorCount[ifee][ierror] = 0;
    }
  }
}

void ITSDecodingErrorTask::createDecodingPlots()
{
  mLinkErrorVsFeeid = new TH2I("General/LinkErrorVsFeeid", "GBTLink errors per FeeId", NFees, 0, NFees, o2::itsmft::GBTLinkDecodingStat::NErrorsDefined, 0.5, o2::itsmft::GBTLinkDecodingStat::NErrorsDefined + 0.5);
  mLinkErrorVsFeeid->SetMinimum(0);
  mLinkErrorVsFeeid->SetStats(0);
  getObjectsManager()->startPublishing(mLinkErrorVsFeeid);
  mChipErrorVsFeeid = new TH2I("General/ChipErrorVsFeeid", "Chip decoding errors per FeeId", NFees, 0, NFees, o2::itsmft::ChipStat::NErrorsDefined, 0.5, o2::itsmft::ChipStat::NErrorsDefined + 0.5);
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
  //
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  // int difference;
  start = std::chrono::high_resolution_clock::now();

  std::vector<InputSpec> rawDataFilter{ InputSpec{ "", ConcreteDataTypeMatcher{ "DS", "RAWDATA0" }, Lifetime::Timeframe } };

  rawDataFilter.push_back(InputSpec{ "", ConcreteDataTypeMatcher{ "ITS", "RAWDATA" }, Lifetime::Timeframe });
  DPLRawParser parser(ctx.inputs(), rawDataFilter);

  auto linkErrors = ctx.inputs().get<gsl::span<o2::itsmft::GBTLinkDecodingStat>>("linkerrors");
  auto decErrors = ctx.inputs().get<gsl::span<o2::itsmft::ChipError>>("decerrors");
  for (const auto& le : linkErrors) {
    int istave = (int)(le.feeID & 0x00ff);
    int ilink = (int)((le.feeID & 0x0f00) >> 8);
    int ilayer = (int)((le.feeID & 0xf000) >> 12);
    int ifee = 3 * StaveBoundary[ilayer] - (StaveBoundary[ilayer] - StaveBoundary[NLayerIB]) * (ilayer >= NLayerIB) + istave * (3 - (ilayer >= NLayerIB)) + ilink;
    for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
      if (le.errorCounts[ierror] <= 0) {
        continue;
      }
      mLinkErrorCount[ifee][ierror] = le.errorCounts[ierror];
    }
  }
  for (int ifee = 0; ifee < NFees; ifee++) {
    for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
      if (mLinkErrorVsFeeid && (mLinkErrorCount[ifee][ierror] != 0)) {
        mLinkErrorVsFeeid->SetBinContent(ifee + 1, ierror + 1, mLinkErrorCount[ifee][ierror]);
      }
    }
  }
  for (const auto& de : decErrors) {
    int istave = (int)(de.getFEEID() & 0x00ff);
    int ilink = (int)((de.getFEEID() & 0x0f00) >> 8);
    int ilayer = (int)((de.getFEEID() & 0xf000) >> 12);
    int ifee = 3 * StaveBoundary[ilayer] - (StaveBoundary[ilayer] - StaveBoundary[NLayerIB]) * (ilayer >= NLayerIB) + istave * (3 - (ilayer >= NLayerIB)) + ilink;
    for (int ierror = 0; ierror < o2::itsmft::ChipStat::NErrorsDefined; ierror++) {
      if ((de.errors >> ierror) % 2) {
        mChipErrorVsFeeid->Fill(ifee + 1, ierror + 1);
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
  end = std::chrono::high_resolution_clock::now();
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
  ILOG(Info, Support) << "Reset" << ENDM;
}

} // namespace o2::quality_control_modules::its
