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
/// \file    runMFTTracksRootFileReader.cxx
/// \author  Guillermo Contreras
/// \author  Tomas Herman
/// \author  Katarina Krizkova Gajdosova
/// \author  Diana Maria Krupova
///
/// \brief This is an executable that reads tracks from a root file from disk and sends the data to QC via DPL.
///
/// This is an executable that reads tracks from a root file from disk and sends the data to QC via the Data Processing Layer.
/// The idea is based in a similar reader by Andrea Ferrero and the DigitReaderSpec definition in MFT
//  It can be used as a data source for QC development. For example, one can do:
/// \code{.sh}
/// o2-qc-mft-tracks-root-file-reader --mft-track-infile=some_data_file | o2-qc --config json://${QUALITYCONTROL_ROOT}/etc/your_config.json
/// \endcode
///

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
#include <DataFormatsMFT/TrackMFT.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <MFTTracking/TrackCA.h>

using namespace o2;
using namespace o2::framework;
using namespace o2::itsmft;
using namespace o2::mft;

class MFTTracksRootFileReader : public o2::framework::Task
{
 public:
  //_________________________________________________________________________________________________
  void init(framework::InitContext& ic)
  {
    LOG(INFO) << " In MFTTracksRootFileReader::init ... entering ";

    // open the input file
    auto filename = ic.options().get<std::string>("mft-track-infile");
    mFile = std::make_unique<TFile>(filename.c_str(), "OLD");
    if (!mFile->IsOpen()) {
      LOG(ERROR) << "MFTTracksRootFileReader::init. Cannot open the file: " << filename.c_str();
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }

    // get the tree
    mTree = (TTree*)mFile->Get("o2sim");
    mTree->SetBranchAddress("MFTTrack", &ptracks);
    mTree->SetBranchAddress("MFTTracksROF", &profs);

    // check that it has entries
    mNumberOfTF = mTree->GetEntries();
    if (mNumberOfTF == 0) {
      LOG(ERROR) << "MFTTracksRootFileReader::init. No TFs ";
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }
  }

  //_________________________________________________________________________________________________

  void run(framework::ProcessingContext& pc)
  {

    // Check if this is the last TF
    if (mCurrentTF == mNumberOfTF) {
      LOG(INFO) << " MFTDTracksRootFileReader::run. End of file reached";
      pc.services().get<ControlService>().endOfStream();
      pc.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }

    mTree->GetEntry(mCurrentTF); // get TF
    mNumberOfROF = rofs.size();  // get number of ROFs in this TF

    // check if we need to read a new TF
    if (mCurrentROF == mNumberOfROF - 1)
      mCurrentTF++;

    // prepare the rof output
    std::vector<o2::itsmft::ROFRecord>* oneROFvec = new std::vector<o2::itsmft::ROFRecord>();
    std::copy(rofs.begin() + mCurrentROF, rofs.begin() + mCurrentROF + 1, std::back_inserter(*oneROFvec));

    // get the tracks in current ROF
    // --> get the current ROF
    auto& rof = rofs[mCurrentROF];
    // --> find the ranges
    int index = rof.getFirstEntry(); // first track position
    int numberOfTracksInROF = rof.getNEntries();
    int lastIndex = index + numberOfTracksInROF;

    // --> fill in the corresponding tracks
    std::vector<o2::mft::TrackMFT>* tracksInROF = new std::vector<o2::mft::TrackMFT>();
    std::copy(tracks.begin() + index, tracks.begin() + lastIndex, std::back_inserter(*tracksInROF));

    // fill in the message
    pc.outputs().snapshot(Output{ "MFT", "TRACKS", 0, Lifetime::Timeframe }, *tracksInROF);
    pc.outputs().snapshot(Output{ "MFT", "MFTTrackROF", 0, Lifetime::Timeframe }, *oneROFvec);

    //  update the ROF counter
    mCurrentROF++;
  }

 private:
  std::unique_ptr<TFile> mFile = nullptr;                    // file to be read
  TTree* mTree = nullptr;                                    // tree inside the file
  std::vector<o2::itsmft::ROFRecord> rofs, *profs = &rofs;   // pointer to ROF branch
  std::vector<o2::mft::TrackMFT> tracks, *ptracks = &tracks; // pointer to digit branch

  unsigned long mNumberOfTF = 0;  // number of TF
  unsigned long mNumberOfROF = 0; // number of ROFs in current TF
  unsigned long mCurrentROF = 2;  // idx of current ROF
  unsigned long mCurrentTF = 0;   // idx of current TF

}; // end class definition

WorkflowSpec defineDataProcessing(const ConfigContext&)
{
  WorkflowSpec specs; // to return the work flow

  // define the outputs
  std::vector<OutputSpec> outputs;

  outputs.emplace_back("MFT", "TRACKS", 0, Lifetime::Timeframe);
  outputs.emplace_back("MFT", "MFTTrackROF", 0, Lifetime::Timeframe);

  // The producer to generate some data in the workflow
  DataProcessorSpec producer{
    "tracks-root-file-reader-mft",
    Inputs{},
    outputs,
    AlgorithmSpec{ adaptFromTask<MFTTracksRootFileReader>() },
    Options{ { "mft-track-infile", VariantType::String, "mfttracks.root", { "Name of the input file" } } }
  };
  specs.push_back(producer);

  return specs;
}
