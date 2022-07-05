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
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>

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
  ILOG(Info, Support) << "initialize TracksQcTask" << ENDM;

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

  for (int i = 0; i <= MID_NDE; i++)
    DetTracks[i] = { 0, 0, 0, 0 };
  for (int i = 0; i <= MID_NLOC; i++)
    LocTracks[i] = { 0, 0, 0, 0 };

  for (int deId = 0; deId <= MID_NDE; deId++) {
    for (int locId = 0; locId <= MID_NLOC; locId++) {
      for (int icol = mMapping.getFirstColumn(deId); icol < 7; ++icol) {
        for (int ib = mMapping.getFirstBoardBP(icol, deId); ib <= mMapping.getLastBoardBP(icol, deId); ++ib) {
          if (mMapping.getBoardId(ib, icol, deId) == locId) {
            LocColMap[locId] = icol;
            break;
          }
        }
      }
    }
  }

  // mTrackBCCounts = std::make_shared<TH1F>("TrackBCCounts", "Tracks Bunch Crossing Counts;BC;Entry (tracks nb)", o2::constants::lhc::LHCMaxBunches, 0., o2::constants::lhc::LHCMaxBunches);
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
  mTrackRatio44->SetStats(0);

  mTrackBDetRatio44 = std::make_shared<TH1F>("TrackBDetRatio44", "Bend Track 44/all vs DetId", MID_NDE, 0., MID_NDE);
  mTrackBDetRatio44->GetXaxis()->SetTitle("DetId");
  mTrackBDetRatio44->GetYaxis()->SetTitle("Track 44/all");
  getObjectsManager()->startPublishing(mTrackBDetRatio44.get());
  mTrackBDetRatio44->SetStats(0);

  mTrackNBDetRatio44 = std::make_shared<TH1F>("TrackNBDetRatio44", "Non-Bend Track 44/all vs DetId", MID_NDE, 0., MID_NDE);
  mTrackNBDetRatio44->GetXaxis()->SetTitle("DetId");
  mTrackNBDetRatio44->GetYaxis()->SetTitle("Track 44/all");
  getObjectsManager()->startPublishing(mTrackNBDetRatio44.get());
  mTrackNBDetRatio44->SetStats(0);

  Double_t xEdges[4] = { -7., -0.2, 0.2, 7. };
  Double_t yEdges[10] = { 0., 1., 2., 3., 4., 5., 6., 7., 8., 9. };
  mTrackDetRatio44Map11 = std::make_shared<TH2F>("TrackDetRatio44Map11", "MT11 Detector Track 44/all Map", 3, xEdges, 9, yEdges);
  getObjectsManager()->startPublishing(mTrackDetRatio44Map11.get());
  mTrackDetRatio44Map11->GetXaxis()->SetTitle("Column");
  mTrackDetRatio44Map11->GetYaxis()->SetTitle("Line");
  mTrackDetRatio44Map11->SetOption("colz");
  mTrackDetRatio44Map11->SetStats(0);

  mTrackDetRatio44Map12 = std::make_shared<TH2F>("TrackDetRatio44Map12", "MT12 Detector Track 44/all Map", 3, xEdges, 9, yEdges);
  getObjectsManager()->startPublishing(mTrackDetRatio44Map12.get());
  mTrackDetRatio44Map12->GetXaxis()->SetTitle("Column");
  mTrackDetRatio44Map12->GetYaxis()->SetTitle("Line");
  mTrackDetRatio44Map12->SetOption("colz");
  mTrackDetRatio44Map12->SetStats(0);

  mTrackDetRatio44Map21 = std::make_shared<TH2F>("TrackDetRatio44Map21", "MT21 Detector Track 44/all Map", 3, xEdges, 9, yEdges);
  getObjectsManager()->startPublishing(mTrackDetRatio44Map21.get());
  mTrackDetRatio44Map21->GetXaxis()->SetTitle("Column");
  mTrackDetRatio44Map21->GetYaxis()->SetTitle("Line");
  mTrackDetRatio44Map21->SetOption("colz");
  mTrackDetRatio44Map21->SetStats(0);

  mTrackDetRatio44Map22 = std::make_shared<TH2F>("TrackDetRatio44Map22", "MT22 Detector Track 44/all Map", 3, xEdges, 9, yEdges);
  getObjectsManager()->startPublishing(mTrackDetRatio44Map22.get());
  mTrackDetRatio44Map22->GetXaxis()->SetTitle("Column");
  mTrackDetRatio44Map22->GetYaxis()->SetTitle("Line");
  mTrackDetRatio44Map22->SetOption("colz");
  mTrackDetRatio44Map22->SetStats(0);

  mTrackBLocRatio44 = std::make_shared<TH1F>("TrackBLocRatio44", "Bend Track 44/all vs LocId", MID_NLOC, 0., MID_NLOC);
  mTrackBLocRatio44->GetXaxis()->SetTitle("LocId");
  mTrackBLocRatio44->GetYaxis()->SetTitle("Track 44/all");
  getObjectsManager()->startPublishing(mTrackBLocRatio44.get());
  mTrackBLocRatio44->SetStats(0);

  mTrackNBLocRatio44 = std::make_shared<TH1F>("TrackNBLocRatio44", "Non-Bend Track 44/all vs LocId", MID_NLOC, 0., MID_NLOC);
  mTrackNBLocRatio44->GetXaxis()->SetTitle("DetId");
  mTrackNBLocRatio44->GetYaxis()->SetTitle("Track 44/all");
  getObjectsManager()->startPublishing(mTrackNBLocRatio44.get());
  mTrackNBLocRatio44->SetStats(0);

  mTrackLocalBoardsRatio44Map = std::make_shared<TH2F>("TrackLocalBoardsRatio44Map", "Local boards Track 44/all Map", 14, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mTrackLocalBoardsRatio44Map.get());
  mTrackLocalBoardsRatio44Map->GetXaxis()->SetTitle("Column");
  mTrackLocalBoardsRatio44Map->GetYaxis()->SetTitle("Line");
  mTrackLocalBoardsRatio44Map->SetOption("colz");
  mTrackLocalBoardsRatio44Map->SetStats(0);

  mTrackLocalBoardsBRatio44Map = std::make_shared<TH2F>("TrackLocalBoardsBRatio44Map", "Local boards Bend Track 44/all Map", 14, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mTrackLocalBoardsBRatio44Map.get());
  mTrackLocalBoardsBRatio44Map->GetXaxis()->SetTitle("Column");
  mTrackLocalBoardsBRatio44Map->GetYaxis()->SetTitle("Line");
  mTrackLocalBoardsBRatio44Map->SetOption("colz");
  mTrackLocalBoardsBRatio44Map->SetStats(0);

  mTrackLocalBoardsNBRatio44Map = std::make_shared<TH2F>("TrackLocalBoardsNBRatio44Map", "Local boards Non-Bend Track 44/all Map", 14, -7, 7, 36, 0, 9);
  getObjectsManager()->startPublishing(mTrackLocalBoardsNBRatio44Map.get());
  mTrackLocalBoardsNBRatio44Map->GetXaxis()->SetTitle("Column");
  mTrackLocalBoardsNBRatio44Map->GetYaxis()->SetTitle("Line");
  mTrackLocalBoardsNBRatio44Map->SetOption("colz");
  mTrackLocalBoardsNBRatio44Map->SetStats(0);
}
void TracksQcTask::startOfActivity(Activity& activity)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
}

