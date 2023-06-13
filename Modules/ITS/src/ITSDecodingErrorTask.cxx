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
#include "ITSMFTReconstruction/DecodingStat.h"
#include <Framework/InputRecord.h>

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
  for (int ilayer = 0; ilayer < 7; ilayer++) {
    delete mChipErrorVsChipid[ilayer];
  }
}

void ITSDecodingErrorTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initializing the ITSDecodingErrorTask" << ENDM;
  getParameters();
  createDecodingPlots();
  setPlotsFormat();
}

void ITSDecodingErrorTask::createDecodingPlots()
{
  mLinkErrorVsFeeid = new TH2D("General/LinkErrorVsFeeid", "GBTLink errors per FeeId", NFees, 0, NFees, o2::itsmft::GBTLinkDecodingStat::NErrorsDefined, 0.5, o2::itsmft::GBTLinkDecodingStat::NErrorsDefined + 0.5);
  mLinkErrorVsFeeid->SetMinimum(0);
  mLinkErrorVsFeeid->SetStats(0);
  getObjectsManager()->startPublishing(mLinkErrorVsFeeid);
  for (int ilayer = 0; ilayer < 7; ilayer++) {
    mChipErrorVsChipid[ilayer] = new TH2D(Form("General/Layer%dChipErrorVsChipid", ilayer), Form("Layer%d Chip errors per FeeId", ilayer), ChipBoundary[ilayer + 1] - ChipBoundary[ilayer], 0, ChipBoundary[ilayer + 1] - ChipBoundary[ilayer], o2::itsmft::ChipStat::NErrorsDefined, 0.5, o2::itsmft::ChipStat::NErrorsDefined + 0.5);
    mChipErrorVsChipid[ilayer]->SetMinimum(0);
    mChipErrorVsChipid[ilayer]->SetStats(0);
    getObjectsManager()->startPublishing(mChipErrorVsChipid[ilayer]);
  }
  mChipErrorVsFeeid = new TH2D("General/ChipErrorVsFeeid", "Chip decoding errors per FeeId", NFees, 0, NFees, o2::itsmft::ChipStat::NErrorsDefined, 0.5, o2::itsmft::ChipStat::NErrorsDefined + 0.5);
  mChipErrorVsFeeid->SetMinimum(0);
  mChipErrorVsFeeid->SetStats(0);
  getObjectsManager()->startPublishing(mChipErrorVsFeeid);
  mLinkErrorPlots = new TH1D("General/LinkErrorPlots", "GBTLink decoding Errors", o2::itsmft::GBTLinkDecodingStat::NErrorsDefined, 0.5, o2::itsmft::GBTLinkDecodingStat::NErrorsDefined + 0.5);
  mLinkErrorPlots->SetMinimum(0);
  mLinkErrorPlots->SetStats(0);
  mLinkErrorPlots->SetFillColor(kRed);
  getObjectsManager()->startPublishing(mLinkErrorPlots); // mLinkErrorPlots
  mChipErrorPlots = new TH1D("General/ChipErrorPlots", "Chip Decoding Errors", o2::itsmft::ChipStat::NErrorsDefined, 0.5, o2::itsmft::ChipStat::NErrorsDefined + 0.5);
  mChipErrorPlots->SetMinimum(0);
  mChipErrorPlots->SetStats(0);
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
  for (int ilayer = 0; ilayer < 7; ilayer++) {
    if (mChipErrorVsChipid[ilayer]) {
      setAxisTitle(mChipErrorVsChipid[ilayer], "ChipID", "Error ID");
    }
  }
  if (mLinkErrorPlots) {
    setAxisTitle(mLinkErrorPlots, "LinkError ID", "Counts");
  }
  if (mChipErrorPlots) {
    setAxisTitle(mChipErrorPlots, "ChipError ID", "Counts");
  }
}

void ITSDecodingErrorTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity : " << activity.mId << ENDM;
}

void ITSDecodingErrorTask::startOfCycle() { ILOG(Debug, Devel) << "startOfCycle" << ENDM; }

void ITSDecodingErrorTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // set timer
  //
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  // int difference;
  start = std::chrono::high_resolution_clock::now();

  auto linkErrors = ctx.inputs().get<gsl::span<o2::itsmft::GBTLinkDecodingStat>>("linkerrors");
  auto decErrors = ctx.inputs().get<gsl::span<o2::itsmft::ChipError>>("decerrors");

  // multiply Error distributions before re-filling
  for (const auto& le : linkErrors) {
    int istave = (int)(le.feeID & 0x00ff);
    int ilink = (int)((le.feeID & 0x0f00) >> 8);
    int ilayer = (int)((le.feeID & 0xf000) >> 12);
    int ifee = 3 * StaveBoundary[ilayer] - (StaveBoundary[ilayer] - StaveBoundary[NLayerIB]) * (ilayer >= NLayerIB) + istave * (3 - (ilayer >= NLayerIB)) + ilink;
    for (int ierror = 0; ierror < o2::itsmft::GBTLinkDecodingStat::NErrorsDefined; ierror++) {
      if (le.errorCounts[ierror] <= 0) {
        continue;
      }
      mLinkErrorVsFeeid->SetBinContent(ifee + 1, ierror + 1, le.errorCounts[ierror]);
    }
  }

  for (const auto& de : decErrors) {
    int istave = (int)(de.getFEEID() & 0x00ff);
    int ilink = (int)((de.getFEEID() & 0x0f00) >> 8);
    int ilayer = (int)((de.getFEEID() & 0xf000) >> 12);
    int ifee = 3 * StaveBoundary[ilayer] - (StaveBoundary[ilayer] - StaveBoundary[NLayerIB]) * (ilayer >= NLayerIB) + istave * (3 - (ilayer >= NLayerIB)) + ilink;
    int ichip = de.getChipID() - ChipBoundary[ilayer];
    for (int ierror = 0; ierror < o2::itsmft::ChipStat::NErrorsDefined; ierror++) {
      if ((de.errors >> ierror) % 2) {
        if (de.getChipID() == -1) {
          continue;
        }
        mChipErrorVsFeeid->Fill(ifee + 1, ierror + 1);
        mChipErrorVsChipid[ilayer]->Fill(ichip + 1, ierror + 1);
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
}

void ITSDecodingErrorTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void ITSDecodingErrorTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ITSDecodingErrorTask::resetGeneralPlots()
{
  mLinkErrorVsFeeid->Reset();
  mChipErrorVsFeeid->Reset();
  mLinkErrorPlots->Reset();
  mChipErrorPlots->Reset();
  for (int ilayer = 0; ilayer < 7; ilayer++) {
    mChipErrorVsChipid[ilayer]->Reset();
  }
}

void ITSDecodingErrorTask::reset()
{
  resetGeneralPlots();
  ILOG(Debug, Devel) << "Reset" << ENDM;
}

} // namespace o2::quality_control_modules::its
