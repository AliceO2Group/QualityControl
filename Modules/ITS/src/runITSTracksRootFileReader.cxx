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
/// \file    runITSTracksRootFileReader.cxx
/// \author  Kotliarov Artem

/// This is an executable that reads clusters from a root file from disk and sends the data to QC via the Data Processing Layer.

/// Code run:
/// o2-qc-its-tracks-root-file-reader --qc-its-tracks-root-file => File_Tracks.root --qc-its-clusters-root-file => File_Clusters.root | o2-qc --config json://${QUALITYCONTROL_ROOT}/etc/itsTrack.json

// C++
#include <vector>
// ROOT
#include <TFile.h>
#include <TTree.h>
// O2
#include "QualityControl/QcInfoLogger.h"
#include <Framework/CallbackService.h>
#include <Framework/ControlService.h>
#include <Framework/runDataProcessing.h>
#include <Framework/Task.h>
#include <DataFormatsITS/TrackITS.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <DataFormatsITSMFT/CompCluster.h>
#include <DataFormatsITSMFT/Cluster.h>
#include "ReconstructionDataFormats/Vertex.h"
#include "ReconstructionDataFormats/PrimaryVertex.h"
#include <Framework/DataSpecUtils.h>

using namespace o2;
using namespace o2::framework;
using namespace o2::itsmft;

class ITSTracksRootFileReader : public o2::framework::Task
{
 public:
  void init(framework::InitContext& ic)
  {
    LOG(info) << "In ITSTracksRootFileReader::init ... entering ";

    // Tracks
    auto filenameTracks = ic.options().get<std::string>("qc-its-tracks-root-file");

    mFileTracks = std::make_unique<TFile>(filenameTracks.c_str(), "READ");
    if (!mFileTracks->IsOpen()) {
      LOG(error) << "ITSTracksRootFileReader::init. Cannot open file: " << filenameTracks.c_str();
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }

    // Tracks: load TTree and branches
    mTreeTracks = (TTree*)mFileTracks->Get("o2sim");
    mTreeTracks->SetBranchAddress("ITSTrack", &ptracks);
    mTreeTracks->SetBranchAddress("ITSTracksROF", &ptrackRofs);
    mTreeTracks->SetBranchAddress("Vertices", &pvertices);

    // Clusters
    auto filenameCluster = ic.options().get<std::string>("qc-its-clusters-root-file");

    mFileClusters = std::make_unique<TFile>(filenameCluster.c_str(), "READ");
    if (!mFileClusters->IsOpen()) {
      LOG(error) << "ITSTracksRootFileReader::init. Cannot open file: " << filenameCluster.c_str();
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }

    // Clusters: load TTree and branches
    mTreeClusters = (TTree*)mFileClusters->Get("o2sim");
    mTreeClusters->SetBranchAddress("ITSClusterComp", &pclusters);
    mTreeClusters->SetBranchAddress("ITSClustersROF", &pclusterRofs);

    // check match of entries in loaded files
    unsigned long mNumberOfEntriesTrack = mTreeTracks->GetEntries();
    unsigned long mNumberOfEntriesCluster = mTreeClusters->GetEntries();

    if (mNumberOfEntriesTrack != mNumberOfEntriesCluster) {
      LOG(error) << "ITSTracksRootFileReader::init. Mismatch of entries in loaded files.";
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }

    // check entries
    mNumberOfEntries = mTreeTracks->GetEntries();
    if (mNumberOfEntries == 0) {
      LOG(error) << "ITSTracksRootFileReader::init. No entries.";
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }
  }

