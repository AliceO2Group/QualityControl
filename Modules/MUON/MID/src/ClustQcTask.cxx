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
/// \file   ClustQcTask.cxx
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
#include "MID/ClustQcTask.h"
#include <Framework/InputRecord.h>
#include "Framework/DataRefUtils.h"
#include "DataFormatsMID/Cluster.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDBase/DetectorParameters.h"
#include "MIDBase/GeometryParameters.h"

#define MID_NDE 72
#define DZpos 10

namespace o2::quality_control_modules::mid
{

ClustQcTask::~ClustQcTask()
{
}

void ClustQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize ClusterQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  /////////////////

  mMultClust11 = std::make_shared<TH1F>("MultClust11", "Multiplicity Clusters - MT11 ", 100, 0, 100);
  getObjectsManager()->startPublishing(mMultClust11.get());
  mMultClust12 = std::make_shared<TH1F>("MultClust12", "Multiplicity Clusters - MT12 ", 100, 0, 100);
  getObjectsManager()->startPublishing(mMultClust12.get());
  mMultClust21 = std::make_shared<TH1F>("MultClust21", "Multiplicity Clusters - MT21 ", 100, 0, 100);
  getObjectsManager()->startPublishing(mMultClust21.get());
  mMultClust22 = std::make_shared<TH1F>("MultClust22", "Multiplicity Clusters - MT22 ", 100, 0, 100);
  getObjectsManager()->startPublishing(mMultClust22.get());

  mClusterMap11 = std::make_shared<TH2F>("ClusterMap11", "Cluster Map MT11", 300, -300., 300., 300, -300., 300.);
  getObjectsManager()->startPublishing(mClusterMap11.get());
  mClusterMap11->GetXaxis()->SetTitle("X Position (cm)");
  mClusterMap11->GetYaxis()->SetTitle("Y Position (cm)");
  mClusterMap11->SetOption("colz");
  mClusterMap11->SetStats(0);

  mClusterMap12 = std::make_shared<TH2F>("ClusterMap12", "Cluster Map MT12", 300, -300., 300., 300, -300., 300.);
  getObjectsManager()->startPublishing(mClusterMap12.get());
  mClusterMap12->GetXaxis()->SetTitle("X Position (cm)");
  mClusterMap12->GetYaxis()->SetTitle("Y Position (cm)");
  mClusterMap12->SetOption("colz");
  mClusterMap12->SetStats(0);

  mClusterMap21 = std::make_shared<TH2F>("ClusterMap21", "Cluster Map MT21", 300, -300., 300., 300, -300., 300.);
  getObjectsManager()->startPublishing(mClusterMap21.get());
  mClusterMap21->GetXaxis()->SetTitle("X Position (cm)");
  mClusterMap21->GetYaxis()->SetTitle("Y Position (cm)");
  mClusterMap21->SetOption("colz");
  mClusterMap21->SetStats(0);

  mClusterMap22 = std::make_shared<TH2F>("ClusterMap22", "Cluster Map MT22", 300, -300., 300., 300, -300., 300.);
  getObjectsManager()->startPublishing(mClusterMap22.get());
  mClusterMap22->GetXaxis()->SetTitle("X Position (cm)");
  mClusterMap22->GetYaxis()->SetTitle("Y Position (cm)");
  mClusterMap22->SetOption("colz");
  mClusterMap22->SetStats(0);

  mClustBCCounts = std::make_shared<TH1F>("ClustBCCounts", "Cluster Bunch Crossing Counts", o2::constants::lhc::LHCMaxBunches, 0., o2::constants::lhc::LHCMaxBunches);
  getObjectsManager()->startPublishing(mClustBCCounts.get());
  mClustBCCounts->GetXaxis()->SetTitle("BC");
  mClustBCCounts->GetYaxis()->SetTitle("Entry");

  mClustResX = std::make_shared<TH1F>("ClustResX", "Cluster X Resolution ", 300, 0, 30);
  getObjectsManager()->startPublishing(mClustResX.get());
  mClustResX->GetXaxis()->SetTitle("X Resolution (cm)");
  mClustResX->GetYaxis()->SetTitle("Entry");

  mClustResY = std::make_shared<TH1F>("ClustResY", "Cluster Y Resolution ", 300, 0, 30);
  getObjectsManager()->startPublishing(mClustResY.get());
  mClustResY->GetXaxis()->SetTitle("Y Resolution (cm)");
  mClustResY->GetYaxis()->SetTitle("Entry");

