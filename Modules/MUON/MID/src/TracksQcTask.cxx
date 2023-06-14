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
/// \file   TracksQcTask.cxx
/// \author Valerie Ramillien
///

#include <TMath.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TProfile2D.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "QualityControl/QcInfoLogger.h"
#include "MID/TracksQcTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>
#include "DataFormatsMID/Track.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDBase/DetectorParameters.h"
#include "MIDBase/GeometryParameters.h"
#include "MIDWorkflow/ColumnDataSpecsUtils.h"

#define MID_NDE 72
#define MID_NLOC 234
#define DZpos 10

namespace o2::quality_control_modules::mid
{

TracksQcTask::~TracksQcTask()
{
}

void TracksQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  // ILOG(Info, Devel) << "initialize TracksQcTask" << ENDM;
  // printf(" =================== > test  initialize Tracks \n");

  multTracksTot = 0;
  multTracks44Tot = 0;
  multTracksBend44 = 0;
  multTracksNBend44 = 0;
  multTraksB34MT11 = 0;
  multTraksNB34MT11 = 0;
  multTraksB34MT12 = 0;
  multTraksNB34MT12 = 0;
  multTraksB34MT21 = 0;
  multTraksNB34MT21 = 0;
  multTraksB34MT22 = 0;
  multTraksNB34MT22 = 0;

  mNbTracksTF = std::make_shared<TH1F>("NbTracksTF", "NbTimeFrame", 1, 0, 1.);
  getObjectsManager()->startPublishing(mNbTracksTF.get());

  mTrackBCCounts = std::make_shared<TProfile>("TrackBCCounts", "Mean Tracks in Bunch Crossing ; BC ; Mean Tracks nb", o2::constants::lhc::LHCMaxBunches, 0., o2::constants::lhc::LHCMaxBunches);
  getObjectsManager()->startPublishing(mTrackBCCounts.get());
  // mTrackBCCounts->GetXaxis()->SetTitle("BC");
  // mTrackBCCounts->GetYaxis()->SetTitle("Mean Tracks nb");

  mMultTracks = std::make_shared<TH1F>("MultTracks", "Multiplicity Tracks ", 100, 0, 100);
  getObjectsManager()->startPublishing(mMultTracks.get());

  mTrackMapXY = std::make_shared<TH2F>("TrackMapXY", "Track Map X-Y ; X Position (cm) ; Y Position (cm) ", 300, -300., 300., 300, -300., 300.);
  getObjectsManager()->startPublishing(mTrackMapXY.get());
  // mTrackMapXY->GetXaxis()->SetTitle("X Position (cm)");
  // mTrackMapXY->GetYaxis()->SetTitle("Y Position (cm)");
  mTrackMapXY->SetOption("colz");
  mTrackMapXY->SetStats(0);

  mTrackDevXY = std::make_shared<TH2F>("TrackDevXY", "Track Deviation X-Y", 100, -50., 50., 100, -50., 50.);
  getObjectsManager()->startPublishing(mTrackDevXY.get());
  mTrackDevXY->GetXaxis()->SetTitle("X Dev (cm)");
  mTrackDevXY->GetYaxis()->SetTitle("Y Dev (cm)");
  mTrackDevXY->SetOption("colz");
  mTrackDevXY->SetStats(0);

  mTrackDevX = std::make_shared<TH1F>("TrackDevX", "Track Deviation X", 100, -50., 50.);
  getObjectsManager()->startPublishing(mTrackDevX.get());
  mTrackDevX->GetXaxis()->SetTitle("Track Dev X (cm)");
  mTrackDevX->GetYaxis()->SetTitle("Entry");

  mTrackDevY = std::make_shared<TH1F>("TrackDevY", "Track Deviation Y", 100, -50., 50.);
  getObjectsManager()->startPublishing(mTrackDevY.get());
  mTrackDevY->GetXaxis()->SetTitle("Track Dev Y (cm)");
  mTrackDevY->GetYaxis()->SetTitle("Entry");

  mTrackTheta = std::make_shared<TH1F>("TrackTheta", "Track Theta", 120, 0., 12.);
  getObjectsManager()->startPublishing(mTrackTheta.get());
  mTrackTheta->GetXaxis()->SetTitle("Track Theta");
  mTrackTheta->GetYaxis()->SetTitle("Entry");