  void run(framework::ProcessingContext& pc)
  {
    // Check if this is the last Entry
    if (mCurrentEntry == mNumberOfEntries) {
      LOG(info) << " ITSClustersRootFileReader::run. End of files reached.";
      pc.services().get<ControlService>().endOfStream();
      pc.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }

    // load Entry from TTree
    mTreeTracks->GetEntry(mCurrentEntry);
    mTreeClusters->GetEntry(mCurrentEntry);

    // Prepare ROFs output: tracks and clusters
    std::vector<o2::itsmft::ROFRecord>* trackRofArr = new std::vector<o2::itsmft::ROFRecord>();
    std::copy(trackRofs.begin(), trackRofs.end(), std::back_inserter(*trackRofArr));

    std::vector<o2::itsmft::ROFRecord>* clusRofArr = new std::vector<o2::itsmft::ROFRecord>();
    std::copy(clusterRofs.begin(), clusterRofs.end(), std::back_inserter(*clusRofArr));

    // Prepare vector with tracks and clusters for all ROFs
    std::vector<o2::its::TrackITS>* trackArr = new std::vector<o2::its::TrackITS>();
    std::copy(tracks.begin(), tracks.end(), std::back_inserter(*trackArr));

    std::vector<o2::dataformats::Vertex<o2::dataformats::TimeStamp<int>>>* vertexArr = new std::vector<o2::dataformats::Vertex<o2::dataformats::TimeStamp<int>>>();
    std::copy(vertices.begin(), vertices.end(), std::back_inserter(*vertexArr));

    std::vector<o2::itsmft::CompClusterExt>* clusArr = new std::vector<o2::itsmft::CompClusterExt>();
    std::copy(clusters.begin(), clusters.end(), std::back_inserter(*clusArr));

    // Output vectors
    pc.outputs().snapshot(Output{ "ITS", "ITSTrackROF", 0, Lifetime::Timeframe }, *trackRofArr);
    pc.outputs().snapshot(Output{ "ITS", "CLUSTERSROF", 0, Lifetime::Timeframe }, *clusRofArr);
    pc.outputs().snapshot(Output{ "ITS", "TRACKS", 0, Lifetime::Timeframe }, *trackArr);
    pc.outputs().snapshot(Output{ "ITS", "VERTICES", 0, Lifetime::Timeframe }, *vertexArr);
    pc.outputs().snapshot(Output{ "ITS", "COMPCLUSTERS", 0, Lifetime::Timeframe }, *clusArr);

    // move to a new entry in TTree
    ++mCurrentEntry;
  }

 private:
  std::unique_ptr<TFile> mFileTracks = nullptr;                                                           // root file with Tracks
  std::unique_ptr<TFile> mFileClusters = nullptr;                                                         // root file with Clusters
  TTree* mTreeTracks = nullptr;                                                                           // TTree object inside file with Tracks
  TTree* mTreeClusters = nullptr;                                                                         // TTree object inside file with Clusters
  std::vector<o2::itsmft::ROFRecord> trackRofs, *ptrackRofs = &trackRofs;                                 // pointer to ROF branch (tracks)
  std::vector<o2::itsmft::ROFRecord> clusterRofs, *pclusterRofs = &clusterRofs;                           // pointer to ROF branch (clusters)
  std::vector<o2::its::TrackITS> tracks, *ptracks = &tracks;                                              // pointer to Track branch
  std::vector<o2::itsmft::CompClusterExt> clusters, *pclusters = &clusters;                               // pointer to Cluster branch
  std::vector<o2::dataformats::Vertex<o2::dataformats::TimeStamp<int>>> vertices, *pvertices = &vertices; // pointer to Vertex branch

  unsigned long mNumberOfEntries = 0; // number of entries from TTree
  unsigned long mCurrentEntry = 0;    // index of current entry

}; // end class definition

WorkflowSpec defineDataProcessing(const ConfigContext&)
{
  WorkflowSpec specs; // To return the work flow

  // Define the outputs
  std::vector<OutputSpec> outputs;
  outputs.emplace_back("ITS", "ITSTrackROF", 0, Lifetime::Timeframe);
  outputs.emplace_back("ITS", "CLUSTERSROF", 0, Lifetime::Timeframe);
  outputs.emplace_back("ITS", "TRACKS", 0, Lifetime::Timeframe);
  outputs.emplace_back("ITS", "VERTICES", 0, Lifetime::Timeframe);
  outputs.emplace_back("ITS", "COMPCLUSTERS", 0, Lifetime::Timeframe);

  // The producer to generate data in the workflow
  DataProcessorSpec producer{
    "QC-ITS-tracks-root-file-reader",
    Inputs{},
    outputs,
    AlgorithmSpec{ adaptFromTask<ITSTracksRootFileReader>() },
    Options{ { "qc-its-tracks-root-file", VariantType::String, "o2trac_its.root", { "Name of the input file with tracks" } }, { "qc-its-clusters-root-file", VariantType::String, "o2clus_its.root", { "Name of the input file with clusters" } } }
  };
  specs.push_back(producer);

  return specs;
}
