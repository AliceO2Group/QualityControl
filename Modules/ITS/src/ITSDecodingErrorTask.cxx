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
#include "QualityControl/QcInfoLogger.h"
#include <Framework/InputRecord.h>
#include "Common/TH1Ratio.h"
#include "Common/Utils.h"
#include "DataFormatsITSMFT/CompCluster.h"
#include "DataFormatsITSMFT/ROFRecord.h"
#include "TLine.h"
#include "TLatex.h"

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
  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    delete mChipErrorVsChipid[ilayer];
  }
  if (doDeadChip) {
    for (int i = 0; i < 3; i++) {
      delete ChipsStack[i];
      delete DeadChips[i];
    }
  }
}

void ITSDecodingErrorTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initializing the ITSDecodingErrorTask" << ENDM;
  getParameters();
  createDecodingPlots();
  setPlotsFormat();

  if (doDeadChip) {
    int Layer_Draw = 0;
    for (int i = 0; i < 3; i++) {

      ChipsStack[i] = new Stack(nQCCycleToMonitor, ChipsBoundaryBarrels[i + 1] - ChipsBoundaryBarrels[i]);

      DeadChips[i] = new TH2D(Form("DeadChips%s", BarrelNames[i].Data()), Form("Dead Chips in %s", BarrelNames[i].Data()), nQCCycleToMonitor, 0, nQCCycleToMonitor, ChipsBoundaryBarrels[i + 1] - ChipsBoundaryBarrels[i], 0, ChipsBoundaryBarrels[i + 1] - ChipsBoundaryBarrels[i]);
      setAxisTitle(DeadChips[i], "QC Cycle ID", "");

      int iLayerBegin, iLayerEnd;
      if (i == 0) {
        iLayerBegin = 0;
        iLayerEnd = 2;
      } else if (i == 1) {
        iLayerBegin = 3;
        iLayerEnd = 4;
      } else {
        iLayerBegin = 5;
        iLayerEnd = 6;
      }
      const int ChipBoundary[8] = { 0, 108, 252, 432, 3120 - 432, 6480 - 432, 14712 - 6480, 24120 - 6480 }; // needed for drawing labels and lines for histogram
      const int StaveBoundary[NLayer] = { 0, 12, 28, 0, 24, 0, 42 };
      for (Int_t iLayer = iLayerBegin; iLayer <= iLayerEnd; iLayer++) {

        TLatex* msg = new TLatex(5, ChipBoundary[iLayer + 1] - 15 * (1 + i * 30), Form("#bf{L%d}", iLayer));
        msg->SetTextSize(20);
        msg->SetTextFont(43);
        DeadChips[i]->GetListOfFunctions()->Add(msg);
        if (iLayer < iLayerEnd) {
          auto l = new TLine(0, ChipBoundary[iLayer + 1] - 0.5, nQCCycleToMonitor, ChipBoundary[iLayer + 1] - 0.5);
          DeadChips[i]->GetListOfFunctions()->Add(l);
        }
      }
      int nChipsPerStave = i == 0 ? 9 : i == 1 ? 112
                                               : 196;
      for (int iy = 0; iy < DeadChips[i]->GetNbinsY(); iy++) {
        if (iy % nChipsPerStave == 0) {
          auto l = new TLine(0, iy - 0.5, nQCCycleToMonitor, iy - 0.5);
          l->SetLineColor(40);
          l->SetLineWidth(1);

          DeadChips[i]->GetListOfFunctions()->Add(l);

          if (iy >= ChipBoundary[Layer_Draw + 1]) {
            Layer_Draw++;
          }
          TLatex* msg = new TLatex(-3, iy, Form("%d", (int)(iy) / nChipsPerStave - StaveBoundary[Layer_Draw]));
          msg->SetTextSize(12);
          msg->SetTextFont(43);
          DeadChips[i]->GetListOfFunctions()->Add(msg);
        }
      }
      Layer_Draw++;

      DeadChips[i]->GetYaxis()->SetLabelSize(0);
      DeadChips[i]->GetYaxis()->SetAxisColor(0);
      getObjectsManager()->startPublishing(DeadChips[i]);
    }
  }
}

