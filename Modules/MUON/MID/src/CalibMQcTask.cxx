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
/// \author Valerie Ramillien

#include <TStyle.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TFile.h>
#include <TProfile2D.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "QualityControl/QcInfoLogger.h"
#include "MID/CalibMQcTask.h"
#include <Framework/InputRecord.h>
#include "Framework/DataRefUtils.h"
#include "DataFormatsMID/ColumnData.h"
#include "DataFormatsMID/ROBoard.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDRaw/ElectronicsDelay.h"
#include "MIDRaw/FEEIdConfig.h"
#include "MIDBase/DetectorParameters.h"
#include "MIDBase/GeometryParameters.h"
#include "MIDWorkflow/ColumnDataSpecsUtils.h"

#define MID_NDE 72
#define MID_NCOL 7

namespace o2::quality_control_modules::mid
{

CalibMQcTask::~CalibMQcTask()
{
}

void CalibMQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Devel) << "initialize CalibMQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  // std::cout << "!!!! START initialize CalibQcTask Test !!!! " << std::endl;
  /////////////////

  gStyle->SetPalette(kRainBow);

  //////////////////////////////////
  /// Noise strips Histograms :: ///
  //////////////////////////////////
  mMMultNoiseMT11B = std::make_shared<TH1F>("MMultNoiseMT11B", "Multiplicity Noise strips - MT11 bending plane", 300, 0, 300);
  mMMultNoiseMT11NB = std::make_shared<TH1F>("MMultNoiseMT11NB", "Multiplicity Noise strips - MT11 non-bending plane", 300, 0, 300);
  mMMultNoiseMT12B = std::make_shared<TH1F>("MMultNoiseMT12B", "Multiplicity Noise strips - MT12 bending plane", 300, 0, 300);
  mMMultNoiseMT12NB = std::make_shared<TH1F>("MMultNoiseMT12NB", "Multiplicity Noise strips - MT12 non-bending plane", 300, 0, 300);
  mMMultNoiseMT21B = std::make_shared<TH1F>("MMultNoiseMT21B", "Multiplicity Noise strips - MT21 bending plane", 300, 0, 300);
  mMMultNoiseMT21NB = std::make_shared<TH1F>("MMultNoiseMT21NB", "Multiplicity Noise strips - MT21 non-bending plane", 300, 0, 300);
  mMMultNoiseMT22B = std::make_shared<TH1F>("MMultNoiseMT22B", "Multiplicity Noise strips - MT22 bending plane", 300, 0, 300);
  mMMultNoiseMT22NB = std::make_shared<TH1F>("MMultNoiseMT22NB", "Multiplicity Noise strips - MT22 non-bending plane", 300, 0, 300);
  getObjectsManager()->startPublishing(mMMultNoiseMT11B.get());
  getObjectsManager()->startPublishing(mMMultNoiseMT11NB.get());
  getObjectsManager()->startPublishing(mMMultNoiseMT12B.get());
  getObjectsManager()->startPublishing(mMMultNoiseMT12NB.get());
  getObjectsManager()->startPublishing(mMMultNoiseMT21B.get());
  getObjectsManager()->startPublishing(mMMultNoiseMT21NB.get());
  getObjectsManager()->startPublishing(mMMultNoiseMT22B.get());
  getObjectsManager()->startPublishing(mMMultNoiseMT22NB.get());

  mMBendNoiseMap11 = std::make_shared<TH2I>("MBendNoiseMap11", "Bending Noise Map MT11", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mMBendNoiseMap11.get());
  mMBendNoiseMap11->GetXaxis()->SetTitle("Column");
  mMBendNoiseMap11->GetYaxis()->SetTitle("Line");
  mMBendNoiseMap11->SetOption("colz");
  mMBendNoiseMap11->SetStats(0);

  mMBendNoiseMap12 = std::make_shared<TH2I>("MBendNoiseMap12", "Bending Noise Map MT12", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mMBendNoiseMap12.get());
  mMBendNoiseMap12->GetXaxis()->SetTitle("Column");
  mMBendNoiseMap12->GetYaxis()->SetTitle("Line");
  mMBendNoiseMap12->SetOption("colz");
  mMBendNoiseMap12->SetStats(0);

  mMBendNoiseMap21 = std::make_shared<TH2I>("MBendNoiseMap21", "Bending Noise Map MT21", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mMBendNoiseMap21.get());
  mMBendNoiseMap21->GetXaxis()->SetTitle("Column");
  mMBendNoiseMap21->GetYaxis()->SetTitle("Line");
  mMBendNoiseMap21->SetOption("colz");
  mMBendNoiseMap21->SetStats(0);

  mMBendNoiseMap22 = std::make_shared<TH2I>("MBendNoiseMap22", "Bending Noise Map MT22", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mMBendNoiseMap22.get());
  mMBendNoiseMap22->GetXaxis()->SetTitle("Column");
  mMBendNoiseMap22->GetYaxis()->SetTitle("Line");
  mMBendNoiseMap22->SetOption("colz");
  mMBendNoiseMap22->SetStats(0);

  mMNBendNoiseMap11 = std::make_shared<TH2I>("MNBendNoiseMap11", "Non-Bending Noise Map MT11", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mMNBendNoiseMap11.get());
  mMNBendNoiseMap11->GetXaxis()->SetTitle("Column");
  mMNBendNoiseMap11->GetYaxis()->SetTitle("Line");
  mMNBendNoiseMap11->SetOption("colz");
  mMNBendNoiseMap11->SetStats(0);

  mMNBendNoiseMap12 = std::make_shared<TH2I>("MNBendNoiseMap12", "Non-Bending Noise Map MT12", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mMNBendNoiseMap12.get());
  mMNBendNoiseMap12->GetXaxis()->SetTitle("Column");
  mMNBendNoiseMap12->GetYaxis()->SetTitle("Line");
  mMNBendNoiseMap12->SetOption("colz");
  mMNBendNoiseMap12->SetStats(0);

  mMNBendNoiseMap21 = std::make_shared<TH2I>("MNBendNoiseMap21", "Non-Bending Noise Map MT21", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mMNBendNoiseMap21.get());
  mMNBendNoiseMap21->GetXaxis()->SetTitle("Column");
  mMNBendNoiseMap21->GetYaxis()->SetTitle("Line");
  mMNBendNoiseMap21->SetOption("colz");
  mMNBendNoiseMap21->SetStats(0);

  mMNBendNoiseMap22 = std::make_shared<TH2I>("MNBendNoiseMap22", "Non-Bending Noise Map MT22", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mMNBendNoiseMap22.get());
  mMNBendNoiseMap22->GetXaxis()->SetTitle("Column");
  mMNBendNoiseMap22->GetYaxis()->SetTitle("Line");
  mMNBendNoiseMap22->SetOption("colz");
  mMNBendNoiseMap22->SetStats(0);

  /////////////////////////////////
  /// Dead strips Histograms :: ///
  /////////////////////////////////
  mMMultDeadMT11B = std::make_shared<TH1F>("MMultDeadMT11B", "Multiplicity Dead strips - MT11 bending plane", 300, 0, 300);
  mMMultDeadMT11NB = std::make_shared<TH1F>("MMultDeadMT11NB", "Multiplicity Dead strips - MT11 non-bending plane", 300, 0, 300);
  mMMultDeadMT12B = std::make_shared<TH1F>("MMultDeadMT12B", "Multiplicity Dead strips - MT12 bending plane", 300, 0, 300);
  mMMultDeadMT12NB = std::make_shared<TH1F>("MMultDeadMT12NB", "MultiplicityDead  strips - MT12 non-bending plane", 300, 0, 300);
  mMMultDeadMT21B = std::make_shared<TH1F>("MMultDeadMT21B", "Multiplicity Dead strips - MT21 bending plane", 300, 0, 300);
  mMMultDeadMT21NB = std::make_shared<TH1F>("MMultDeadMT21NB", "Multiplicity Dead strips - MT21 non-bending plane", 300, 0, 300);
  mMMultDeadMT22B = std::make_shared<TH1F>("MMultDeadMT22B", "Multiplicity Dead strips - MT22 bending plane", 300, 0, 300);
  mMMultDeadMT22NB = std::make_shared<TH1F>("MMultDeadMT22NB", "Multiplicity Dead strips - MT22 non-bending plane", 300, 0, 300);
  getObjectsManager()->startPublishing(mMMultDeadMT11B.get());
  getObjectsManager()->startPublishing(mMMultDeadMT11NB.get());
  getObjectsManager()->startPublishing(mMMultDeadMT12B.get());
  getObjectsManager()->startPublishing(mMMultDeadMT12NB.get());
  getObjectsManager()->startPublishing(mMMultDeadMT21B.get());
  getObjectsManager()->startPublishing(mMMultDeadMT21NB.get());
  getObjectsManager()->startPublishing(mMMultDeadMT22B.get());
  getObjectsManager()->startPublishing(mMMultDeadMT22NB.get());

  mMBendDeadMap11 = std::make_shared<TH2I>("MBendDeadMap11", "Bending Dead Map MT11", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mMBendDeadMap11.get());
  mMBendDeadMap11->GetXaxis()->SetTitle("Column");
  mMBendDeadMap11->GetYaxis()->SetTitle("Line");
  mMBendDeadMap11->SetOption("colz");
  mMBendDeadMap11->SetStats(0);

  mMBendDeadMap12 = std::make_shared<TH2I>("MBendDeadMap12", "Bending Dead Map MT12", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mMBendDeadMap12.get());
  mMBendDeadMap12->GetXaxis()->SetTitle("Column");
  mMBendDeadMap12->GetYaxis()->SetTitle("Line");
  mMBendDeadMap12->SetOption("colz");
  mMBendDeadMap12->SetStats(0);

  mMBendDeadMap21 = std::make_shared<TH2I>("MBendDeadMap21", "Bending Dead Map MT21", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mMBendDeadMap21.get());
  mMBendDeadMap21->GetXaxis()->SetTitle("Column");
  mMBendDeadMap21->GetYaxis()->SetTitle("Line");
  mMBendDeadMap21->SetOption("colz");
  mMBendDeadMap21->SetStats(0);

  mMBendDeadMap22 = std::make_shared<TH2I>("MBendDeadMap22", "Bending Dead Map MT22", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mMBendDeadMap22.get());
  mMBendDeadMap22->GetXaxis()->SetTitle("Column");
  mMBendDeadMap22->GetYaxis()->SetTitle("Line");
  mMBendDeadMap22->SetOption("colz");
  mMBendDeadMap22->SetStats(0);

  mMNBendDeadMap11 = std::make_shared<TH2I>("MNBendDeadMap11", "Non-Bending Dead Map MT11", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mMNBendDeadMap11.get());
  mMNBendDeadMap11->GetXaxis()->SetTitle("Column");
  mMNBendDeadMap11->GetYaxis()->SetTitle("Line");
  mMNBendDeadMap11->SetOption("colz");
  mMNBendDeadMap11->SetStats(0);

  mMNBendDeadMap12 = std::make_shared<TH2I>("MNBendDeadMap12", "Non-Bending Dead Map MT12", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mMNBendDeadMap12.get());
  mMNBendDeadMap12->GetXaxis()->SetTitle("Column");
  mMNBendDeadMap12->GetYaxis()->SetTitle("Line");
  mMNBendDeadMap12->SetOption("colz");
  mMNBendDeadMap12->SetStats(0);

  mMNBendDeadMap21 = std::make_shared<TH2I>("MNBendDeadMap21", "Non-Bending Dead Map MT21", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mMNBendDeadMap21.get());
  mMNBendDeadMap21->GetXaxis()->SetTitle("Column");
  mMNBendDeadMap21->GetYaxis()->SetTitle("Line");
  mMNBendDeadMap21->SetOption("colz");
  mMNBendDeadMap21->SetStats(0);

  mMNBendDeadMap22 = std::make_shared<TH2I>("MNBendDeadMap22", "Non-Bending Dead Map MT22", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mMNBendDeadMap22.get());
  mMNBendDeadMap22->GetXaxis()->SetTitle("Column");
  mMNBendDeadMap22->GetYaxis()->SetTitle("Line");
  mMNBendDeadMap22->SetOption("colz");
  mMNBendDeadMap22->SetStats(0);

  /////////////////////////////////
  /// Bad strips Histograms ::  ///
  /////////////////////////////////

  mMMultBadMT11B = std::make_shared<TH1F>("MMultBadMT11B", "Multiplicity Bad strips - MT11 bending plane", 300, 0, 300);
  mMMultBadMT11NB = std::make_shared<TH1F>("MMultBadMT11NB", "Multiplicity Bad strips - MT11 non-bending plane", 300, 0, 300);
  mMMultBadMT12B = std::make_shared<TH1F>("MMultBadMT12B", "Multiplicity Bad strips - MT12 bending plane", 300, 0, 300);
  mMMultBadMT12NB = std::make_shared<TH1F>("MMultBadMT12NB", "MultiplicityBad  strips - MT12 non-bending plane", 300, 0, 300);
  mMMultBadMT21B = std::make_shared<TH1F>("MMultBadMT21B", "Multiplicity Bad strips - MT21 bending plane", 300, 0, 300);
  mMMultBadMT21NB = std::make_shared<TH1F>("MMultBadMT21NB", "Multiplicity Bad strips - MT21 non-bending plane", 300, 0, 300);
  mMMultBadMT22B = std::make_shared<TH1F>("MMultBadMT22B", "Multiplicity Bad strips - MT22 bending plane", 300, 0, 300);
  mMMultBadMT22NB = std::make_shared<TH1F>("MMultBadMT22NB", "Multiplicity Bad strips - MT22 non-bending plane", 300, 0, 300);
  getObjectsManager()->startPublishing(mMMultBadMT11B.get());
  getObjectsManager()->startPublishing(mMMultBadMT11NB.get());
  getObjectsManager()->startPublishing(mMMultBadMT12B.get());
  getObjectsManager()->startPublishing(mMMultBadMT12NB.get());
  getObjectsManager()->startPublishing(mMMultBadMT21B.get());
  getObjectsManager()->startPublishing(mMMultBadMT21NB.get());
  getObjectsManager()->startPublishing(mMMultBadMT22B.get());
  getObjectsManager()->startPublishing(mMMultBadMT22NB.get());

  mMBendBadMap11 = std::make_shared<TH2I>("MBendBadMap11", "Bending Bad Map MT11", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mMBendBadMap11.get());
  mMBendBadMap11->GetXaxis()->SetTitle("Column");
  mMBendBadMap11->GetYaxis()->SetTitle("Line");
  mMBendBadMap11->SetOption("colz");
  mMBendBadMap11->SetStats(0);

  mMBendBadMap12 = std::make_shared<TH2I>("MBendBadMap12", "Bending Bad Map MT12", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mMBendBadMap12.get());
  mMBendBadMap12->GetXaxis()->SetTitle("Column");
  mMBendBadMap12->GetYaxis()->SetTitle("Line");
  mMBendBadMap12->SetOption("colz");
  mMBendBadMap12->SetStats(0);

  mMBendBadMap21 = std::make_shared<TH2I>("MBendBadMap21", "Bending Bad Map MT21", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mMBendBadMap21.get());
  mMBendBadMap21->GetXaxis()->SetTitle("Column");
  mMBendBadMap21->GetYaxis()->SetTitle("Line");
  mMBendBadMap21->SetOption("colz");
  mMBendBadMap21->SetStats(0);

  mMBendBadMap22 = std::make_shared<TH2I>("MBendBadMap22", "Bending Bad Map MT22", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mMBendBadMap22.get());
  mMBendBadMap22->GetXaxis()->SetTitle("Column");
  mMBendBadMap22->GetYaxis()->SetTitle("Line");
  mMBendBadMap22->SetOption("colz");
  mMBendBadMap22->SetStats(0);

  mMNBendBadMap11 = std::make_shared<TH2I>("MNBendBadMap11", "Non-Bending Bad Map MT11", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mMNBendBadMap11.get());
  mMNBendBadMap11->GetXaxis()->SetTitle("Column");
  mMNBendBadMap11->GetYaxis()->SetTitle("Line");
  mMNBendBadMap11->SetOption("colz");
  mMNBendBadMap11->SetStats(0);

  mMNBendBadMap12 = std::make_shared<TH2I>("MNBendBadMap12", "Non-Bending Bad Map MT12", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mMNBendBadMap12.get());
  mMNBendBadMap12->GetXaxis()->SetTitle("Column");
  mMNBendBadMap12->GetYaxis()->SetTitle("Line");
  mMNBendBadMap12->SetOption("colz");
  mMNBendBadMap12->SetStats(0);

  mMNBendBadMap21 = std::make_shared<TH2I>("MNBendBadMap21", "Non-Bending Bad Map MT21", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mMNBendBadMap21.get());
  mMNBendBadMap21->GetXaxis()->SetTitle("Column");
  mMNBendBadMap21->GetYaxis()->SetTitle("Line");
  mMNBendBadMap21->SetOption("colz");
  mMNBendBadMap21->SetStats(0);

  mMNBendBadMap22 = std::make_shared<TH2I>("MNBendBadMap22", "Non-Bending Bad Map MT22", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mMNBendBadMap22.get());
  mMNBendBadMap22->GetXaxis()->SetTitle("Column");
  mMNBendBadMap22->GetYaxis()->SetTitle("Line");
  mMNBendBadMap22->SetOption("colz");
  mMNBendBadMap22->SetStats(0);
}

void CalibMQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Devel) << "startOfActivity" << ENDM;
  // std::cout << "!!!! START startOfActivity a CalibMQcTask TEST   !!!! " << std::endl;
}

void CalibMQcTask::startOfCycle()
{
  ILOG(Info, Devel) << "startOfCycle" << ENDM;
  // std::cout << "!!!! START startOfCycle CalibMQcTask TEST   !!!! " << std::endl;
}

void CalibMQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // std::cout << "!!!! START monitorData CalibMQcTask TEST  !!!! " << std::endl;
  mTF++;
  // printf("----------------> %05d TF \n", nTF);

  int multNoiseMT11B = 0;
  int multNoiseMT12B = 0;
  int multNoiseMT21B = 0;
  int multNoiseMT22B = 0;
  int multNoiseMT11NB = 0;
  int multNoiseMT12NB = 0;
  int multNoiseMT21NB = 0;
  int multNoiseMT22NB = 0;

  int multDeadMT11B = 0;
  int multDeadMT12B = 0;
  int multDeadMT21B = 0;
  int multDeadMT22B = 0;
  int multDeadMT11NB = 0;
  int multDeadMT12NB = 0;
  int multDeadMT21NB = 0;
  int multDeadMT22NB = 0;

  int multBadMT11B = 0;
  int multBadMT12B = 0;
  int multBadMT21B = 0;
  int multBadMT22B = 0;
  int multBadMT11NB = 0;
  int multBadMT12NB = 0;
  int multBadMT21NB = 0;
  int multBadMT22NB = 0;

  auto noises = ctx.inputs().get<gsl::span<o2::mid::ColumnData>>("noisych");

  //  printf("noises ========================================================== \n");

  mNoiseROF++;
  multNoiseMT11B = 0;
  multNoiseMT12B = 0;
  multNoiseMT21B = 0;
  multNoiseMT22B = 0;
  multNoiseMT11NB = 0;
  multNoiseMT12NB = 0;
  multNoiseMT21NB = 0;
  multNoiseMT22NB = 0;

  for (auto& noise : noises) { // loop DE //

    // std::cout << "!!!! START monitorData CalibMQcTaskLoop noises (DE)!!!! " << std::endl;

    int deIndex = noise.deId;
    int colId = noise.columnId;
    int rpcLine = o2::mid::detparams::getRPCLine(deIndex);
    int ichamber = o2::mid::detparams::getChamber(deIndex);
    auto isRightSide = o2::mid::detparams::isRightSide(deIndex);
    auto detId = o2::mid::detparams::getDEId(isRightSide, ichamber, colId);

    // std::cout << " deId  " << deIndex << std::endl;

    int nZoneHistoX = 1;
    int nZoneHistoY = 4;
    double stripYSize = 1. / 16;
    double stripXSize = 0.25 / 16;
    if (mMapping.getLastBoardBP(colId, deIndex) == 1) {
      nZoneHistoX = 2;
      stripXSize = 0.5 / 16;
    } else if (mMapping.getLastBoardBP(colId, deIndex) == 0) {
      nZoneHistoX = 4;
      stripXSize = 1. / 16;
    }

    for (int board = mMapping.getFirstBoardBP(colId, deIndex), lastBoard = mMapping.getLastBoardBP(colId, deIndex); board <= lastBoard + 1; board++) {
      // These are the existing bend boards for this column Id (board = n°board in the column) + 1 (for non-bend)
      if ((lastBoard < 4) && (board == lastBoard + 1))
        board = 4;                      // for Non-Bend digits
      if (noise.patterns[board] != 0) { //  Bend + Non-Bend plane
        //// Bend Local Boards Display ::
        double linePos0 = rpcLine;

        //// Strips Display ::
        int mask = 1;

        double shift = 0; // for central zone with only 3 loc boards
        if ((colId == 0) && ((rpcLine == 3) || (rpcLine == 5)))
          nZoneHistoY = 3;
        if ((colId == 0) && (rpcLine == 5))
          shift = 0.25;

        for (int j = 0; j < 16; j++) {
          int fired = 0;
          if ((noise.patterns[board] & mask) != 0)
            fired = 1;
          double lineHitPos = linePos0 + (j * stripXSize);
          int colj = j;
          if (mMapping.getNStripsNBP(colId, deIndex) == 8)
            colj = j * 2; // Bend with only 8 stripY
          double colHitPos = colId + (colj * stripYSize);
          // std::cout << " bit (j) =>>  " << j << "  stripXPos = "<< stripXPos <<"  lineHitPos = "<< lineHitPos << std::endl;
          if (isRightSide) {
            if (ichamber == 0) {
              if (board == 4) { // Non-Bend
                multNoiseMT11NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendNoiseMap11->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), fired);
              } else { // Bend
                multNoiseMT11B++;
                mMBendNoiseMap11->Fill(colId + 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 1) {
              if (board == 4) {
                multNoiseMT12NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendNoiseMap12->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), fired);
              } else { // Bend
                multNoiseMT12B++;
                mMBendNoiseMap12->Fill(colId + 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 2) {
              if (board == 4) {
                multNoiseMT21NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendNoiseMap21->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), fired);
              } else { // Bend
                multNoiseMT21B++;
                mMBendNoiseMap21->Fill(colId + 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 3) {
              if (board == 4) {
                multNoiseMT22NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendNoiseMap22->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), fired);
              } else { // Bend
                multNoiseMT22B++;
                mMBendNoiseMap22->Fill(colId + 0.5, lineHitPos, fired);
              }
            }
          } else {
            if (ichamber == 0) {
              if (board == 4) { // Non-Bend
                multNoiseMT11NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendNoiseMap11->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), fired);
              } else { // Bend
                multNoiseMT11B++;
                mMBendNoiseMap11->Fill(-colId - 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 1) {
              if (board == 4) {
                multNoiseMT12NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendNoiseMap12->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), fired);
              } else { // Bend
                multNoiseMT12B++;
                mMBendNoiseMap12->Fill(-colId - 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 2) {
              if (board == 4) {
                multNoiseMT21NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendNoiseMap21->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), fired);
              } else { // Bend
                multNoiseMT21B++;
                mMBendNoiseMap21->Fill(-colId - 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 3) {
              if (board == 4) {
                multNoiseMT22NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendNoiseMap22->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), fired);
              } else { // Bend
                multNoiseMT22B++;
                mMBendNoiseMap22->Fill(-colId - 0.5, lineHitPos, fired);
              }
            }
          } // isRightSide
          mask <<= 1;
        } // pattern loop 1-16
      }   // noise.patterns[board] Bend or Non-Bend not empty
    }     // boards loop  in DE
  }       // DE loop
  // Histograms noise strip Multiplicity ::
  mMMultNoiseMT11B->Fill(multNoiseMT11B);
  mMMultNoiseMT12B->Fill(multNoiseMT12B);
  mMMultNoiseMT21B->Fill(multNoiseMT21B);
  mMMultNoiseMT22B->Fill(multNoiseMT22B);
  mMMultNoiseMT11NB->Fill(multNoiseMT11NB);
  mMMultNoiseMT12NB->Fill(multNoiseMT12NB);
  mMMultNoiseMT21NB->Fill(multNoiseMT21NB);
  mMMultNoiseMT22NB->Fill(multNoiseMT22NB);

  auto deads = ctx.inputs().get<gsl::span<o2::mid::ColumnData>>("deadch");

  mDeadROF++;

  multDeadMT11B = 0;
  multDeadMT12B = 0;
  multDeadMT21B = 0;
  multDeadMT22B = 0;
  multDeadMT11NB = 0;
  multDeadMT12NB = 0;
  multDeadMT21NB = 0;
  multDeadMT22NB = 0;

  // printf(" deadROF =%i ;EventType %02d   ;  \n", deadROF, deadrof.eventType);
  // if (deadROF != 1)
  //  continue; /// TEST ONE FET EVENT ONLY !!!

  int Ntest = 0;
  // loadStripPatterns (ColumnData)

  for (auto& dead : deads) { // loop DE //

    // std::cout << "!!!! START monitorData CalibMQcTaskLoop deads (DE)!!!! " << std::endl;

    int deIndex = dead.deId;
    int colId = dead.columnId;
    int rpcLine = o2::mid::detparams::getRPCLine(deIndex);
    int ichamber = o2::mid::detparams::getChamber(deIndex);
    auto isRightSide = o2::mid::detparams::isRightSide(deIndex);
    auto detId = o2::mid::detparams::getDEId(isRightSide, ichamber, colId);
    Ntest++;
    // if (ichamber == 0)std::cout << "====> new  ColumnData : Nb = "  << Ntest <<" ich = "<<ichamber<< std::endl;

    int nZoneHistoX = 1;
    int nZoneHistoY = 4;
    double stripYSize = 1. / 16;
    double stripXSize = 0.25 / 16;
    if (mMapping.getLastBoardBP(colId, deIndex) == 1) {
      nZoneHistoX = 2;
      stripXSize = 0.5 / 16;
    } else if (mMapping.getLastBoardBP(colId, deIndex) == 0) {
      nZoneHistoX = 4;
      stripXSize = 1. / 16;
    }
    for (int board = mMapping.getFirstBoardBP(colId, deIndex), lastBoard = mMapping.getLastBoardBP(colId, deIndex); board <= lastBoard + 1; board++) {
      // These are the existing bend boards for this column Id (board = n°board in the column) + 1 (for non-bend)

      if ((lastBoard < 4) && (board == lastBoard + 1))
        board = 4;                     // for Non-Bend digits
      if (dead.patterns[board] != 0) { //  Bend + Non-Bend plane
        //// Bend Local Boards Display ::
        double linePos0 = rpcLine;

        //// Strips Display ::
        int mask = 1;

        double shift = 0; // for central zone with only 3 loc boards
        if ((colId == 0) && ((rpcLine == 3) || (rpcLine == 5)))
          nZoneHistoY = 3;
        if ((colId == 0) && (rpcLine == 5))
          shift = 0.25;

        for (int j = 0; j < 16; j++) {
          int fired = 0;
          if ((dead.patterns[board] & mask) != 0)
            fired = 1;
          double lineHitPos = linePos0 + (j * stripXSize);

          int colj = j;
          if (mMapping.getNStripsNBP(colId, deIndex) == 8)
            colj = j * 2; // Bend with only 8 stripY
          double colHitPos = colId + (colj * stripYSize);
          // if (ichamber == 0)std::cout << "col = "<<colId<<" line = "<<linePos0 <<" board = "<<board<<" bit (j) =>>  " << j << "  lineHitPos = "<< lineHitPos << std::endl;

          if (isRightSide) {
            if (ichamber == 0) {
              if (board == 4) { // Non-Bend
                multDeadMT11NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendDeadMap11->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), fired);
              } else { // Bend
                multDeadMT11B++;
                mMBendDeadMap11->Fill(colId + 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 1) {
              if (board == 4) {
                multDeadMT12NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendDeadMap12->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), fired); // Non-Bend
              } else {                                                                            // Bend
                multDeadMT12B++;
                mMBendDeadMap12->Fill(colId + 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 2) {
              if (board == 4) {
                multDeadMT21NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendDeadMap21->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), fired); // Non-Bend
              } else {                                                                            // Bend
                multDeadMT21B++;
                mMBendDeadMap21->Fill(colId + 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 3) {
              if (board == 4) {
                multDeadMT22NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendDeadMap22->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), fired); // Non-Bend
              } else {                                                                            // Bend
                multDeadMT22B++;
                mMBendDeadMap22->Fill(colId + 0.5, lineHitPos, fired);
              }
            }
          } else {
            if (ichamber == 0) {
              if (board == 4) {
                multDeadMT11NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendDeadMap11->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), fired); // Non-Bend
              } else {                                                                             // Bend
                multDeadMT11B++;
                mMBendDeadMap11->Fill(-colId - 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 1) {
              if (board == 4) {
                multDeadMT12NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendDeadMap12->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), fired); // Non-Bend
              } else {                                                                             // Bend
                multDeadMT12B++;
                mMBendDeadMap12->Fill(-colId - 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 2) {
              if (board == 4) {
                multDeadMT21NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendDeadMap21->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), fired); // Non-Bend
              } else {                                                                             // Bend
                multDeadMT21B++;
                mMBendDeadMap21->Fill(-colId - 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 3) {
              if (board == 4) {
                multDeadMT22NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendDeadMap22->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), fired); // Non-Bend
              } else {                                                                             // Bend
                multDeadMT22B++;
                mMBendDeadMap22->Fill(-colId - 0.5, lineHitPos, fired);
              }
            }
          } // IsRightSide
          mask <<= 1;
        } // loop pattern 1-16
      }   // Dead.patterns[board] Bend or Non-Bend not empty
    }     // loop board in DE
  }       // loop DE dead
  // Histograms Dead Strip Multiplicity ::
  // std::cout << "!!!! 5 monitorData CalibQcTask !!!! " << std::endl;
  // printf(" Mult Dead B = %i ; %i   ; %i ; %i \n", multDeadMT11B,  multDeadMT12B, multDeadMT21B, multDeadMT22B);
  // printf(" Mult Dead NB = %i ; %i   ; %i ; %i \n", multDeadMT11NB,  multDeadMT12NB, multDeadMT21NB, multDeadMT22NB);

  mMMultDeadMT11B->Fill(multDeadMT11B);
  mMMultDeadMT12B->Fill(multDeadMT12B);
  mMMultDeadMT21B->Fill(multDeadMT21B);
  mMMultDeadMT22B->Fill(multDeadMT22B);
  mMMultDeadMT11NB->Fill(multDeadMT11NB);
  mMMultDeadMT12NB->Fill(multDeadMT12NB);
  mMMultDeadMT21NB->Fill(multDeadMT21NB);
  mMMultDeadMT22NB->Fill(multDeadMT22NB);
  // std::cout << "!!!! END boards loop in ROF !!!! " << std::endl;

  ////  } //  deadROFs //

  mBadROF++;

  multBadMT11B = 0;
  multBadMT12B = 0;
  multBadMT21B = 0;
  multBadMT22B = 0;
  multBadMT11NB = 0;
  multBadMT12NB = 0;
  multBadMT21NB = 0;
  multBadMT22NB = 0;

  auto bads = ctx.inputs().get<gsl::span<o2::mid::ColumnData>>("badch");
  // printf("----------------> %05d TF \n", nTF);

  // printf("bad :: ========================================================== \n");
  for (auto& bad : bads) { // loop DE //

    // std::cout << "!!!! START monitorData CalibMQcTaskLoop bads (DE)!!!! " << std::endl;

    int deIndex = bad.deId;
    int colId = bad.columnId;
    int rpcLine = o2::mid::detparams::getRPCLine(deIndex);
    int ichamber = o2::mid::detparams::getChamber(deIndex);
    auto isRightSide = o2::mid::detparams::isRightSide(deIndex);
    auto detId = o2::mid::detparams::getDEId(isRightSide, ichamber, colId);
    Ntest++;
    // if (ichamber == 0)std::cout << "====> new  ColumnData : Nb = "  << Ntest <<" ich = "<<ichamber<< std::endl;

    int nZoneHistoX = 1;
    int nZoneHistoY = 4;
    double stripYSize = 1. / 16;
    double stripXSize = 0.25 / 16;
    if (mMapping.getLastBoardBP(colId, deIndex) == 1) {
      nZoneHistoX = 2;
      stripXSize = 0.5 / 16;
    } else if (mMapping.getLastBoardBP(colId, deIndex) == 0) {
      nZoneHistoX = 4;
      stripXSize = 1. / 16;
    }
    for (int board = mMapping.getFirstBoardBP(colId, deIndex), lastBoard = mMapping.getLastBoardBP(colId, deIndex); board <= lastBoard + 1; board++) {
      // These are the existing bend boards for this column Id (board = n°board in the column) + 1 (for non-bend)

      if ((lastBoard < 4) && (board == lastBoard + 1))
        board = 4;                    // for Non-Bend digits
      if (bad.patterns[board] != 0) { //  Bend + Non-Bend plane
        //// Bend Local Boards Display ::
        double linePos0 = rpcLine;

        //// Strips Display ::
        int mask = 1;

        double shift = 0; // for central zone with only 3 loc boards
        if ((colId == 0) && ((rpcLine == 3) || (rpcLine == 5)))
          nZoneHistoY = 3;
        if ((colId == 0) && (rpcLine == 5))
          shift = 0.25;

        for (int j = 0; j < 16; j++) {
          int fired = 0;
          if ((bad.patterns[board] & mask) != 0)
            fired = 1;
          double lineHitPos = linePos0 + (j * stripXSize);

          int colj = j;
          if (mMapping.getNStripsNBP(colId, deIndex) == 8)
            colj = j * 2; // Bend with only 8 stripY
          double colHitPos = colId + (colj * stripYSize);
          // if (ichamber == 0)std::cout << "col = "<<colId<<" line = "<<linePos0 <<" board = "<<board<<" bit (j) =>>  " << j << "  lineHitPos = "<< lineHitPos << std::endl;

          if (isRightSide) {
            if (ichamber == 0) {
              if (board == 4) { // Non-Bend
                multBadMT11NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendBadMap11->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), fired);
              } else { // Bend
                multBadMT11B++;
                mMBendBadMap11->Fill(colId + 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 1) {
              if (board == 4) {
                multDeadMT12NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendBadMap12->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), fired); // Non-Bend
              } else {                                                                           // Bend
                multBadMT12B++;
                mMBendBadMap12->Fill(colId + 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 2) {
              if (board == 4) {
                multBadMT21NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendBadMap21->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), fired); // Non-Bend
              } else {                                                                           // Bend
                multBadMT21B++;
                mMBendBadMap21->Fill(colId + 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 3) {
              if (board == 4) {
                multBadMT22NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendBadMap22->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), fired); // Non-Bend
              } else {                                                                           // Bend
                multBadMT22B++;
                mMBendBadMap22->Fill(colId + 0.5, lineHitPos, fired);
              }
            }
          } else {
            if (ichamber == 0) {
              if (board == 4) {
                multBadMT11NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendBadMap11->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), fired); // Non-Bend
              } else {                                                                            // Bend
                multBadMT11B++;
                mMBendBadMap11->Fill(-colId - 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 1) {
              if (board == 4) {
                multBadMT12NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendBadMap12->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), fired); // Non-Bend
              } else {                                                                            // Bend
                multBadMT12B++;
                mMBendBadMap12->Fill(-colId - 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 2) {
              if (board == 4) {
                multBadMT21NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendBadMap21->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), fired); // Non-Bend
              } else {                                                                            // Bend
                multBadMT21B++;
                mMBendBadMap21->Fill(-colId - 0.5, lineHitPos, fired);
              }
            } else if (ichamber == 3) {
              if (board == 4) {
                multBadMT22NB++;
                for (int ib = 0; ib < nZoneHistoY; ib++)
                  mMNBendBadMap22->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), fired); // Non-Bend
              } else {                                                                            // Bend
                multBadMT22B++;
                mMBendBadMap22->Fill(-colId - 0.5, lineHitPos, fired);
              }
            }
          } // IsRightSide
          mask <<= 1;
        } // loop pattern 1-16
      }   // Dead.patterns[board] Bend or Non-Bend not empty
    }     // loop board in DE

  } // loop DE bad

  mMMultBadMT11B->Fill(multBadMT11B);
  mMMultBadMT12B->Fill(multBadMT12B);
  mMMultBadMT21B->Fill(multBadMT21B);
  mMMultBadMT22B->Fill(multBadMT22B);
  mMMultBadMT11NB->Fill(multBadMT11NB);
  mMMultBadMT12NB->Fill(multBadMT12NB);
  mMMultBadMT21NB->Fill(multBadMT21NB);
  mMMultBadMT22NB->Fill(multBadMT22NB);

  // std::cout << "!!!! END monitorData CalibQcTask !!!! " << std::endl;
}

