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
/// \file   ITSChipStatusTask.cxx
/// \author My Name
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "ITS/ITSChipStatusTask.h"
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>
#include "TLine.h"
#include "TLatex.h"

namespace o2::quality_control_modules::its
{
ITSChipStatusTask::ITSChipStatusTask()
  : TaskInterface()
{
}

ITSChipStatusTask::~ITSChipStatusTask()
{
  for (int i = 0; i < 3; i++) {
    delete ChipsStack[i];
  }
}

void ITSChipStatusTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initializing the ITSChipStatusTask" << ENDM;
  getParameters();

  for (int i = 0; i < 3; i++) {

    ChipsStack[i] = new Stack(nQCCycleToMonitor, ChipsBoundaryBarrels[i + 1] - ChipsBoundaryBarrels[i]);
    TFsStack[i] = new Stack(nQCCycleToMonitor, ChipsBoundaryBarrels[i + 1] - ChipsBoundaryBarrels[i]);

    DeadChips[i] = std::make_shared<TH2FRatio>(Form("DeadChips%s", BarrelNames[i].Data()), Form("Time fraction without data from %s chips", BarrelNames[i].Data()), nQCCycleToMonitor, 0, nQCCycleToMonitor, ChipsBoundaryBarrels[i + 1] - ChipsBoundaryBarrels[i], 0, ChipsBoundaryBarrels[i + 1] - ChipsBoundaryBarrels[i], false);
    setAxisTitle(DeadChips[i].get(), Form("Last %d QC Cycles", nQCCycleToMonitor), "Chip ID");

    getObjectsManager()->startPublishing(DeadChips[i].get());
  }
}

void ITSChipStatusTask::setAxisTitle(TH1* object, const char* xTitle, const char* yTitle)
{
  object->GetXaxis()->SetTitle(xTitle);
  object->GetYaxis()->SetTitle(yTitle);
}

void ITSChipStatusTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity : " << activity.mId << ENDM;
}

void ITSChipStatusTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;

  for (int i = 0; i < 3; i++) {
    CurrentDeadChips[i].clear();
    CurrentDeadChips[i].resize(ChipsBoundaryBarrels[i + 1] - ChipsBoundaryBarrels[i], 0);
    CurrentTFs[i].clear();
    CurrentTFs[i].resize(ChipsBoundaryBarrels[i + 1] - ChipsBoundaryBarrels[i], 0);
  }
}

void ITSChipStatusTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto aliveChips = ctx.inputs().get<gsl::span<char>>("chipstatus");

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
      CurrentTFs[iBarrel][ChipID_fill]++;
    }
    id++;
  }
}

void ITSChipStatusTask::getParameters()
{

  nQCCycleToMonitor = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "nQCCycleToMonitor", nQCCycleToMonitor);
}

void ITSChipStatusTask::endOfCycle()
{

  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  for (int iBarrel = 0; iBarrel < 3; iBarrel++) {
    ChipsStack[iBarrel]->push(CurrentDeadChips[iBarrel]);
    TFsStack[iBarrel]->push(CurrentTFs[iBarrel]);
    for (int ix = 1; ix <= DeadChips[iBarrel]->GetNbinsX(); ix++)
      for (int iy = 1; iy <= DeadChips[iBarrel]->GetNbinsY(); iy++) {
        DeadChips[iBarrel]->getNum()->SetBinContent(ix, iy, ChipsStack[iBarrel]->stack[ix - 1][iy - 1]);
        DeadChips[iBarrel]->getDen()->SetBinContent(ix, iy, TFsStack[iBarrel]->stack[ix - 1][iy - 1]);
      }

    DeadChips[iBarrel]->update();
  }

  Beautify();
}

void ITSChipStatusTask::Beautify()
{

  int Layer_Draw = 0;
  for (int i = 0; i < 3; i++) {

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

    for (Int_t iLayer = iLayerBegin; iLayer <= iLayerEnd; iLayer++) {

      TLatex* msg = new TLatex(5, ChipsBoundaryLayers[iLayer + 1] - 15 * (1 + i * 30), Form("#bf{L%d}", iLayer));
      msg->SetTextSize(20);
      msg->SetTextFont(43);
      DeadChips[i]->GetListOfFunctions()->Add(msg);
      if (iLayer < iLayerEnd) {
        auto l = new TLine(0, ChipsBoundaryLayers[iLayer + 1] - 0.5, nQCCycleToMonitor, ChipsBoundaryLayers[iLayer + 1] - 0.5);
        DeadChips[i]->GetListOfFunctions()->Add(l);
      }
    }
  }
}

void ITSChipStatusTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ITSChipStatusTask::reset()
{
  ILOG(Debug, Devel) << "Reset" << ENDM;
}

} // namespace o2::quality_control_modules::its
