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
/// o2-qc-its-clusters-root-file-reader --its-cluster-root-file => File_Name.root | o2-qc --config json://${QUALITYCONTROL_ROOT}/etc/itsCluster.json

// C++
#include <vector>
// ROOT
#include <TFile.h>
#include <TTree.h>
// O2
#include <Framework/CallbackService.h>
#include <Framework/ControlService.h>
#include <Framework/runDataProcessing.h>
#include <Framework/Task.h>
#include <DataFormatsITSMFT/Cluster.h>
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
    LOG(info) << " In ITSClustersRootFileReader::init ... entering ";

    // open input file
    auto filename = ic.options().get<std::string>("its-cluster-root-file");
    mFile = std::make_unique<TFile>(filename.c_str(), "READ");
    if (!mFile->IsOpen()) {
      LOG(error) << "ITSClustersRootFileReader::init. Cannot open file: " << filename.c_str();
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }

    // get TTree
    mTree = (TTree*)mFile->Get("o2sim");
    mTree->SetBranchAddress("ITSClusterComp", &pclusters);
    mTree->SetBranchAddress("ITSClustersROF", &profs);

    mNumberOfEntries = mTree->GetEntries();
    if (mNumberOfEntries == 0) {
      LOG(error) << "ITSClustersRootFileReader::init. No entries.";
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }
  }

  void run(framework::ProcessingContext& pc)
  {

    // Check if this is the last TF
    if (mCurrentEntry == mNumberOfEntries) {
      LOG(info) << " ITSClustersRootFileReader::run. End of file reached.";
      pc.services().get<ControlService>().endOfStream();
      pc.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }

    mTree->GetEntry(mCurrentEntry); // load Entry from TTree

    // Prepare ROF output
    std::vector<o2::itsmft::ROFRecord>* clusRofArr = new std::vector<o2::itsmft::ROFRecord>();
    std::copy(rofs.begin(), rofs.end(), std::back_inserter(*clusRofArr));

    // Prepare vector with clusters for all ROFs
    std::vector<o2::itsmft::CompClusterExt>* clusArr = new std::vector<o2::itsmft::CompClusterExt>();

    // Get number of rofs
    mNumberOfROF = rofs.size();

    for (unsigned int mCurrentROF = 0; mCurrentROF < mNumberOfROF; mCurrentROF++) {

      // get clusters in current ROF
      auto& rof = rofs[mCurrentROF];

      // First cluster position in current ROF
      unsigned int index = rof.getFirstEntry();

      // Final cluster position in current ROF
      unsigned int numberOfClustersInROF = rof.getNEntries();
      unsigned int lastIndex = index + numberOfClustersInROF;

      // Fill corresponding clusters
      std::copy(clusters.begin() + index, clusters.begin() + lastIndex, std::back_inserter(*clusArr));
    }

    // Output vectors
    pc.outputs().snapshot(Output{ "ITS", "COMPCLUSTERS", 0, Lifetime::Timeframe }, *clusArr);
    pc.outputs().snapshot(Output{ "ITS", "CLUSTERSROF", 0, Lifetime::Timeframe }, *clusRofArr);

    // read a new entry in TTree
    ++mCurrentEntry;
  }

 private:
  std::unique_ptr<TFile> mFile = nullptr;                                   // file to be read
  TTree* mTree = nullptr;                                                   // TTree object inside file
  std::vector<o2::itsmft::ROFRecord> rofs, *profs = &rofs;                  // pointer to ROF branch
  std::vector<o2::itsmft::CompClusterExt> clusters, *pclusters = &clusters; // pointer to Cluster branch

  unsigned long mNumberOfEntries = 0; // number of entries from TTree
  unsigned long mCurrentEntry = 0;    // index of current entry
  unsigned long mNumberOfROF = 0;     // number of ROFs in current entry

}; // end class definition

WorkflowSpec defineDataProcessing(const ConfigContext&)
{
  WorkflowSpec specs; // To return the work flow

  // Define the outputs
  std::vector<OutputSpec> outputs;
  outputs.emplace_back("ITS", "COMPCLUSTERS", 0, Lifetime::Timeframe);
  outputs.emplace_back("ITS", "CLUSTERSROF", 0, Lifetime::Timeframe);

  // The producer to generate data in the workflow
  DataProcessorSpec producer{
    "clusters-root-file-reader-ITS",
    Inputs{},
    outputs,
    AlgorithmSpec{ adaptFromTask<ITSClustersRootFileReader>() },
    Options{ { "its-cluster-root-file", VariantType::String, "o2clus_its.root", { "Name of the input file" } } }
  };
  specs.push_back(producer);

  return specs;
}