void CalibMQcTask::endOfCycle()
{
  ILOG(Info, Devel) << "endOfCycle" << ENDM;
}

void CalibMQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Devel) << "endOfActivity" << ENDM;
}

void CalibMQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Devel) << "Resetting the histogram" << ENDM;

  mMMultNoiseMT11B->Reset();
  mMMultNoiseMT11NB->Reset();
  mMMultNoiseMT12B->Reset();
  mMMultNoiseMT12NB->Reset();
  mMMultNoiseMT21B->Reset();
  mMMultNoiseMT21NB->Reset();
  mMMultNoiseMT22B->Reset();
  mMMultNoiseMT22NB->Reset();

  mMBendNoiseMap11->Reset();
  mMBendNoiseMap12->Reset();
  mMBendNoiseMap21->Reset();
  mMBendNoiseMap22->Reset();
  mMNBendNoiseMap11->Reset();
  mMNBendNoiseMap12->Reset();
  mMNBendNoiseMap21->Reset();
  mMNBendNoiseMap22->Reset();

  mMMultDeadMT11B->Reset();
  mMMultDeadMT11NB->Reset();
  mMMultDeadMT12B->Reset();
  mMMultDeadMT12NB->Reset();
  mMMultDeadMT21B->Reset();
  mMMultDeadMT21NB->Reset();
  mMMultDeadMT22B->Reset();
  mMMultDeadMT22NB->Reset();

  mMBendDeadMap11->Reset();
  mMBendDeadMap12->Reset();
  mMBendDeadMap21->Reset();
  mMBendDeadMap22->Reset();
  mMNBendDeadMap11->Reset();
  mMNBendDeadMap12->Reset();
  mMNBendDeadMap21->Reset();
  mMNBendDeadMap22->Reset();

  mMBendBadMap11->Reset();
  mMBendBadMap12->Reset();
  mMBendBadMap21->Reset();
  mMBendBadMap22->Reset();
  mMNBendBadMap11->Reset();
  mMNBendBadMap12->Reset();
  mMNBendBadMap21->Reset();
  mMNBendBadMap22->Reset();
}

} // namespace o2::quality_control_modules::mid