  mClustResXDetId = std::make_shared<TH2F>("ClustResXDetId", "Cluster X Resolution vs DetID", MID_NDE, 0, MID_NDE, 300, 0, 30);
  getObjectsManager()->startPublishing(mClustResXDetId.get());
  mClustResXDetId->GetXaxis()->SetTitle("DetID");
  mClustResXDetId->GetYaxis()->SetTitle("X Resolution (cm)");
  mClustResXDetId->SetOption("colz");
  mClustResXDetId->SetStats(0);

  mClustResYDetId = std::make_shared<TH2F>("ClustResYDetId", "Cluster Y Resolution vs DetID", MID_NDE, 0, MID_NDE, 300, 0, 30);
  getObjectsManager()->startPublishing(mClustResYDetId.get());
  mClustResYDetId->GetXaxis()->SetTitle("DetID");
  mClustResYDetId->GetYaxis()->SetTitle("Y Resolution (cm)");
  mClustResYDetId->SetOption("colz");
  mClustResYDetId->SetStats(0);
}

void ClustQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
}

void ClustQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void ClustQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto clusters = ctx.inputs().get<gsl::span<o2::mid::Cluster>>("clusters");
  auto rofs = ctx.inputs().get<gsl::span<o2::mid::ROFRecord>>("clusterrofs");

  int multClusterMT11 = 0;
  int multClusterMT12 = 0;
  int multClusterMT21 = 0;
  int multClusterMT22 = 0;

  std::pair<uint32_t, uint32_t> prevSize;
  o2::InteractionRecord prevIr;

  for (const auto& rofRecord : rofs) { // loop ROFRecords == Events //
    // printf("========================================================== \n");
    // printf("%05d ROF with first entry %05zu and nentries %02zu , BC %05d, ORB %05d , EventType %02d\n", nROF, rofRecord.firstEntry, rofRecord.nEntries, rofRecord.interactionRecord.bc, rofRecord.interactionRecord.orbit,rofRecord.eventType);
    //   eventType::  Standard = 0, Calib = 1, FET = 2
    multClusterMT11 = 0;
    multClusterMT12 = 0;
    multClusterMT21 = 0;
    multClusterMT22 = 0;
    nROF++;

    for (auto& cluster : clusters.subspan(rofRecord.firstEntry, rofRecord.nEntries)) { // loop Cluster in ROF//
      mClustBCCounts->Fill(rofRecord.interactionRecord.bc);
      // std::cout << "  =>>  " << cluster << std::endl;
      mClustResX->Fill(cluster.getEX());
      mClustResXDetId->Fill(cluster.deId, cluster.getEX());
      mClustResY->Fill(cluster.getEY());
      mClustResYDetId->Fill(cluster.deId, cluster.getEY());

      for (int i = 0; i < 4; i++) {
        if ((cluster.getZ() >= o2::mid::geoparams::DefaultChamberZ[i] - DZpos) && (cluster.getZ() <= o2::mid::geoparams::DefaultChamberZ[i] + DZpos)) {
          if (i == 0) {
            multClusterMT11 += 1;
            mClusterMap11->Fill(cluster.getX(), cluster.getY(), 1);
          } else if (i == 1) {
            multClusterMT12 += 1;
            mClusterMap12->Fill(cluster.getX(), cluster.getY(), 1);
          } else if (i == 2) {
            multClusterMT21 += 1;
            mClusterMap21->Fill(cluster.getX(), cluster.getY(), 1);
          } else if (i == 3) {
            multClusterMT22 += 1;
            mClusterMap22->Fill(cluster.getX(), cluster.getY(), 1);
          }
        }
      } // loop on MT //
    }   // cluster in ROF //
    mMultClust11->Fill(multClusterMT11);
    mMultClust12->Fill(multClusterMT12);
    mMultClust21->Fill(multClusterMT21);
    mMultClust22->Fill(multClusterMT22);
  } //  ROFRecords //
}

void ClustQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void ClustQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ClustQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;

  mClusterMap11->Reset();
  mClusterMap12->Reset();
  mClusterMap21->Reset();
  mClusterMap22->Reset();
  mClustBCCounts->Reset();
  mMultClust11->Reset();
  mMultClust12->Reset();
  mMultClust21->Reset();
  mMultClust22->Reset();
}

} // namespace o2::quality_control_modules::mid
