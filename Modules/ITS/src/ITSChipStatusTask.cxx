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
      delete DeadChips[i];
    }
}

void ITSChipStatusTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initializing the ITSChipStatusTask" << ENDM;
  getParameters();

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

      for (int ix = 1; ix <= DeadChips[iBarrel]->GetNbinsX(); ix++)
        for (int iy = 1; iy <= DeadChips[iBarrel]->GetNbinsY(); iy++)
          DeadChips[iBarrel]->SetBinContent(ix, iy, ChipsStack[iBarrel]->stack[ix - 1][iy - 1]);
    }

  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
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
