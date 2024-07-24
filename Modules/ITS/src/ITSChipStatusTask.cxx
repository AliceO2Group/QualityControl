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

  StaveOverview = new TH2Poly();
  StaveOverview->SetName("StaveStatusOverview");
  TString title = "Number of QC cycles when stave is without data";
  title += ";mm (IB 3x);mm (IB 3x)";
  StaveOverview->SetTitle(title);
  StaveOverview->SetStats(0);
  StaveOverview->SetOption("lcolz");
  StaveOverview->SetMinimum(0);
  StaveOverview->SetMaximum(1);
  for (int ilayer = 0; ilayer < 7; ilayer++) {
    for (int istave = 0; istave < NStaves[ilayer]; istave++) {
      double* px = new double[4];
      double* py = new double[4];
      getStavePoint(ilayer, istave, px, py);
      if (ilayer < 3) {
        for (int icoo = 0; icoo < 4; icoo++) {
          px[icoo] *= 3.;
          py[icoo] *= 3.;
        }
      }
      StaveOverview->AddBin(4, px, py);
    }
  }
  getObjectsManager()->startPublishing(StaveOverview);
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

void ITSChipStatusTask::getStavePoint(int layer, int stave, double* px, double* py)
{
  float stepAngle = TMath::Pi() * 2 / NStaves[layer];             // the angle between to stave
  float midAngle = StartAngle[layer] + (stave * stepAngle);       // mid point angle
  float staveRotateAngle = TMath::Pi() / 2 - (stave * stepAngle); // how many angle this stave rotate(compare with first stave)
  px[1] = MidPointRad[layer] * TMath::Cos(midAngle);              // there are 4 point to decide this TH2Poly bin
                                                                  // 0:left point in this stave;
                                                                  // 1:mid point in this stave;
                                                                  // 2:right point in this stave;
                                                                  // 3:higher point int this stave;
  py[1] = MidPointRad[layer] * TMath::Sin(midAngle);              // 4 point calculated accord the blueprint
                                                                  // roughly calculate
  if (layer < NLayerIB) {
    px[0] = 7.7 * TMath::Cos(staveRotateAngle) + px[1];
    py[0] = -7.7 * TMath::Sin(staveRotateAngle) + py[1];
    px[2] = -7.7 * TMath::Cos(staveRotateAngle) + px[1];
    py[2] = 7.7 * TMath::Sin(staveRotateAngle) + py[1];
    px[3] = 5.623 * TMath::Sin(staveRotateAngle) + px[1];
    py[3] = 5.623 * TMath::Cos(staveRotateAngle) + py[1];
  } else {
    px[0] = 21 * TMath::Cos(staveRotateAngle) + px[1];
    py[0] = -21 * TMath::Sin(staveRotateAngle) + py[1];
    px[2] = -21 * TMath::Cos(staveRotateAngle) + px[1];
    py[2] = 21 * TMath::Sin(staveRotateAngle) + py[1];
    px[3] = 40 * TMath::Sin(staveRotateAngle) + px[1];
    py[3] = 40 * TMath::Cos(staveRotateAngle) + py[1];
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
  NCycleForOverview = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "nQCCycleToOverview", NCycleForOverview);
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
  //---------- Shifter plot

  int iLayer = 0;
  int iStave = 0;
  StaveOverview->Reset("content");
  for (int iBarrel = 0; iBarrel < 3; iBarrel++) {

    for (int iSlice = 0; iSlice < NCycleForOverview; iSlice++) {
      iLayer = iBarrel == 0 ? 0 : iBarrel == 1 ? 3
                                               : 5;
      TH1D* hBarrelProj = DeadChips[iBarrel]->getNum()->ProjectionY("dummy", nQCCycleToMonitor - iSlice, nQCCycleToMonitor - iSlice);
      iStave = 0;
      int nEmptyChips = 0;
      for (int iChip = 1; iChip <= hBarrelProj->GetNbinsX(); iChip++) {
        if (hBarrelProj->GetBinContent(iChip) > 0)
          nEmptyChips++;
        if (((iChip) % (nChipsPerHic[iLayer] * nHicPerStave[iLayer]) == 0)) {
          if (nEmptyChips == nChipsPerHic[iLayer] * nHicPerStave[iLayer]) {
            int binContent = StaveOverview->GetBinContent(iStave + StaveBoundary[iLayer]) * NCycleForOverview + 1;
            StaveOverview->SetBinContent(iStave + StaveBoundary[iLayer], 1. * binContent / NCycleForOverview);
          }
          nEmptyChips = 0;
          iStave++;
          if ((iChip) == ChipsBoundaryLayers[iLayer + 1]) {
            iLayer++;
            iStave = 0;
          }
        }
      }
    }
  }
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