  mTrackEta = std::make_shared<TH1F>("TrackEta", "Track Eta", 250, 2., 4.5);
  getObjectsManager()->startPublishing(mTrackEta.get());
  mTrackEta->GetXaxis()->SetTitle("Track Eta");
  mTrackEta->GetYaxis()->SetTitle("Entry");

  mTrackThetaI = std::make_shared<TH1F>("TrackThetaI", "Track ThetaI", 120, 0., 12.);
  getObjectsManager()->startPublishing(mTrackThetaI.get());
  mTrackThetaI->GetXaxis()->SetTitle("Track ThetaI");
  mTrackThetaI->GetYaxis()->SetTitle("Entry");

  mTrackEtaI = std::make_shared<TH1F>("TrackEtaI", "Track EtaI", 250, 2., 4.5);
  getObjectsManager()->startPublishing(mTrackEtaI.get());
  mTrackEtaI->GetXaxis()->SetTitle("Track EtaI");
  mTrackEtaI->GetYaxis()->SetTitle("Entry");

  mTrackAlpha = std::make_shared<TH1F>("TrackAlpha", "Track Alpha", 240, -12., 12.);
  getObjectsManager()->startPublishing(mTrackAlpha.get());
  mTrackAlpha->GetXaxis()->SetTitle("Track Alpha");
  mTrackAlpha->GetYaxis()->SetTitle("Entry");

  mTrackPhi = std::make_shared<TH1F>("TrackPhi", "Track Phi", 360, -180., 180.);
  getObjectsManager()->startPublishing(mTrackPhi.get());
  mTrackPhi->GetXaxis()->SetTitle("Track Phi");
  mTrackPhi->GetYaxis()->SetTitle("Entry");

  mTrackThetaD = std::make_shared<TH1F>("TrackThetaD", "Track Theta Deviation", 200, -50., 50.);
  getObjectsManager()->startPublishing(mTrackThetaD.get());
  mTrackThetaD->GetXaxis()->SetTitle("Track Theta Dev");
  mTrackThetaD->GetYaxis()->SetTitle("Entry");

  mTrackPT = std::make_shared<TH1F>("TrackPT", "Track pT", 200, 0., 200.);
  getObjectsManager()->startPublishing(mTrackPT.get());
  mTrackPT->GetXaxis()->SetTitle("Track pT");
  mTrackPT->GetYaxis()->SetTitle("Entry");

  mTrackRatio44 = std::make_shared<TProfile>("TrackRatio44", "Track 44/all ", 9, 0., 9.);
  // mTrackRatio44 = std::make_shared<TH1F>("TrackRatio44", "Track 44/all ", 9, 0., 9.);
  getObjectsManager()->startPublishing(mTrackRatio44.get());
  mTrackRatio44->GetXaxis()->SetBinLabel(1, "Global");
  mTrackRatio44->GetXaxis()->SetBinLabel(2, "MT11 Bend");
  mTrackRatio44->GetXaxis()->SetBinLabel(3, "MT12 Bend");
  mTrackRatio44->GetXaxis()->SetBinLabel(4, "MT21 Bend");
  mTrackRatio44->GetXaxis()->SetBinLabel(5, "MT22 Bend");
  mTrackRatio44->GetXaxis()->SetBinLabel(6, "MT11 NBend");
  mTrackRatio44->GetXaxis()->SetBinLabel(7, "MT12 NBend");
  mTrackRatio44->GetXaxis()->SetBinLabel(8, "MT21 NBend");
  mTrackRatio44->GetXaxis()->SetBinLabel(9, "MT22 NBend");
  mTrackRatio44->GetYaxis()->SetTitle("Track 44/all");
  mTrackRatio44->SetMinimum(0.);
  mTrackRatio44->SetMaximum(1.1);
  mTrackRatio44->SetStats(0);

