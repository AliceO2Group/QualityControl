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
/// \file    runMFTDigitsHotPixelRootFileReader.cxx
/// \author  Guillermo Contreras
/// \author  Tomas Herman
/// \author  Katarina Krizkova Gajdosova
/// \author  Diana Maria Krupova
///
/// \brief This is an executable that reads digits from a root file from disk adds hot pixels and sends the data to QC via DPL.
///
/// This is an executable that reads digits from a root file from disk and sends the data to QC via the Data Processing Layer.
/// The idea is based in a similar reader by Andrea Ferrero and the DigitReaderSpec definition in MFT
//  It can be used as a data source for QC development with hot pixels. For example, one can do:
/// \code{.sh}
/// o2-qc-mft-digits-hot-pixel-root-file-reader --mft-digit-infile=some_data_file | o2-qc --config json://${QUALITYCONTROL_ROOT}/etc/your_config.json
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

class MFTDigitsHotPixelRootFileReader : public o2::framework::Task
{
 public:
  //_________________________________________________________________________________________________
  void init(framework::InitContext& ic)
  {
    LOG(INFO) << " In MFTDigitsHotPixelRootFileReader::init ... entering ";

    // open the input file
    auto filename = ic.options().get<std::string>("mft-digit-infile");
    mFile = std::make_unique<TFile>(filename.c_str(), "OLD");
    if (!mFile->IsOpen()) {
      LOG(ERROR) << "MFTDigitsHotPixelRootFileReader::init. Cannot open the file: " << filename.c_str();
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }

    // get the tree
    mTree = (TTree*)mFile->Get("o2sim");
    mTree->SetBranchAddress("MFTDigit", &pdigits);
    mTree->SetBranchAddress("MFTDigitROF", &profs);

    // check that it has entries
    mNumberOfTF = mTree->GetEntries();
    if (mNumberOfTF == 0) {
      LOG(ERROR) << "MFTDigitsHotPixelRootFileReader::init. No TFs ";
      ic.services().get<ControlService>().endOfStream();
      ic.services().get<ControlService>().readyToQuit(QuitRequest::Me);
      return;
    }
    //LOG(INFO) << " oooooooooooo In MFTDigitsHotPixelRootFileReader::init ... mNumberOfTF = " << mNumberOfTF;
  }

  //_________________________________________________________________________________________________

  void run(framework::ProcessingContext& pc)
  {

    // Check if this is the last TF
    if (mCurrentTF == mNumberOfTF) {
      LOG(INFO) << " MFTDigitsHotPixelRootFileReader::run. End of file reached";
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

    // get the digits in current ROF
    // --> get the current ROF
    auto& rof = rofs[mCurrentROF];
    // --> find the ranges
    int index = rof.getFirstEntry(); // first digit position
    int numberOfDigitsInROF = rof.getNEntries();
    int lastIndex = index + numberOfDigitsInROF;

    // --> fill in the corresponding digits
    std::vector<o2::itsmft::Digit>* digitsInROF = new std::vector<o2::itsmft::Digit>();
    std::copy(digits.begin() + index, digits.begin() + lastIndex, std::back_inserter(*digitsInROF));

    // --> create a noisy digit
    AddHotPixel(digitsInROF, 2, 200, 200, 165);
    AddHotPixel(digitsInROF, 10, 200, 200, 165);
    AddHotPixel(digitsInROF, 35, 200, 200, 165);

    // fill in the message
    pc.outputs().snapshot(Output{ "MFT", "DIGITS", 0, Lifetime::Timeframe }, *digitsInROF);
    pc.outputs().snapshot(Output{ "MFT", "DIGITSROF", 0, Lifetime::Timeframe }, *oneROFvec);

    // update the ROF counter
    mCurrentROF++;
  }

 private:
  std::unique_ptr<TFile> mFile = nullptr;                    // file to be read
  TTree* mTree = nullptr;                                    // tree inside the file
  std::vector<o2::itsmft::ROFRecord> rofs, *profs = &rofs;   // pointer to ROF branch
  std::vector<o2::itsmft::Digit> digits, *pdigits = &digits; // pointer to digit branch

  unsigned long mNumberOfTF = 0;  // number of TF
  unsigned long mNumberOfROF = 0; // number of ROFs in current TF
  unsigned long mCurrentROF = 0;  // idx of current ROF
  unsigned long mCurrentTF = 0;   // idx of current TF

  void AddHotPixel(std::vector<o2::itsmft::Digit>* digitsInROF, int chipID, int col, int row, int charge)
  {
    o2::itsmft::Digit NoisyPixel;
    NoisyPixel.setChipIndex(chipID);
    NoisyPixel.setPixelIndex(row, col);
    NoisyPixel.setCharge(charge);

    auto check = 1;
    for (auto& one_digit : *digitsInROF) {
      if ((one_digit.getChipIndex() == chipID) && (one_digit.getColumn() == col) && (one_digit.getRow() == row))
        check = 0;
    }

    if (check)
      digitsInROF->push_back(NoisyPixel);
  }

}; // end class definition

WorkflowSpec defineDataProcessing(const ConfigContext&)
{
  WorkflowSpec specs; // to return the work flow

  // define the outputs
  std::vector<OutputSpec> outputs;
  outputs.emplace_back("MFT", "DIGITS", 0, Lifetime::Timeframe);
  outputs.emplace_back("MFT", "DIGITSROF", 0, Lifetime::Timeframe);

  // The producer to generate some data in the workflow
  DataProcessorSpec producer{
    "digits-root-file-reader-mft",
    Inputs{},
    outputs,
    AlgorithmSpec{ adaptFromTask<MFTDigitsHotPixelRootFileReader>() },
    Options{ { "mft-digit-infile", VariantType::String, "mftdigits.root", { "Name of the input file" } } }
  };
  specs.push_back(producer);

  return specs;
}