void ITSDecodingErrorTask::createDecodingPlots()
{
  mLinkErrorVsFeeid = new TH2D("General/LinkErrorVsFeeid", "GBTLink errors per FeeId", NFees, 0, NFees, o2::itsmft::GBTLinkDecodingStat::NErrorsDefined, 0.5, (float)o2::itsmft::GBTLinkDecodingStat::NErrorsDefined + 0.5);
  mLinkErrorVsFeeid->SetMinimum(0);
  mLinkErrorVsFeeid->SetStats(0);
  getObjectsManager()->startPublishing(mLinkErrorVsFeeid);
  for (int ilayer = 0; ilayer < 7; ilayer++) {
    mChipErrorVsChipid[ilayer] = new TH2D(Form("General/Layer%dChipErrorVsChipid", ilayer), Form("Layer%d Chip errors per FeeId", ilayer), ChipBoundary[ilayer + 1] - ChipBoundary[ilayer], 0, ChipBoundary[ilayer + 1] - ChipBoundary[ilayer], o2::itsmft::ChipStat::NErrorsDefined, 0.5, (float)o2::itsmft::ChipStat::NErrorsDefined + 0.5);
    mChipErrorVsChipid[ilayer]->SetMinimum(0);
    mChipErrorVsChipid[ilayer]->SetStats(0);
    getObjectsManager()->startPublishing(mChipErrorVsChipid[ilayer]);
  }
  mChipErrorVsFeeid = new TH2D("General/ChipErrorVsFeeid", "Chip decoding errors per FeeId", NFees, 0, NFees, o2::itsmft::ChipStat::NErrorsDefined, 0.5, (float)o2::itsmft::ChipStat::NErrorsDefined + 0.5);
  mChipErrorVsFeeid->SetMinimum(0);
  mChipErrorVsFeeid->SetStats(0);
  getObjectsManager()->startPublishing(mChipErrorVsFeeid);
  mLinkErrorPlots = new TH1D("General/LinkErrorPlots", "GBTLink decoding Errors", o2::itsmft::GBTLinkDecodingStat::NErrorsDefined, 0.5, (float)o2::itsmft::GBTLinkDecodingStat::NErrorsDefined + 0.5);
  mLinkErrorPlots->SetMinimum(0);
  mLinkErrorPlots->SetStats(0);
  mLinkErrorPlots->SetFillColor(kOrange);
  getObjectsManager()->startPublishing(mLinkErrorPlots); // mLinkErrorPlots
  mChipErrorPlots = new TH1D("General/ChipErrorPlots", "Chip Decoding Errors", o2::itsmft::ChipStat::NErrorsDefined, 0.5, (float)o2::itsmft::ChipStat::NErrorsDefined + 0.5);
  mChipErrorPlots->SetMinimum(0);
  mChipErrorPlots->SetStats(0);
  mChipErrorPlots->SetFillColor(kOrange);
  getObjectsManager()->startPublishing(mChipErrorPlots); // mChipErrorPlots

  hAlwaysBusy = new TH1D("AlwaysBusyChips", "Number of Chips always in BUSY state", 11, 0, 11);
  setAxisTitle(hAlwaysBusy, "Layer", "Counts");
  getObjectsManager()->startPublishing(hAlwaysBusy);

  hBusyFraction = std::make_unique<TH1FRatio>("FractionOfBusyChips", "Fraction of chips in BUSY, excluding permanent", 11, 0, 11, false);
  setAxisTitle(hBusyFraction.get(), "Layer", "BusyViolations / TF / NChips");
  getObjectsManager()->startPublishing(hBusyFraction.get());

  for (int iBin = 1; iBin <= 11; iBin++) {
    hAlwaysBusy->GetXaxis()->SetBinLabel(iBin, LayerBinLabels[iBin - 1]);
    hBusyFraction->GetXaxis()->SetBinLabel(iBin, LayerBinLabels[iBin - 1]);
  }
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

void ITSDecodingErrorTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;

  if (doDeadChip) {
    for (int i = 0; i < 3; i++) {
      CurrentDeadChips[i].clear();
      CurrentDeadChips[i].resize(ChipsBoundaryBarrels[i + 1] - ChipsBoundaryBarrels[i], 0);
    }
  }
}

void ITSDecodingErrorTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto linkErrors = ctx.inputs().get<gsl::span<o2::itsmft::GBTLinkDecodingStat>>("linkerrors");
  auto decErrors = ctx.inputs().get<gsl::span<o2::itsmft::ChipError>>("decerrors");
  auto aliveChips = ctx.inputs().get<gsl::span<char>>("chipstatus");

  if (doDeadChip) {
    int id = 0;
    for (auto chipID : aliveChips) {
      int iBarrel = id < ChipsBoundaryBarrels[1] ? 0 : id < ChipsBoundaryBarrels[2] ? 1
                                                                                    : 2;
      if ((int)chipID != 1) {
        int ChipID_fill = id - ChipsBoundaryBarrels[iBarrel];
        if ((ChipID_fill < 0) || (ChipID_fill >= CurrentDeadChips[iBarrel].size())) {
          ILOG(Warning, Devel) << " prolematic ID for chip: " << ChipID_fill << ENDM;
        } else {
          CurrentDeadChips[iBarrel][ChipID_fill]++;
        }
      }
      id++;
    }
  }
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
        mChipErrorVsFeeid->Fill(ifee, ierror + 1);
        mChipErrorVsChipid[ilayer]->Fill(ichip, ierror + 1);
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
  mTFCount++;
}

void ITSDecodingErrorTask::getParameters()
{
  mBusyViolationLimit = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "mBusyViolationLimit", mBusyViolationLimit);

  nQCCycleToMonitor = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "nQCCycleToMonitor", nQCCycleToMonitor);
  doDeadChip = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "doDeadChip", doDeadChip);
}

void ITSDecodingErrorTask::endOfCycle()
{
  if (doDeadChip) {
    for (int iBarrel = 0; iBarrel < 3; iBarrel++) {
      ChipsStack[iBarrel]->push(CurrentDeadChips[iBarrel]);

      for (int ix = 1; ix <= DeadChips[iBarrel]->GetNbinsX(); ix++)
        for (int iy = 1; iy <= DeadChips[iBarrel]->GetNbinsY(); iy++)
          DeadChips[iBarrel]->SetBinContent(ix, iy, ChipsStack[iBarrel]->stack[ix - 1][iy - 1]);
    }
  }

  nQCCycle++;
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  hBusyFraction->Reset();
  hAlwaysBusy->Reset();

  int binIterator = 0;

  for (int iLayer = 0; iLayer < NLayer; iLayer++) {
    for (int iChip = 1; iChip <= nChipsPerLayer[iLayer]; iChip++) {

      if (iLayer > 2 && iChip == nChipsPerLayer[iLayer] / 2)
        binIterator++; // to account for bot/top barrel

      int nBusyViolations = mChipErrorVsChipid[iLayer]->GetBinContent(iChip, 1);
      if (1. * nBusyViolations / mTFCount > mBusyViolationLimit) {
        hAlwaysBusy->Fill(binIterator);
        continue;
      }
      if (nBusyViolations > 0) {
        hBusyFraction->getNum()->Fill(binIterator, nBusyViolations);
      }
    }
    hBusyFraction->getDen()->SetBinContent(binIterator + 1, mTFCount * nChipsPerLayer[iLayer]);
    binIterator++;
  }
  hBusyFraction->update();
}

void ITSDecodingErrorTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
  nQCCycle = 0;
}

void ITSDecodingErrorTask::resetGeneralPlots()
{
  mTFCount++;
  mLinkErrorVsFeeid->Reset();
  mChipErrorVsFeeid->Reset();
  mLinkErrorPlots->Reset();
  mChipErrorPlots->Reset();
  for (int ilayer = 0; ilayer < 7; ilayer++) {
    mChipErrorVsChipid[ilayer]->Reset();
  }
  mTFCount = 0;
  hBusyFraction->Reset();
  hAlwaysBusy->Reset();
}

void ITSDecodingErrorTask::reset()
{
  resetGeneralPlots();
  ILOG(Debug, Devel) << "Reset" << ENDM;
}

} // namespace o2::quality_control_modules::its
