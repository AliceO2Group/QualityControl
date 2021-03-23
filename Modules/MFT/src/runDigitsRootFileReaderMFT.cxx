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
/// \file    runDigitsRootFileReaderMFT.cxx
/// \author  Guillermo Contreras
/// \author  Tomas Herman
///
/// \brief This is an executable that reads digits from a root file from disk and sends the data to QC via DPL.
///
/// This is an executable that reads digits from a root file from disk and sends the data to QC via the Data Processing Layer.
/// The idea is based in a similar reader by Andrea Ferrero and the DigitReaderSpec definition in MFT
//  It can be used as a data source for QC development. For example, one can do:
/// \code{.sh}
/// o2-qc-run-digits-root-file-reader-mft --mft-digit-infile=some_data_file | o2-qc --config json://${QUALITYCONTROL_ROOT}/etc/your_config.json
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
#include <DataFormatsITSMFT/Digit.h>
#include <DataFormatsITSMFT/ROFRecord.h>

using namespace o2;
using namespace o2::framework;
using namespace o2::itsmft;

class DigitsRootFileReaderMFT : public o2::framework::Task
{
 public:
  //_________________________________________________________________________________________________
  void init(framework::InitContext& ic)
  {
    LOG(INFO) << " In DigitsRootFileReaderMFT::init ... entering ";

    // open the input file
    auto filename = ic.options().get<std::string>("mft-digit-infile");
    mFile = std::make_unique<TFile>(filename.c_str(), "OLD");
    if (!mFile->IsOpen()) {
      LOG(ERROR) << "DigitsRootFileReaderMFT::init. Cannot open the file: " << filename.c_str();
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }

    // get the tree
    mTree = (TTree*) mFile->Get("o2sim");
    mTree->SetBranchAddress("MFTDigit", &pdigits);
    mTree->SetBranchAddress("MFTDigitROF", &profs);

    // check that it has entries
    nTFs = mTree->GetEntries();
    if (nTFs == 0) {
      LOG(ERROR) << "DigitsRootFileReaderMFT::init. No TFs ";
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }
    LOG(INFO) << " oooooooooooo In DigitsRootFileReaderMFT::init ... nTFs = " << nTFs;
    
  }

  //_________________________________________________________________________________________________

  void run(framework::ProcessingContext& pc)
  {    
    // Check if this is the last TF
    if (currentTF == nTFs) {
      LOG(INFO) << " DigitsRootFileReaderMFT::run. End of file reached";
      pc.services().get<ControlService>().endOfStream();
      pc.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }

    // check if we need to read a new TF
    if (currentROF == nROFs) {
      mTree->GetEntry(currentTF); // get new TF
      currentTF++;
      nROFs = rofs.size(); // get number of ROFs in this TF
      currentROF = 0;
      LOG(INFO) << " oooooooooooo Reading TF " << currentTF << " from " << nTFs
    << " with " << nROFs << " ROFs";      
    }

    // prepare the rof output
    std::vector<o2::itsmft::ROFRecord>* oneROFvec = new std::vector<o2::itsmft::ROFRecord>();
    std::copy(rofs.begin() + currentROF, rofs.begin() + currentROF + 1, std::back_inserter(*oneROFvec));
    
    // get the digits in current ROF
    // --> get the current ROF
    auto& rof = rofs[currentROF];
    // --> find the ranges
    int index = rof.getFirstEntry();      // first digit position
    int nDigitsInROF = rof.getNEntries(); // number of digits
    int lastIndex = index + nDigitsInROF;
    
    // --> fill in the corresponding digits
    std::vector<o2::itsmft::DigitHW>* DigitsInROF = new std::vector<o2::itsmft::DigitHW>();
    std::copy(digits.begin() + index, digits.begin() + lastIndex, std::back_inserter(*DigitsInROF));

    // fill in the message
    // LOG(INFO) << " DigitsRootFileReaderMFT::run. In this ROF there are  " << DigitsInROF.size() << " digits";
    pc.outputs().snapshot(Output{ "MFT", "DIGITS", 0, Lifetime::Timeframe }, *DigitsInROF);
    pc.outputs().snapshot(Output{ "MFT", "MFTDigitROF", 0, Lifetime::Timeframe }, *oneROFvec);

    // update the ROF counter
    currentROF++;
    // usleep(100);

  }

 private:
  std::unique_ptr<TFile> mFile = nullptr; // file to be read
  TTree *mTree = nullptr; // tree inside the file 
  std::vector<o2::itsmft::ROFRecord> rofs, *profs = &rofs; // pointer to ROF branch
  std::vector<o2::itsmft::DigitHW> digits, *pdigits = &digits; // pointer to digit branch

  unsigned long nTFs = 0; // number of TF
  unsigned long nROFs = 0; // number of ROFs in current TF
  unsigned long currentROF = 0; // idx of current ROF
  unsigned long currentTF = 0; // idx of current TF

}; // end class definition

WorkflowSpec defineDataProcessing(const ConfigContext&)
{
  WorkflowSpec specs; // to return the work flow

  // define the outputs
  std::vector<OutputSpec> outputs;
  outputs.emplace_back("MFT", "DIGITS", 0, Lifetime::Timeframe);
  outputs.emplace_back("MFT", "MFTDigitROF", 0, Lifetime::Timeframe);

  // The producer to generate some data in the workflow
  DataProcessorSpec producer{
    "digits-root-file-reader-mft",
    Inputs{},
    outputs,
    AlgorithmSpec{ adaptFromTask<DigitsRootFileReaderMFT>() },
    Options{ { "mft-digit-infile", VariantType::String, "mftdigits.root", { "Name of the input file" } } }
  };
  specs.push_back(producer);

  return specs;
}
