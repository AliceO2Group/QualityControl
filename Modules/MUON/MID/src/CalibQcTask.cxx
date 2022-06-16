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

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TFile.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "QualityControl/QcInfoLogger.h"
#include "MID/CalibQcTask.h"
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

CalibQcTask::~CalibQcTask()
{
}

void CalibQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize CalibQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  std::cout << "!!!! START initialize CalibQcTask !!!! " << std::endl;
  /////////////////
  /// Noise strips Histograms ::

  mMultNoiseMT11B = std::make_shared<TH1F>("MultNoiseMT11B", "Multiplicity Noise strips - MT11 bending plane", 300, 0, 3000);
  mMultNoiseMT11NB = std::make_shared<TH1F>("MultNoiseMT11NB", "Multiplicity Noise strips - MT11 non-bending plane", 300, 0, 3000);
  mMultNoiseMT12B = std::make_shared<TH1F>("MultNoiseMT12B", "Multiplicity Noise strips - MT12 bending plane", 300, 0, 3000);
  mMultNoiseMT12NB = std::make_shared<TH1F>("MultNoiseMT12NB", "Multiplicity Noise strips - MT12 non-bending plane", 300, 0, 3000);
  mMultNoiseMT21B = std::make_shared<TH1F>("MultNoiseMT21B", "Multiplicity Noise strips - MT21 bending plane", 300, 0, 3000);
  mMultNoiseMT21NB = std::make_shared<TH1F>("MultNoiseMT21NB", "Multiplicity Noise strips - MT21 non-bending plane", 300, 0, 3000);
  mMultNoiseMT22B = std::make_shared<TH1F>("MultNoiseMT22B", "Multiplicity Noise strips - MT22 bending plane", 300, 0, 3000);
  mMultNoiseMT22NB = std::make_shared<TH1F>("MultNoiseMT22NB", "Multiplicity Noise strips - MT22 non-bending plane", 300, 0, 3000);
  getObjectsManager()->startPublishing(mMultNoiseMT11B.get());
  getObjectsManager()->startPublishing(mMultNoiseMT11NB.get());
  getObjectsManager()->startPublishing(mMultNoiseMT12B.get());
  getObjectsManager()->startPublishing(mMultNoiseMT12NB.get());
  getObjectsManager()->startPublishing(mMultNoiseMT21B.get());
  getObjectsManager()->startPublishing(mMultNoiseMT21NB.get());
  getObjectsManager()->startPublishing(mMultNoiseMT22B.get());
  getObjectsManager()->startPublishing(mMultNoiseMT22NB.get());

  mBendNoiseMap11 = std::make_shared<TH2F>("BendNoiseMap11", "Bending Noise Map MT11", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mBendNoiseMap11.get());
  mBendNoiseMap11->GetXaxis()->SetTitle("Column");
  mBendNoiseMap11->GetYaxis()->SetTitle("Line");
  mBendNoiseMap11->SetOption("colz");
  mBendNoiseMap11->SetStats(0);

  mBendNoiseMap12 = std::make_shared<TH2F>("BendNoiseMap12", "Bending Noise Map MT12", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mBendNoiseMap12.get());
  mBendNoiseMap12->GetXaxis()->SetTitle("Column");
  mBendNoiseMap12->GetYaxis()->SetTitle("Line");
  mBendNoiseMap12->SetOption("colz");
  mBendNoiseMap12->SetStats(0);

  mBendNoiseMap21 = std::make_shared<TH2F>("BendNoiseMap21", "Bending Noise Map MT21", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mBendNoiseMap21.get());
  mBendNoiseMap21->GetXaxis()->SetTitle("Column");
  mBendNoiseMap21->GetYaxis()->SetTitle("Line");
  mBendNoiseMap21->SetOption("colz");
  mBendNoiseMap21->SetStats(0);

  mBendNoiseMap22 = std::make_shared<TH2F>("BendNoiseMap22", "Bending Noise Map MT22", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mBendNoiseMap22.get());
  mBendNoiseMap22->GetXaxis()->SetTitle("Column");
  mBendNoiseMap22->GetYaxis()->SetTitle("Line");
  mBendNoiseMap22->SetOption("colz");
  mBendNoiseMap22->SetStats(0);

  mNBendNoiseMap11 = std::make_shared<TH2F>("NBendNoiseMap11", "Non-Bending Noise Map MT11", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mNBendNoiseMap11.get());
  mNBendNoiseMap11->GetXaxis()->SetTitle("Column");
  mNBendNoiseMap11->GetYaxis()->SetTitle("Line");
  mNBendNoiseMap11->SetOption("colz");
  mNBendNoiseMap11->SetStats(0);

  mNBendNoiseMap12 = std::make_shared<TH2F>("NBendNoiseMap12", "Non-Bending Noise Map MT12", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mNBendNoiseMap12.get());
  mNBendNoiseMap12->GetXaxis()->SetTitle("Column");
  mNBendNoiseMap12->GetYaxis()->SetTitle("Line");
  mNBendNoiseMap12->SetOption("colz");
  mNBendNoiseMap12->SetStats(0);

  mNBendNoiseMap21 = std::make_shared<TH2F>("NBendNoiseMap21", "Non-Bending Noise Map MT21", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mNBendNoiseMap21.get());
  mNBendNoiseMap21->GetXaxis()->SetTitle("Column");
  mNBendNoiseMap21->GetYaxis()->SetTitle("Line");
  mNBendNoiseMap21->SetOption("colz");
  mNBendNoiseMap21->SetStats(0);

  mNBendNoiseMap22 = std::make_shared<TH2F>("NBendNoiseMap22", "Non-Bending Noise Map MT22", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mNBendNoiseMap22.get());
  mNBendNoiseMap22->GetXaxis()->SetTitle("Column");
  mNBendNoiseMap22->GetYaxis()->SetTitle("Line");
  mNBendNoiseMap22->SetOption("colz");
  mNBendNoiseMap22->SetStats(0);

  /////////////////
  /// Dead strips Histograms ::
  mMultDeadMT11B = std::make_shared<TH1F>("MultDeadMT11B", "Multiplicity Dead strips - MT11 bending plane", 300, 0, 300);
  mMultDeadMT11NB = std::make_shared<TH1F>("MultDeadMT11NB", "Multiplicity Dead strips - MT11 non-bending plane", 300, 0, 300);
  mMultDeadMT12B = std::make_shared<TH1F>("MultDeadMT12B", "Multiplicity Dead strips - MT12 bending plane", 300, 0, 300);
  mMultDeadMT12NB = std::make_shared<TH1F>("MultDeadMT12NB", "MultiplicityDead  strips - MT12 non-bending plane", 300, 0, 300);
  mMultDeadMT21B = std::make_shared<TH1F>("MultDeadMT21B", "Multiplicity Dead strips - MT21 bending plane", 300, 0, 300);
  mMultDeadMT21NB = std::make_shared<TH1F>("MultDeadMT21NB", "Multiplicity Dead strips - MT21 non-bending plane", 300, 0, 300);
  mMultDeadMT22B = std::make_shared<TH1F>("MultDeadMT22B", "Multiplicity Dead strips - MT22 bending plane", 300, 0, 300);
  mMultDeadMT22NB = std::make_shared<TH1F>("MultDeadMT22NB", "Multiplicity Dead strips - MT22 non-bending plane", 300, 0, 300);
  getObjectsManager()->startPublishing(mMultDeadMT11B.get());
  getObjectsManager()->startPublishing(mMultDeadMT11NB.get());
  getObjectsManager()->startPublishing(mMultDeadMT12B.get());
  getObjectsManager()->startPublishing(mMultDeadMT12NB.get());
  getObjectsManager()->startPublishing(mMultDeadMT21B.get());
  getObjectsManager()->startPublishing(mMultDeadMT21NB.get());
  getObjectsManager()->startPublishing(mMultDeadMT22B.get());
  getObjectsManager()->startPublishing(mMultDeadMT22NB.get());

  mBendDeadMap11 = std::make_shared<TH2F>("BendDeadMap11", "Bending Dead Map MT11", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mBendDeadMap11.get());
  mBendDeadMap11->GetXaxis()->SetTitle("Column");
  mBendDeadMap11->GetYaxis()->SetTitle("Line");
  mBendDeadMap11->SetOption("colz");
  mBendDeadMap11->SetStats(0);

  mBendDeadMap12 = std::make_shared<TH2F>("BendDeadMap12", "Bending Dead Map MT12", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mBendDeadMap12.get());
  mBendDeadMap12->GetXaxis()->SetTitle("Column");
  mBendDeadMap12->GetYaxis()->SetTitle("Line");
  mBendDeadMap12->SetOption("colz");
  mBendDeadMap12->SetStats(0);

  mBendDeadMap21 = std::make_shared<TH2F>("BendDeadMap21", "Bending Dead Map MT21", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mBendDeadMap21.get());
  mBendDeadMap21->GetXaxis()->SetTitle("Column");
  mBendDeadMap21->GetYaxis()->SetTitle("Line");
  mBendDeadMap21->SetOption("colz");
  mBendDeadMap21->SetStats(0);

  mBendDeadMap22 = std::make_shared<TH2F>("BendDeadMap22", "Bending Dead Map MT22", 14, -7, 7, 576, 0, 9);
  getObjectsManager()->startPublishing(mBendDeadMap22.get());
  mBendDeadMap22->GetXaxis()->SetTitle("Column");
  mBendDeadMap22->GetYaxis()->SetTitle("Line");
  mBendDeadMap22->SetOption("colz");
  mBendDeadMap22->SetStats(0);

  mNBendDeadMap11 = std::make_shared<TH2F>("NBendDeadMap11", "Non-Bending Dead Map MT11", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mNBendDeadMap11.get());
  mNBendDeadMap11->GetXaxis()->SetTitle("Column");
  mNBendDeadMap11->GetYaxis()->SetTitle("Line");
  mNBendDeadMap11->SetOption("colz");
  mNBendDeadMap11->SetStats(0);

  mNBendDeadMap12 = std::make_shared<TH2F>("NBendDeadMap12", "Non-Bending Dead Map MT12", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mNBendDeadMap12.get());
  mNBendDeadMap12->GetXaxis()->SetTitle("Column");
  mNBendDeadMap12->GetYaxis()->SetTitle("Line");
  mNBendDeadMap12->SetOption("colz");
  mNBendDeadMap12->SetStats(0);

  mNBendDeadMap21 = std::make_shared<TH2F>("NBendDeadMap21", "Non-Bending Dead Map MT21", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mNBendDeadMap21.get());
  mNBendDeadMap21->GetXaxis()->SetTitle("Column");
  mNBendDeadMap21->GetYaxis()->SetTitle("Line");
  mNBendDeadMap21->SetOption("colz");
  mNBendDeadMap21->SetStats(0);

  mNBendDeadMap22 = std::make_shared<TH2F>("NBendDeadMap22", "Non-Bending Dead Map MT22", 224, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mNBendDeadMap22.get());
  mNBendDeadMap22->GetXaxis()->SetTitle("Column");
  mNBendDeadMap22->GetYaxis()->SetTitle("Line");
  mNBendDeadMap22->SetOption("colz");
  mNBendDeadMap22->SetStats(0);
}

void CalibQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
}

void CalibQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void CalibQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  std::cout << "!!!! START monitorData CalibQcTask !!!! " << std::endl;

  //auto noises = ctx.inputs().get<gsl::span<o2::mid::ColumnData>>("noise");
  //auto noiserofs = ctx.inputs().get<gsl::span<o2::mid::ROFRecord>>("noiserofs");

  auto noises = o2::mid::specs::getData(ctx, "noise", o2::mid::EventType::Standard);
  auto noiserofs = o2::mid::specs::getRofs(ctx, "noise", o2::mid::EventType::Standard);
 
  
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

  for (const auto& noiserof : noiserofs) { // loop noiseROFs //
    // printf("========================================================== \n");
    // printf("%05d ROF with first entry %05zu and nentries %02zu , BC %05d, ORB %05d , EventType %02d\n", noiseROF, noiserof.firstEntry, noiserof.nEntries, noiserof.interactionRecord.bc, noiserof.interactionRecord.orbit,noiserof.eventType);
    //   eventType::  Standard = 0, Calib = 1, FET = 2
    noiseROF++;
    multNoiseMT11B = 0;
    multNoiseMT12B = 0;
    multNoiseMT21B = 0;
    multNoiseMT22B = 0;
    multNoiseMT11NB = 0;
    multNoiseMT12NB = 0;
    multNoiseMT21NB = 0;
    multNoiseMT22NB = 0;

    // printf(" noiseROF =%i ;EventType %02d   ;  \n", noiseROF, noiserof.eventType);
    //  loadStripPatterns (ColumnData)
    for (auto& noise : noises.subspan(noiserof.firstEntry, noiserof.nEntries)) { // loop DE //
      int deIndex = noise.deId;
      int colId = noise.columnId;
      int rpcLine = o2::mid::detparams::getRPCLine(deIndex);
      int ichamber = o2::mid::detparams::getChamber(deIndex);
      auto isRightSide = o2::mid::detparams::isRightSide(deIndex);
      auto detId = o2::mid::detparams::getDEId(isRightSide, ichamber, colId);

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
            if ((noise.patterns[board] & mask) != 0) {
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
                      mNBendNoiseMap11->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), 1);
                  } else { // Bend
                    multNoiseMT11B++;
                    mBendNoiseMap11->Fill(colId + 0.5, lineHitPos, 1);
                  }
                } else if (ichamber == 1) {
                  if (board == 4) {
                    multNoiseMT12NB++;
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendNoiseMap12->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), 1);
                  } else { // Bend
                    multNoiseMT12B++;
                    mBendNoiseMap12->Fill(colId + 0.5, lineHitPos, 1);
                  }
                } else if (ichamber == 2) {
                  if (board == 4) {
                    multNoiseMT21NB++;
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendNoiseMap21->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), 1);
                  } else { // Bend
                    multNoiseMT21B++;
                    mBendNoiseMap21->Fill(colId + 0.5, lineHitPos, 1);
                  }
                } else if (ichamber == 3) {
                  if (board == 4) {
                    multNoiseMT22NB++;
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendNoiseMap22->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), 1);
                  } else { // Bend
                    multNoiseMT22B++;
                    mBendNoiseMap22->Fill(colId + 0.5, lineHitPos, 1);
                  }
                }
              } else {
                if (ichamber == 0) {
                  if (board == 4) { // Non-Bend
                    multNoiseMT11NB++;
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendNoiseMap11->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), 1);
                  } else { // Bend
                    multNoiseMT11B++;
                    mBendNoiseMap11->Fill(-colId - 0.5, lineHitPos, 1);
                  }
                } else if (ichamber == 1) {
                  if (board == 4) {
                    multNoiseMT12NB++;
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendNoiseMap12->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), 1);
                  } else { // Bend
                    multNoiseMT12B++;
                    mBendNoiseMap12->Fill(-colId - 0.5, lineHitPos, 1);
                  }
                } else if (ichamber == 2) {
                  if (board == 4) {
                    multNoiseMT21NB++;
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendNoiseMap21->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), 1);
                  } else { // Bend
                    multNoiseMT21B++;
                    mBendNoiseMap21->Fill(-colId - 0.5, lineHitPos, 1);
                  }
                } else if (ichamber == 3) {
                  if (board == 4) {
                    multNoiseMT22NB++;
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendNoiseMap22->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), 1);
                  } else { // Bend
                    multNoiseMT22B++;
                    mBendNoiseMap22->Fill(-colId - 0.5, lineHitPos, 1);
                  }
                }
              } // isRightSide
            }   // noisePattern & mask
            mask <<= 1;
          } // pattern loop 1-16
        }   // noise.patterns[board] Bend or Non-Bend not empty
      }     // boards loop  in DE
    }       // DE loop
    // Histograms noise strip Multiplicity ::
    mMultNoiseMT11B->Fill(multNoiseMT11B);
    mMultNoiseMT12B->Fill(multNoiseMT12B);
    mMultNoiseMT21B->Fill(multNoiseMT21B);
    mMultNoiseMT22B->Fill(multNoiseMT22B);
    mMultNoiseMT11NB->Fill(multNoiseMT11NB);
    mMultNoiseMT12NB->Fill(multNoiseMT12NB);
    mMultNoiseMT21NB->Fill(multNoiseMT21NB);
    mMultNoiseMT22NB->Fill(multNoiseMT22NB);
  } //  noiseROFs //

  //auto deads = ctx.inputs().get<gsl::span<o2::mid::ColumnData>>("dead");
  //auto deadrofs = ctx.inputs().get<gsl::span<o2::mid::ROFRecord>>("deadrofs");

  auto deads = o2::mid::specs::getData(ctx, "dead", o2::mid::EventType::Standard);
  auto deadrofs = o2::mid::specs::getRofs(ctx, "dead", o2::mid::EventType::Standard);

  for (const auto& deadrof : deadrofs) { // loop deadROFs //
    printf("========================================================== \n");
    printf("%05d deadROF with first entry %05zu and nentries %02zu , BC %05d, ORB %05d , EventType %02d\n", deadROF, deadrof.firstEntry, deadrof.nEntries, deadrof.interactionRecord.bc, deadrof.interactionRecord.orbit, deadrof.eventType);
    //  eventType::  Standard = 0, Calib = 1, FET = 2
    deadROF++;

    multDeadMT11B = 0;
    multDeadMT12B = 0;
    multDeadMT21B = 0;
    multDeadMT22B = 0;
    multDeadMT11NB = 0;
    multDeadMT12NB = 0;
    multDeadMT21NB = 0;
    multDeadMT22NB = 0;

    // printf(" deadROF =%i ;EventType %02d   ;  \n", deadROF, deadrof.eventType);
    if (deadROF != 1)
      continue; /// TEST ONE FET EVENT ONLY !!!

    int Ntest = 0;
    // loadStripPatterns (ColumnData)
    for (auto& dead : deads.subspan(deadrof.firstEntry, deadrof.nEntries)) { // loop DE //
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
            if ((dead.patterns[board] & mask) != 0) {
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
                      mNBendDeadMap11->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), 1);
                  } else { // Bend
                    multDeadMT11B++;
                    mBendDeadMap11->Fill(colId + 0.5, lineHitPos, 1);
                  }
                } else if (ichamber == 1) {
                  if (board == 4) {
                    multDeadMT12NB++;
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendDeadMap12->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else {                                                                       // Bend
                    multDeadMT12B++;
                    mBendDeadMap12->Fill(colId + 0.5, lineHitPos, 1);
                  }
                } else if (ichamber == 2) {
                  if (board == 4) {
                    multDeadMT21NB++;
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendDeadMap21->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else {                                                                       // Bend
                    multDeadMT21B++;
                    mBendDeadMap21->Fill(colId + 0.5, lineHitPos, 1);
                  }
                } else if (ichamber == 3) {
                  if (board == 4) {
                    multDeadMT22NB++;
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendDeadMap22->Fill(colHitPos + 0.01, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else {                                                                       // Bend
                    multDeadMT22B++;
                    mBendDeadMap22->Fill(colId + 0.5, lineHitPos, 1);
                  }
                }
              } else {
                if (ichamber == 0) {
                  if (board == 4) {
                    multDeadMT11NB++;
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendDeadMap11->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else {                                                                        // Bend
                    multDeadMT11B++;
                    mBendDeadMap11->Fill(-colId - 0.5, lineHitPos, 1);
                  }
                } else if (ichamber == 1) {
                  if (board == 4) {
                    multDeadMT12NB++;
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendDeadMap12->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else {                                                                        // Bend
                    multDeadMT12B++;
                    mBendDeadMap12->Fill(-colId - 0.5, lineHitPos, 1);
                  }
                } else if (ichamber == 2) {
                  if (board == 4) {
                    multDeadMT21NB++;
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendDeadMap21->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else {                                                                        // Bend
                    multDeadMT21B++;
                    mBendDeadMap21->Fill(-colId - 0.5, lineHitPos, 1);
                  }
                } else if (ichamber == 3) {
                  if (board == 4) {
                    multDeadMT22NB++;
                    for (int ib = 0; ib < nZoneHistoY; ib++)
                      mNBendDeadMap22->Fill(-colHitPos - 0.01, rpcLine + shift + (0.25 * ib), 1); // Non-Bend
                  } else {                                                                        // Bend
                    multDeadMT22B++;
                    mBendDeadMap22->Fill(-colId - 0.5, lineHitPos, 1);
                  }
                }
              } // IsRightSide
            }   // Dead.pattern & mask
            mask <<= 1;
          } // loop pattern 1-16
        }   // Dead.patterns[board] Bend or Non-Bend not empty
      }     // loop board in DE
    }       // loop DE
    // Histograms Dead Strip Multiplicity ::
    // std::cout << "!!!! 5 monitorData CalibQcTask !!!! " << std::endl;
    // printf(" Mult Dead B = %i ; %i   ; %i ; %i \n", multDeadMT11B,  multDeadMT12B, multDeadMT21B, multDeadMT22B);
    // printf(" Mult Dead NB = %i ; %i   ; %i ; %i \n", multDeadMT11NB,  multDeadMT12NB, multDeadMT21NB, multDeadMT22NB);

    mMultDeadMT11B->Fill(multDeadMT11B);
    mMultDeadMT12B->Fill(multDeadMT12B);
    mMultDeadMT21B->Fill(multDeadMT21B);
    mMultDeadMT22B->Fill(multDeadMT22B);
    mMultDeadMT11NB->Fill(multDeadMT11NB);
    mMultDeadMT12NB->Fill(multDeadMT12NB);
    mMultDeadMT21NB->Fill(multDeadMT21NB);
    mMultDeadMT22NB->Fill(multDeadMT22NB);
    std::cout << "!!!! END boards loop in ROF !!!! " << std::endl;
  } //  deadROFs //
  std::cout << "!!!! END monitorData CalibQcTask !!!! " << std::endl;
}

void CalibQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void CalibQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void CalibQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;

  mMultNoiseMT11B->Reset();
  mMultNoiseMT11NB->Reset();
  mMultNoiseMT12B->Reset();
  mMultNoiseMT12NB->Reset();
  mMultNoiseMT21B->Reset();
  mMultNoiseMT21NB->Reset();
  mMultNoiseMT22B->Reset();
  mMultNoiseMT22NB->Reset();

  mBendNoiseMap11->Reset();
  mBendNoiseMap12->Reset();
  mBendNoiseMap21->Reset();
  mBendNoiseMap22->Reset();
  mNBendNoiseMap11->Reset();
  mNBendNoiseMap12->Reset();
  mNBendNoiseMap21->Reset();
  mNBendNoiseMap22->Reset();

  mMultDeadMT11B->Reset();
  mMultDeadMT11NB->Reset();
  mMultDeadMT12B->Reset();
  mMultDeadMT12NB->Reset();
  mMultDeadMT21B->Reset();
  mMultDeadMT21NB->Reset();
  mMultDeadMT22B->Reset();
  mMultDeadMT22NB->Reset();

  mBendDeadMap11->Reset();
  mBendDeadMap12->Reset();
  mBendDeadMap21->Reset();
  mBendDeadMap22->Reset();
  mNBendDeadMap11->Reset();
  mNBendDeadMap12->Reset();
  mNBendDeadMap21->Reset();
  mNBendDeadMap22->Reset();
}

} // namespace o2::quality_control_modules::mid
