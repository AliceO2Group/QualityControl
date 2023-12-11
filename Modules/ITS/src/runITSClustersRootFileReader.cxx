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
/// \file    runITSClustersRootFileReader.cxx
/// \author  Kotliarov Artem

/// This is an executable that reads clusters from a root file from disk and sends the data to QC via the Data Processing Layer.
/// The code is inspired by a similar reader "runMFTClustersRootFileReader.cxx" (authors Guillermo Contreras, Tomas Herman, Katarina Krizkova Gajdosova, Diana Maria Krupova)

/// Code run:
/// o2-qc-its-clusters-root-file-reader --qc-its-clusters-root-file => File_Clusters.root | o2-qc --config json://${QUALITYCONTROL_ROOT}/etc/itsCluster.json

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
#include <DataFormatsITSMFT/Cluster.h>
#include <DataFormatsITSMFT/ClusterTopology.h>
#include <DataFormatsITSMFT/CompCluster.h>
#include <DataFormatsITSMFT/ROFRecord.h>

using namespace o2;
using namespace o2::framework;
using namespace o2::itsmft;

class ITSClustersRootFileReader : public o2::framework::Task
{

 public:
  void init(framework::InitContext& ic)
  {

    // open input file
    auto filename = ic.options().get<std::string>("qc-its-clusters-root-file");
    mFile = std::make_unique<TFile>(filename.c_str(), "READ");
    if (!mFile->IsOpen()) {
      ILOG(Error, Support) << "ITSClustersRootFileReader::init. Cannot open file: " << filename.c_str() << ENDM;
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }

    // load TTree and branches
    mTree = (TTree*)mFile->Get("o2sim");
    mTree->SetBranchAddress("ITSClustersROF", &profs);
    mTree->SetBranchAddress("ITSClusterComp", &pclusters);
    mTree->SetBranchAddress("ITSClusterPatt", &ppatterns);

    // check entries
    mNumberOfEntries = mTree->GetEntries();
    if (mNumberOfEntries == 0) {
      ILOG(Error, Support) << "ITSClustersRootFileReader::init. No entries." << ENDM;
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }
  }

  void run(framework::ProcessingContext& pc)
  {

    // Check if this is the last Entry
    if (mCurrentEntry == mNumberOfEntries) {
      ILOG(Info, Support) << " ITSClustersRootFileReader::run. End of file reached." << ENDM;
      pc.services().get<ControlService>().endOfStream();
      pc.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }

    // load Entry from TTree
    mTree->GetEntry(mCurrentEntry);

    // Prepare ROFs output
    std::vector<o2::itsmft::ROFRecord>* clusRofArr = new std::vector<o2::itsmft::ROFRecord>();
    std::copy(rofs.begin(), rofs.end(), std::back_inserter(*clusRofArr));

    // Prepare vector with clusters for all ROFs
    std::vector<o2::itsmft::CompClusterExt>* clusArr = new std::vector<o2::itsmft::CompClusterExt>();
    std::copy(clusters.begin(), clusters.end(), std::back_inserter(*clusArr));

    // Prepare vector with cluster patterns for all ROFs
    std::vector<unsigned char>* clusPatternArr = new std::vector<unsigned char>();
    std::copy(patterns.begin(), patterns.end(), std::back_inserter(*clusPatternArr));

    // Output vectors
    pc.outputs().snapshot(Output{ "ITS", "CLUSTERSROF", 0 }, *clusRofArr);
    pc.outputs().snapshot(Output{ "ITS", "COMPCLUSTERS", 0 }, *clusArr);
    pc.outputs().snapshot(Output{ "ITS", "PATTERNS", 0 }, *clusPatternArr);

    // move to a new entry in TTree
    ++mCurrentEntry;
  }

 private:
  std::unique_ptr<TFile> mFile = nullptr;                                   // root file with Clusters
  TTree* mTree = nullptr;                                                   // TTree object inside file
  std::vector<o2::itsmft::ROFRecord> rofs, *profs = &rofs;                  // pointer to ROF branch
  std::vector<o2::itsmft::CompClusterExt> clusters, *pclusters = &clusters; // pointer to Cluster branch
  std::vector<unsigned char> patterns, *ppatterns = &patterns;              // pointer to Pattern branch

  unsigned long mNumberOfEntries = 0; // number of entries from TTree
  unsigned long mCurrentEntry = 0;    // index of current entry

}; // end class definition

WorkflowSpec defineDataProcessing(const ConfigContext&)
{
  WorkflowSpec specs; // To return the work flow

  // Define the outputs
  std::vector<OutputSpec> outputs;
  outputs.emplace_back("ITS", "CLUSTERSROF", 0, Lifetime::Timeframe);
  outputs.emplace_back("ITS", "COMPCLUSTERS", 0, Lifetime::Timeframe);
  outputs.emplace_back("ITS", "PATTERNS", 0, Lifetime::Timeframe);

  // The producer to generate data in the workflow
  DataProcessorSpec producer{
    "QC-ITS-clusters-root-file-reader",
    Inputs{},
    outputs,
    AlgorithmSpec{ adaptFromTask<ITSClustersRootFileReader>() },
    Options{ { "qc-its-clusters-root-file", VariantType::String, "o2clus_its.root", { "Name of the input file with clusters" } } }
  };
  specs.push_back(producer);

  return specs;
}