  mTrackBDetRatio44 = std::make_shared<TProfile>("TrackBDetRatio44", "Bend Track 44/all vs DetId", MID_NDE, 0., MID_NDE);
  mTrackBDetRatio44->GetXaxis()->SetTitle("DetId");
  mTrackBDetRatio44->GetYaxis()->SetTitle("Track 44/all");
  mTrackBDetRatio44->SetMinimum(0.);
  mTrackBDetRatio44->SetMaximum(1.1);
  getObjectsManager()->startPublishing(mTrackBDetRatio44.get());
  mTrackBDetRatio44->SetStats(0);

  mTrackNBDetRatio44 = std::make_shared<TProfile>("TrackNBDetRatio44", "Non-Bend Track 44/all vs DetId", MID_NDE, 0., MID_NDE);
  mTrackNBDetRatio44->GetXaxis()->SetTitle("DetId");
  mTrackNBDetRatio44->GetYaxis()->SetTitle("Track 44/all");
  mTrackNBDetRatio44->SetMinimum(0.);
  mTrackNBDetRatio44->SetMaximum(1.1);
  getObjectsManager()->startPublishing(mTrackNBDetRatio44.get());
  mTrackNBDetRatio44->SetStats(0);

  Double_t xEdges[4] = { -7., -0.2, 0.2, 7. };
  Double_t yEdges[10] = { 0., 1., 2., 3., 4., 5., 6., 7., 8., 9. };

  mTrackDetRatio44Map11 = std::make_shared<TProfile2D>("TrackDetRatio44Map11", "MT11 Detector Track 44/all Map", 3, xEdges, 9, yEdges);
  getObjectsManager()->startPublishing(mTrackDetRatio44Map11.get());
  mTrackDetRatio44Map11->GetXaxis()->SetTitle("Column");
  mTrackDetRatio44Map11->GetYaxis()->SetTitle("Line");
  mTrackDetRatio44Map11->SetMinimum(0.);
  mTrackDetRatio44Map11->SetMaximum(1.1);
  mTrackDetRatio44Map11->SetOption("colz");
  mTrackDetRatio44Map11->SetStats(0);

  mTrackDetRatio44Map12 = std::make_shared<TProfile2D>("TrackDetRatio44Map12", "MT12 Detector Track 44/all Map", 3, xEdges, 9, yEdges);
  getObjectsManager()->startPublishing(mTrackDetRatio44Map12.get());
  mTrackDetRatio44Map12->GetXaxis()->SetTitle("Column");
  mTrackDetRatio44Map12->GetYaxis()->SetTitle("Line");
  mTrackDetRatio44Map12->SetMinimum(0.);
  mTrackDetRatio44Map12->SetMaximum(1.1);
  mTrackDetRatio44Map12->SetOption("colz");
  mTrackDetRatio44Map12->SetStats(0);

  mTrackDetRatio44Map21 = std::make_shared<TProfile2D>("TrackDetRatio44Map21", "MT21 Detector Track 44/all Map", 3, xEdges, 9, yEdges);
  getObjectsManager()->startPublishing(mTrackDetRatio44Map21.get());
  mTrackDetRatio44Map21->GetXaxis()->SetTitle("Column");
  mTrackDetRatio44Map21->GetYaxis()->SetTitle("Line");
  mTrackDetRatio44Map21->SetMinimum(0.);
  mTrackDetRatio44Map21->SetMaximum(1.1);
  mTrackDetRatio44Map21->SetOption("colz");
  mTrackDetRatio44Map21->SetStats(0);

  mTrackDetRatio44Map22 = std::make_shared<TProfile2D>("TrackDetRatio44Map22", "MT22 Detector Track 44/all Map", 3, xEdges, 9, yEdges);
  getObjectsManager()->startPublishing(mTrackDetRatio44Map22.get());
  mTrackDetRatio44Map22->GetXaxis()->SetTitle("Column");
  mTrackDetRatio44Map22->GetYaxis()->SetTitle("Line");
  mTrackDetRatio44Map22->SetMinimum(0.);
  mTrackDetRatio44Map22->SetMaximum(1.1);
  mTrackDetRatio44Map22->SetOption("colz");
  mTrackDetRatio44Map22->SetStats(0);

