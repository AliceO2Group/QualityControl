// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file    runTracksRootFileReaderMFT.cxx
/// \author  Guillermo Contreras
/// \author  Tomas Herman
///
/// \brief This is an executable that reads tracks from a root file from disk and sends the data to QC via DPL.
///
/// This is an executable that reads tracks from a root file from disk and sends the data to QC via the Data Processing Layer.
/// The idea is based in a similar reader by Andrea Ferrero and the DigitReaderSpec definition in MFT
//  It can be used as a data source for QC development. For example, one can do:
/// \code{.sh}
/// o2-qc-run-tracks-root-file-reader-mft --mft-track-infile=some_data_file | o2-qc --config json://${QUALITYCONTROL_ROOT}/etc/your_config.json
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

class TracksRootFileReaderMFT : public o2::framework::Task
{
 public:
  //_________________________________________________________________________________________________
  void init(framework::InitContext& ic)
  {
    LOG(INFO) << " In TracksRootFileReaderMFT::init ... entering ";

    // open the input file
    auto filename = ic.options().get<std::string>("mft-track-infile");
    mFile = std::make_unique<TFile>(filename.c_str(), "OLD");
    if (!mFile->IsOpen()) {
      LOG(ERROR) << "TracksRootFileReaderMFT::init. Cannot open the file: " << filename.c_str();
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }
  }

  //_________________________________________________________________________________________________

  void run(framework::ProcessingContext& pc)
  {
    // get vector of ROF
    std::unique_ptr<TTree> tree((TTree*)mFile->Get("o2sim"));
    std::vector<o2::mft::TrackMFT> tracks, *ptracks = &tracks;
    tree->SetBranchAddress("MFTTrack", &ptracks);
    tree->GetEntry(0);

    // Check if there is a new Track
    auto ntracks = tracks.size();
    if (currentTrack >= ntracks) {
      // if (currentTrack >= 50) {
      LOG(INFO) << " TracksRootFileReaderMFT::run. End of file reached";
      pc.services().get<ControlService>().endOfStream();
      pc.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }
    // prepare the track output
    std::vector<o2::mft::TrackMFT>* TracksInFile = new std::vector<o2::mft::TrackMFT>();
    std::copy(tracks.begin() + currentTrack, tracks.begin() + currentTrack + 1, std::back_inserter(*TracksInFile));
    currentTrack++;

    // fill in the message
    // LOG(INFO) << " TracksRootFileReaderMFT::run. In this file there are  " << TracksInFile.size() << " tracks";
    pc.outputs().snapshot(Output{ "MFT", "TRACKSMFT", 0, Lifetime::Timeframe }, *TracksInFile);
  }

 private:
  std::unique_ptr<TFile> mFile = nullptr;
  unsigned long currentTrack = 0;

}; // end class definition

WorkflowSpec defineDataProcessing(const ConfigContext&)
{
  WorkflowSpec specs; // to return the work flow

  // define the outputs
  std::vector<OutputSpec> outputs;
  outputs.emplace_back("MFT", "TRACKSMFT", 0, Lifetime::Timeframe);

  // The producer to generate some data in the workflow
  DataProcessorSpec producer{
    "tracks-root-file-reader-mft",
    Inputs{},
    outputs,
    AlgorithmSpec{ adaptFromTask<TracksRootFileReaderMFT>() },
    Options{ { "mft-track-infile", VariantType::String, "mfttracks.root", { "Name of the input file" } } }
  };
  specs.push_back(producer);

  return specs;
}