void TracksQcTask::startOfCycle()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void TracksQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto tracks = ctx.inputs().get<gsl::span<o2::mid::Track>>("tracks");
  auto rofs = ctx.inputs().get<gsl::span<o2::mid::ROFRecord>>("trackrofs");

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
    nROF++;
    multTracks = 0;
    mTrackBCCounts->Fill(rofRecord.interactionRecord.bc, rofRecord.nEntries);

    // if(rofRecord.nEntries!=1)continue;/// TEST ONE TACKS ONLY !!!

    for (auto& track : tracks.subspan(rofRecord.firstEntry, rofRecord.nEntries)) { // loop Tracks in ROF//
      multTracks += 1;
      auto isRightSide = o2::mid::detparams::isRightSide(track.getFiredDeId());
      int deIndex = track.getFiredDeId();
      int rpcLine = o2::mid::detparams::getRPCLine(track.getFiredDeId());
      int colId = LocColMap[track.getFiredLocalBoard()];

      // std::cout << "  =>>  " << track << std::endl;
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
      if (EffFlag > 0) {
        if (HitMap == 0xFF)
          multTracks44Tot++;
        multTracksTot++;
        uint8_t HitMapB = HitMap & 0xF;
        if (HitMapB == 0xF)
          multTracksBend44++;
        uint8_t HitMapNB = (HitMap >> 4) & 0xF;
        if (HitMapNB == 0xF)
          multTracksNBend44++;

        // track.isFiredChamber(i,j) :: i=0->3 (MT11->MT22) ; j=0->1 (BP->NBP)
        if (track.isFiredChamber(0, 0))
          multTraksB34MT11++;
        else if (track.isFiredChamber(1, 0))
          multTraksB34MT12++;
        else if (track.isFiredChamber(2, 0))
          multTraksB34MT21++;
        else if (track.isFiredChamber(3, 0))
          multTraksB34MT22++;
        if (track.isFiredChamber(0, 1))
          multTraksNB34MT11++;
        else if (track.isFiredChamber(1, 1))
          multTraksNB34MT12++;
        else if (track.isFiredChamber(2, 1))
          multTraksNB34MT21++;
        else if (track.isFiredChamber(3, 1))
          multTraksNB34MT22++;

        if (EffFlag > 1) { // RPCeff
          int DetId0 = track.getFiredDeId();
          int chamb = o2::mid::detparams::getChamber(track.getFiredDeId());
          // if (HitMap != 0xFF) printf("DetId0 = %i, chamb = %i , HitMap = %X   \n",DetId0,chamb,HitMap);

          if (chamb == 1)
            DetId0 = DetId0 - 9; // if MT11 not fired

          int mask = 1;
          for (int i = 0; i < 4; i++) {
            auto& DetTrack = DetTracks[DetId0 + 9 * i]; // DetTrack: {Bnb34,Bnb44,NBnb34,NBnb44} //vect
            if ((HitMapB & mask) != 0)
              DetTrack[1]++; // 4 planes
            else
              DetTrack[0]++;
            if ((HitMapNB & mask) != 0)
              DetTrack[3]++; // 4 planes
            else
              DetTrack[2]++;

            // if ((i==0)&&(DetId0<10))printf("\n mask = %X , HitMapB = %X :: Bnb34 = %i, Bnb44 = %i \n",mask,HitMapB,DetTrack[0],DetTrack[1]);

            mask <<= 1;
            DetTracks[DetId0 + 9 * i] = DetTrack; // map of vect
            if ((DetTrack[0] + DetTrack[1]) > 0)
              BDetEff = float(DetTrack[1]) / float(DetTrack[0] + DetTrack[1]);
            // if ((i==0)&&(DetId0<10))printf(" detId = %i ====>  BDetEff = %f \n",DetId0+9*i, BDetEff);

            if ((DetTrack[2] + DetTrack[3]) > 0)
              NBDetEff = float(DetTrack[3]) / float(DetTrack[2] + DetTrack[3]);

            mTrackBDetRatio44->SetBinContent(DetId0 + 9 * i, BDetEff);
            mTrackNBDetRatio44->SetBinContent(DetId0 + 9 * i, NBDetEff);

            if (i == 0) {
              if (isRightSide == 0)
                mTrackDetRatio44Map11->SetBinContent(1, rpcLine + 1, BDetEff * NBDetEff);
              else
                mTrackDetRatio44Map11->SetBinContent(3, rpcLine + 1, BDetEff * NBDetEff);
            } else if (i == 1) {
              if (isRightSide == 0)
                mTrackDetRatio44Map12->SetBinContent(1, rpcLine + 1, BDetEff * NBDetEff);
              else
                mTrackDetRatio44Map12->SetBinContent(3, rpcLine + 1, BDetEff * NBDetEff);
            } else if (i == 2) {
              if (isRightSide == 0)
                mTrackDetRatio44Map21->SetBinContent(1, rpcLine + 1, BDetEff * NBDetEff);
              else
                mTrackDetRatio44Map21->SetBinContent(3, rpcLine + 1, BDetEff * NBDetEff);
            } else if (i == 3) {
              if (isRightSide == 0)
                mTrackDetRatio44Map22->SetBinContent(1, rpcLine + 1, BDetEff * NBDetEff);
              else
                mTrackDetRatio44Map22->SetBinContent(3, rpcLine + 1, BDetEff * NBDetEff);
            }
          }

          if (EffFlag > 2) {                                        // LocBoardeff
            auto& LocTrack = LocTracks[track.getFiredLocalBoard()]; // DetTrack: {Bnb34,Bnb44,NBnb34,NBnb44} //vect
            if (HitMapB == 0xF)
              LocTrack[1]++;
            else
              LocTrack[0]++;
            if (HitMapNB == 0xF)
              LocTrack[3]++;
            else
              LocTrack[2]++;
            // LocTracks[track.getFiredLocalBoard()] = LocTrack; // map of vect

            float BLocEff = 0.;
            float NBLocEff = 0.;
            if ((LocTrack[0] + LocTrack[1]) > 0)
              BLocEff = float(LocTrack[1]) / float(LocTrack[0] + LocTrack[1]);
            if ((LocTrack[2] + LocTrack[3]) > 0)
              NBLocEff = float(LocTrack[3]) / float(LocTrack[2] + LocTrack[3]);
            mTrackBLocRatio44->SetBinContent(track.getFiredLocalBoard() + 1, BLocEff);
            mTrackNBLocRatio44->SetBinContent(track.getFiredLocalBoard() + 1, NBLocEff);

            //// Local Boards Display::

            int nZoneHistoX = 1;
            if (mMapping.getLastBoardBP(colId, deIndex) == 1)
              nZoneHistoX = 2;
            else if (mMapping.getLastBoardBP(colId, deIndex) == 0)
              nZoneHistoX = 4;

            for (int board = mMapping.getFirstBoardBP(colId, deIndex), lastBoard = mMapping.getLastBoardBP(colId, deIndex); board <= lastBoard; board++) {
              // These are the existing bend boards for this column Id (board = n°board in the column) + 1 (for non-bend)
              if (mMapping.getBoardId(board, colId, deIndex) == track.getFiredLocalBoard()) {
                double linePos0 = rpcLine;
                linePos0 = rpcLine + 0.25 * board;
                if ((nZoneHistoX == 2) && (board == 1))
                  linePos0 += 0.25;
                for (int ib = 0; ib < nZoneHistoX; ib++) {
                  double linePos = linePos0 + (0.25 * ib);
                  int lineBin = TMath::Floor(linePos * 36 / 9) + 1;
                  if (isRightSide) {
                    int colBin = colId + 8;
                    mTrackLocalBoardsRatio44Map->SetBinContent(colBin, lineBin, BLocEff * NBLocEff);
                    mTrackLocalBoardsBRatio44Map->SetBinContent(colBin, lineBin, BLocEff);
                    mTrackLocalBoardsNBRatio44Map->SetBinContent(colBin, lineBin, NBLocEff);
                  } else {
                    int colBin = -colId + 7;
                    mTrackLocalBoardsRatio44Map->SetBinContent(colBin, lineBin, BLocEff * NBLocEff);
                    mTrackLocalBoardsBRatio44Map->SetBinContent(colBin, lineBin, BLocEff);
                    mTrackLocalBoardsNBRatio44Map->SetBinContent(colBin, lineBin, NBLocEff);
                  }
                } // board in line loop
              }   // board fired
            }     // board loop
          }       //(EffFlag>2)
        }         //(EffFlag>1)
      }           // Efficiency part (EffFlag>0)
    }             // tracks in ROF
    mMultTracks->Fill(multTracks);

    /// Efficiency part
    if (multTracksTot > 0) {
      globEff = float(multTracks44Tot) / float(multTracksTot);
      // mTrackRatio44->SetBinContent(1, float(multTracks44Tot) / float(multTracksTot));
      mTrackRatio44->Fill(0., float(multTracks44Tot) / float(multTracksTot));
      globBendEff = float(multTracksBend44) / float(multTracksTot);
      globNBendEff = float(multTracksNBend44) / float(multTracksTot);
      if (multTracksBend44 > 0) {
        /*	mTrackRatio44->SetBinContent(2, float(multTracksBend44) / float(multTracksBend44 + multTraksB34MT11));
        mTrackRatio44->SetBinContent(3, float(multTracksBend44) / float(multTracksBend44 + multTraksB34MT12));
        mTrackRatio44->SetBinContent(4, float(multTracksBend44) / float(multTracksBend44 + multTraksB34MT21));
        mTrackRatio44->SetBinContent(5, float(multTracksBend44) / float(multTracksBend44 + multTraksB34MT22));
      */ mTrackRatio44->Fill(1, float(multTracksBend44) / float(multTracksBend44 + multTraksB34MT11));
        mTrackRatio44->Fill(2, float(multTracksBend44) / float(multTracksBend44 + multTraksB34MT12));
        mTrackRatio44->Fill(3, float(multTracksBend44) / float(multTracksBend44 + multTraksB34MT21));
        mTrackRatio44->Fill(4, float(multTracksBend44) / float(multTracksBend44 + multTraksB34MT22));
      }
      if (multTracksNBend44 > 0) {
        mTrackRatio44->Fill(5, float(multTracksNBend44) / float(multTracksNBend44 + multTraksNB34MT11));
        mTrackRatio44->Fill(6, float(multTracksNBend44) / float(multTracksNBend44 + multTraksNB34MT12));
        mTrackRatio44->Fill(7, float(multTracksNBend44) / float(multTracksNBend44 + multTraksNB34MT21));
        mTrackRatio44->Fill(8, float(multTracksNBend44) / float(multTracksNBend44 + multTraksNB34MT22));
  /*	mTrackRatio44->SetBinContent(6, float(multTracksNBend44) / float(multTracksNBend44 + multTraksNB34MT11));
	mTrackRatio44->SetBinContent(7, float(multTracksNBend44) / float(multTracksNBend44 + multTraksNB34MT12));
	mTrackRatio44->SetBinContent(8, float(multTracksNBend44) / float(multTracksNBend44 + multTraksNB34MT21));
	mTrackRatio44->SetBinContent(9, float(multTracksNBend44) / float(multTracksNBend44 + multTraksNB34MT22));
	*/      }
    }
  } //  ROFRecords //
}

void TracksQcTask::endOfCycle()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void TracksQcTask::endOfActivity(Activity& /*activity*/)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TracksQcTask::reset()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.

  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
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