  mTrackLocRatio44 = std::make_shared<TProfile>("TrackLocRatio44", " Track 44/all vs LocId", MID_NLOC, 0., MID_NLOC);
  mTrackLocRatio44->GetXaxis()->SetTitle("LocId");
  mTrackLocRatio44->GetYaxis()->SetTitle("Track 44/all");
  mTrackLocRatio44->SetMinimum(0.);
  mTrackLocRatio44->SetMaximum(1.1);
  getObjectsManager()->startPublishing(mTrackLocRatio44.get());
  mTrackLocRatio44->SetStats(0);

  mTrackBLocRatio44 = std::make_shared<TProfile>("TrackBLocRatio44", "Bend Track 44/all vs LocId", MID_NLOC, 0., MID_NLOC);
  mTrackBLocRatio44->GetXaxis()->SetTitle("LocId");
  mTrackBLocRatio44->GetYaxis()->SetTitle("Track 44/all");
  mTrackBLocRatio44->SetMinimum(0.);
  mTrackBLocRatio44->SetMaximum(1.1);
  getObjectsManager()->startPublishing(mTrackBLocRatio44.get());
  mTrackBLocRatio44->SetStats(0);

  mTrackNBLocRatio44 = std::make_shared<TProfile>("TrackNBLocRatio44", "Non-Bend Track 44/all vs LocId", MID_NLOC, 0., MID_NLOC);
  mTrackNBLocRatio44->GetXaxis()->SetTitle("LocId");
  mTrackNBLocRatio44->GetYaxis()->SetTitle("Track 44/all");
  mTrackNBLocRatio44->SetMinimum(0.);
  mTrackNBLocRatio44->SetMaximum(1.1);
  getObjectsManager()->startPublishing(mTrackNBLocRatio44.get());
  mTrackNBLocRatio44->SetStats(0);

  mTrackLocalBoardsRatio44Map = std::make_shared<TProfile2D>("TrackLocalBoardsRatio44Map", "Local boards Track 44/all Map", 14, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mTrackLocalBoardsRatio44Map.get());
  mTrackLocalBoardsRatio44Map->GetXaxis()->SetTitle("Column");
  mTrackLocalBoardsRatio44Map->GetYaxis()->SetTitle("Line");
  mTrackLocalBoardsRatio44Map->SetMinimum(0.);
  mTrackLocalBoardsRatio44Map->SetMaximum(1.1);
  mTrackLocalBoardsRatio44Map->SetOption("colz");
  mTrackLocalBoardsRatio44Map->SetStats(0);

  mTrackLocalBoardsBRatio44Map = std::make_shared<TProfile2D>("TrackLocalBoardsBRatio44Map", "Local boards Bend Track 44/all Map", 14, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mTrackLocalBoardsBRatio44Map.get());
  mTrackLocalBoardsBRatio44Map->GetXaxis()->SetTitle("Column");
  mTrackLocalBoardsBRatio44Map->GetYaxis()->SetTitle("Line");
  mTrackLocalBoardsBRatio44Map->SetMinimum(0.);
  mTrackLocalBoardsBRatio44Map->SetMaximum(1.1);
  mTrackLocalBoardsBRatio44Map->SetOption("colz");
  mTrackLocalBoardsBRatio44Map->SetStats(0);

  mTrackLocalBoardsNBRatio44Map = std::make_shared<TProfile2D>("TrackLocalBoardsNBRatio44Map", "Local boards Non-Bend Track 44/all Map", 14, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mTrackLocalBoardsNBRatio44Map.get());
  mTrackLocalBoardsNBRatio44Map->GetXaxis()->SetTitle("Column");
  mTrackLocalBoardsNBRatio44Map->GetYaxis()->SetTitle("Line");
  mTrackLocalBoardsNBRatio44Map->SetMinimum(0.);
  mTrackLocalBoardsNBRatio44Map->SetMaximum(1.1);
  mTrackLocalBoardsNBRatio44Map->SetOption("colz");
  mTrackLocalBoardsNBRatio44Map->SetStats(0);
}
void TracksQcTask::startOfActivity(const Activity& activity)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Info, Devel) << "startOfActivity " << activity.mId << ENDM;
  // printf(" =================== > test startOfActivity Tracks \n");
}

void TracksQcTask::startOfCycle()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  // ILOG(Info, Devel) << "startOfCycle" << ENDM;
  // printf(" =================== > test startOfCycle Tracks \n");
}

void TracksQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // ILOG(Info, Devel) << "monitorData" << ENDM;
  // printf(" =================== > test monitorData Tracks\n");

  auto tracks = ctx.inputs().get<gsl::span<o2::mid::Track>>("tracks");
  auto rofs = ctx.inputs().get<gsl::span<o2::mid::ROFRecord>>("trackrofs");

  mNbTracksTF->Fill(0.5, 1.);
  // auto tracks = o2::mid::specs::getData(ctx, "tracks", o2::mid::EventType::Standard);
  // auto rofs = o2::mid::specs::getRofs(ctx, "tracks", o2::mid::EventType::Standard);

  int multTracks;
  float DeltaZ = o2::mid::geoparams::DefaultChamberZ[0] - o2::mid::geoparams::DefaultChamberZ[3];
  float Zf = -975.; // Zf= position mid-dipole
  float p0 = 3.;    // T.m for charge = 1
  float theta, eta;
  float thetaI, etaI;
  float alpha, phi;
  float thetaD, pT;

  for (const auto& rofRecord : rofs) { // loop ROFRecords == Events //
    // printf("========================================================== \n");
    // printf("Tracks :: %05d ROF with first entry %05zu and nentries %02zu , BC %05d, ORB %05d , EventType %02d\n", nROF, rofRecord.firstEntry, rofRecord.nEntries, rofRecord.interactionRecord.bc, rofRecord.interactionRecord.orbit,rofRecord.eventType);
    mROF++;
    multTracks = 0;
    mTrackBCCounts->Fill(rofRecord.interactionRecord.bc, rofRecord.nEntries);

    // if(rofRecord.nEntries!=1)continue;/// TEST ONE TACKS ONLY !!!

    for (auto& track : tracks.subspan(rofRecord.firstEntry, rofRecord.nEntries)) { // loop Tracks in ROF//
      multTracks += 1;
      auto isRightSide = o2::mid::detparams::isRightSide(track.getFiredDEId());
      int deIndex = track.getFiredDEId();
      int rpcLine = o2::mid::detparams::getRPCLine(track.getFiredDEId());
      int colId = track.getFiredColumnId();
      int lineId = track.getFiredLineId();

      mTrackMapXY->Fill(track.getPositionX(), track.getPositionY(), 1);
      mTrackDevX->Fill(track.getDirectionX() * DeltaZ);
      mTrackDevY->Fill(track.getDirectionY() * DeltaZ);
      mTrackDevXY->Fill(track.getDirectionX() * DeltaZ, track.getDirectionY() * DeltaZ, 1);
      theta = TMath::ATan(TMath::Sqrt(track.getPositionX() * track.getPositionX() + track.getPositionY() * track.getPositionY()) / TMath::Abs(track.getPositionZ()));
      eta = -log(TMath::Tan(theta / 2));
      mTrackEta->Fill(eta);
      theta = theta * TMath::RadToDeg();
      mTrackTheta->Fill(theta);
      alpha = TMath::ATan(track.getPositionY() / TMath::Abs(track.getPositionZ())) * TMath::RadToDeg();
      mTrackAlpha->Fill(alpha);
      phi = TMath::ATan2(track.getPositionY(), track.getPositionX()) * TMath::RadToDeg();
      mTrackPhi->Fill(phi);

      // Propagate the track position parameters to MT11, MT21 and Zf /// VERIFIER !!!!
      float dZ1 = o2::mid::geoparams::DefaultChamberZ[0] - track.getPositionZ();
      float X1 = track.getPositionX() + track.getDirectionX() * dZ1;
      float Y1 = track.getPositionY() + track.getDirectionY() * dZ1;

      float dZ2 = o2::mid::geoparams::DefaultChamberZ[2] - track.getPositionZ();
      float X2 = track.getPositionX() + track.getDirectionX() * dZ2;
      float Y2 = track.getPositionY() + track.getDirectionY() * dZ2;

      float dZf = Zf - track.getPositionZ();
      float Xf = X1 * Zf / o2::mid::geoparams::DefaultChamberZ[0];
      float Yf = Y2 - (Y2 - Y1) * (o2::mid::geoparams::DefaultChamberZ[2] - Zf) / (o2::mid::geoparams::DefaultChamberZ[2] - o2::mid::geoparams::DefaultChamberZ[0]);
      // printf("\n X =%02f ; Y =%02f ;  Z =%02f ;  dX =%02f ;  dY =%02f ; \n",track.getPositionX(),track.getPositionY(),track.getPositionZ(),track.getDirectionX(),track.getDirectionY());
      // printf(" X1 =%02f ; Y1 =%02f ;  Z1 =%02f ;  \n",X1,Y1, o2::mid::geoparams::DefaultChamberZ[0]);
      // printf(" X2 =%02f ; Y2 =%02f ;  Z2 =%02f ;  \n",X2,Y2, o2::mid::geoparams::DefaultChamberZ[2]);
      // printf(" Xf =%02f ; Yf =%02f ;  Zf =%02f ;  \n",Xf,Yf,Zf);

      thetaI = TMath::ATan(TMath::Sqrt(Xf * Xf + Yf * Yf) / TMath::Abs(Zf));
      etaI = -log(TMath::Tan(thetaI / 2));
      mTrackEtaI->Fill(etaI);
      mTrackThetaI->Fill(thetaI * TMath::RadToDeg());

      thetaD = (o2::mid::geoparams::DefaultChamberZ[0] * Y2 - o2::mid::geoparams::DefaultChamberZ[2] * Y1) / ((o2::mid::geoparams::DefaultChamberZ[2] - o2::mid::geoparams::DefaultChamberZ[0]) * Zf);
      mTrackThetaD->Fill(thetaD * TMath::RadToDeg());
      pT = TMath::Abs(p0 / thetaD) * (TMath::Sqrt(Xf * Xf + Yf * Yf) / TMath::Abs(Zf));
      // printf("==========================> thetaD = %05f ; pT =  %05f \n",thetaD,pT);
      mTrackPT->Fill(pT);

      /// Efficiency part
      uint32_t EffWord = track.getEfficiencyWord();
      int EffFlag = track.getEfficiencyFlag();
      uint8_t HitMap = track.getHitMap();
      float BDetEff = 0.;
      float NBDetEff = 0.;
      float DetEff = 0.;
      if (EffFlag > 0) {
        multTracksTot++;
        if (HitMap == 0xFF)
          mTrackRatio44->Fill(0.5, 1.); // multTracks44Tot++;
        else
          mTrackRatio44->Fill(0.5, 0.);

        // track.isFiredChamber(i,j) :: i=0->3 (MT11->MT22) ; j=0->1 (BP->NBP)
        uint8_t HitMapB = HitMap & 0xF;
        if (track.isFiredChamber(0, 0))
          mTrackRatio44->Fill(1.5, 1.);
        else
          mTrackRatio44->Fill(1.5, 0.);
        if (track.isFiredChamber(1, 0))
          mTrackRatio44->Fill(2.5, 1.);
        else
          mTrackRatio44->Fill(2.5, 0.);
        if (track.isFiredChamber(2, 0))
          mTrackRatio44->Fill(3.5, 1.);
        else
          mTrackRatio44->Fill(3.5, 0.);
        if (track.isFiredChamber(3, 0))
          mTrackRatio44->Fill(4.5, 1.);
        else
          mTrackRatio44->Fill(4.5, 0.);

        uint8_t HitMapNB = (HitMap >> 4) & 0xF;
        if (track.isFiredChamber(0, 1))
          mTrackRatio44->Fill(5.5, 1.);
        else
          mTrackRatio44->Fill(5.5, 0.);
        if (track.isFiredChamber(1, 1))
          mTrackRatio44->Fill(6.5, 1.);
        else
          mTrackRatio44->Fill(6.5, 0.);
        if (track.isFiredChamber(2, 1))
          mTrackRatio44->Fill(7.5, 1.);
        else
          mTrackRatio44->Fill(7.5, 0.);
        if (track.isFiredChamber(3, 1))
          mTrackRatio44->Fill(8.5, 1.);
        else
          mTrackRatio44->Fill(8.5, 0.);

        if (EffFlag > 1) { // RPCeff
          int DetId0 = track.getFiredDEId();
          int chamb = o2::mid::detparams::getChamber(track.getFiredDEId());

          if (chamb == 1)
            DetId0 = DetId0 - 9; // if MT11 not fired

          // if (DetId0==0) printf("************  DetId0 = %i, chamb = %i , HitMap = %X   \n",DetId0,chamb,HitMap);
          for (int i = 0; i < 4; i++) {

            if (track.isFiredChamber(i, 0))
              mTrackBDetRatio44->Fill(DetId0 + 9 * i, 1.); // fired
            else
              mTrackBDetRatio44->Fill(DetId0 + 9 * i, 0.);

            if (track.isFiredChamber(i, 1))
              mTrackNBDetRatio44->SetBinContent(DetId0 + 9 * i, 1.); // fired
            else
              mTrackNBDetRatio44->Fill(DetId0 + 9 * i, 0.);

            if (i == 0) { // MT11
              if (isRightSide == 0) {
                if (track.isFiredChamber(i, 0))
                  mTrackDetRatio44Map11->Fill(-1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map11->Fill(-1, rpcLine, 0.);
                if (track.isFiredChamber(i, 1))
                  mTrackDetRatio44Map11->Fill(-1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map11->Fill(-1, rpcLine, 0.);
              } else { // LeftSide
                if (track.isFiredChamber(i, 0))
                  mTrackDetRatio44Map11->Fill(1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map11->Fill(1, rpcLine, 0.);
                if (track.isFiredChamber(i, 1))
                  mTrackDetRatio44Map11->Fill(1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map11->Fill(1, rpcLine, 0.);
              }
            } else if (i == 1) { // MT12
              if (isRightSide == 0) {
                if (track.isFiredChamber(i, 0))
                  mTrackDetRatio44Map12->Fill(-1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map12->Fill(-1, rpcLine, 0.);
                if (track.isFiredChamber(i, 1))
                  mTrackDetRatio44Map12->Fill(-1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map12->Fill(-1, rpcLine, 0.);
              } else { // LeftSide
                if (track.isFiredChamber(i, 0))
                  mTrackDetRatio44Map12->Fill(1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map12->Fill(1, rpcLine, 0.);
                if (track.isFiredChamber(i, 1))
                  mTrackDetRatio44Map12->Fill(1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map12->Fill(1, rpcLine, 0.);
              }
            } else if (i == 2) { // MT21
              if (isRightSide == 0) {
                if (track.isFiredChamber(i, 0))
                  mTrackDetRatio44Map21->Fill(-1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map21->Fill(-1, rpcLine, 0.);
                if (track.isFiredChamber(i, 1))
                  mTrackDetRatio44Map21->Fill(-1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map21->Fill(-1, rpcLine, 0.);
              } else { // LeftSide
                if (track.isFiredChamber(i, 0))
                  mTrackDetRatio44Map21->Fill(1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map21->Fill(1, rpcLine, 0.);
                if (track.isFiredChamber(i, 1))
                  mTrackDetRatio44Map21->Fill(1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map21->Fill(1, rpcLine, 0.);
              }
            } else if (i == 3) { // MT22
              if (isRightSide == 0) {
                if (track.isFiredChamber(i, 0))
                  mTrackDetRatio44Map22->Fill(-1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map22->Fill(-1, rpcLine, 0.);
                if (track.isFiredChamber(i, 1))
                  mTrackDetRatio44Map22->Fill(-1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map22->Fill(-1, rpcLine, 0.);
              } else { // LeftSide
                if (track.isFiredChamber(i, 0))
                  mTrackDetRatio44Map22->Fill(1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map22->Fill(1, rpcLine, 0.);
                if (track.isFiredChamber(i, 1))
                  mTrackDetRatio44Map22->Fill(1, rpcLine, 1.); // Fired
                else
                  mTrackDetRatio44Map22->Fill(1, rpcLine, 0.);
              }
            }
          }

          if (EffFlag > 2) { // LocBoardeff
            auto localBoard = mMapping.getBoardId(lineId, colId, deIndex);

            int nZoneHistoX = 1;
            if (mMapping.getLastBoardBP(colId, deIndex) == 1)
              nZoneHistoX = 2;
            else if (mMapping.getLastBoardBP(colId, deIndex) == 0)
              nZoneHistoX = 4;

            for (int board = mMapping.getFirstBoardBP(colId, deIndex), lastBoard = mMapping.getLastBoardBP(colId, deIndex); board <= lastBoard; board++) {
              // These are the existing bend boards for this column Id (board = n°board in the column) + 1 (for non-bend)
              int Fired = 0;
              int BFired = 0;
              int NBFired = 0;
              if (board == lineId) {
                // if (localBoard < 10)printf(" Loc %i ====> Fired ; col %i, rpcLine %i nZoneX %i\n", mMapping.getBoardId(board, colId, deIndex), colId, rpcLine, nZoneHistoX);
                if (HitMap == 0xFF)
                  Fired = 1;
                if (HitMapB == 0xF)
                  BFired = 1;
                if (HitMapNB == 0xF)
                  NBFired = 1;
              }

              double linePos0 = rpcLine;
              if (localBoard == mMapping.getBoardId(board, colId, deIndex)) { // only fire board in the line
                mTrackLocRatio44->Fill(localBoard - 1, Fired);
                mTrackBLocRatio44->Fill(localBoard - 1, BFired);
                mTrackNBLocRatio44->Fill(localBoard - 1, NBFired);
                linePos0 = rpcLine + 0.25 * board;
                if ((nZoneHistoX == 2) && (board == 1))
                  linePos0 += 0.25;
                for (int ib = 0; ib < nZoneHistoX; ib++) {
                  double linePos = linePos0 + (0.25 * ib);
                  if (isRightSide == 1) {
                    mTrackLocalBoardsRatio44Map->Fill(colId + 0.5, linePos, Fired);
                    mTrackLocalBoardsBRatio44Map->Fill(colId + 0.5, linePos, BFired);
                    mTrackLocalBoardsNBRatio44Map->Fill(colId + 0.5, linePos, NBFired);
                  } else {
                    mTrackLocalBoardsRatio44Map->Fill(-colId - 0.5, linePos, Fired);
                    mTrackLocalBoardsBRatio44Map->Fill(-colId - 0.5, linePos, BFired);
                    mTrackLocalBoardsNBRatio44Map->Fill(-colId - 0.5, linePos, NBFired);
                  }
                } // board in line loop
              }   // board loop
            }     // only fire board in the line
          }       //(EffFlag>2)
        }         //(EffFlag>1)
      }           // Efficiency part (EffFlag>0)
    }             // tracks in ROF
    mMultTracks->Fill(multTracks);

  } //  ROFRecords //
}

void TracksQcTask::endOfCycle()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  // ILOG(Info, Devel) << "endOfCycle" << ENDM;
  // printf(" =================== > test endOfCycle Tracks \n");
}

void TracksQcTask::endOfActivity(const Activity& /*activity*/)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Info, Devel) << "endOfActivity" << ENDM;
  // printf(" =================== > test endOfActivity Tracks \n");
}

void TracksQcTask::reset()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.

  // clean all the monitor objects here

  ILOG(Info, Devel) << "Resetting the histogram" << ENDM;
  // printf(" =================== > test reset Tracks \n");

  mNbTracksTF->Reset();
  mTrackMapXY->Reset();
  mTrackDevX->Reset();
  mTrackDevY->Reset();
  mTrackDevXY->Reset();
  mTrackTheta->Reset();
  mTrackEta->Reset();
  mTrackThetaI->Reset();
  mTrackEtaI->Reset();
  mTrackAlpha->Reset();
  mTrackPhi->Reset();
  mTrackThetaD->Reset();
  mTrackPT->Reset();
  mTrackRatio44->Reset();
  mTrackBDetRatio44->Reset();
  mTrackNBDetRatio44->Reset();
  mTrackLocRatio44->Reset();
  mTrackBLocRatio44->Reset();
  mTrackNBLocRatio44->Reset();
  mTrackLocalBoardsRatio44Map->Reset();
  mTrackLocalBoardsBRatio44Map->Reset();
  mTrackLocalBoardsNBRatio44Map->Reset();

  mTrackDetRatio44Map11->Reset();
  mTrackDetRatio44Map12->Reset();
  mTrackDetRatio44Map21->Reset();
  mTrackDetRatio44Map22->Reset();

  mTrackBCCounts->Reset();
}

} // namespace o2::quality_control_modules::mid
