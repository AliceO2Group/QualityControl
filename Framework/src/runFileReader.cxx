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
/// \file    runFileReader.cxx
/// \author  Andrea Ferrero
///
/// \brief This is an executable that reads a data file from disk and sends the data to QC via DPL.
///
/// This is an executable that reads a data file from disk and sends the data to QC via the Data Processing Layer.
/// It can be used as a data source for QC development. For example, one can do:
/// \code{.sh}
/// o2-qc-run-file-reader --infile=some_data_file | o2-qc --config json://${QUALITYCONTROL_ROOT}/etc/your_config.json
/// \endcode
///

#include <random>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <Framework/CallbackService.h>
#include <Framework/ControlService.h>
#include <Framework/Task.h>
#include <Framework/runDataProcessing.h>

using namespace o2;
using namespace o2::framework;


class FileReaderTask
{
public:
  //_________________________________________________________________________________________________
  void init(framework::InitContext& ic)
  {
    /// Get the input file and other options from the context
    LOG(INFO) << "initializing file reader";

    auto inputFileName = ic.options().get<std::string>("infile");
    mInputFile.open(inputFileName, std::ios::binary);
    if (!mInputFile.is_open()) {
      throw std::invalid_argument("Cannot open input file " + inputFileName);
    }

    auto stop = [this]() {
      /// close the input file
      LOG(INFO) << "stop file reader";
      this->mInputFile.close();
    };
    ic.services().get<CallbackService>().set(CallbackService::Id::Stop, stop);
  }

  //_________________________________________________________________________________________________
  void run(framework::ProcessingContext& pc)
  {
    /// send one RDH block via DPL

    const int RDH_BLOCK_SIZE = 8192;
    char* buf = (char*)malloc(RDH_BLOCK_SIZE);

    mInputFile.read(buf, RDH_BLOCK_SIZE);
    if (mInputFile.fail()) {
      if (mPrint) {
        LOG(INFO) << "end of file reached";
      }
      free(buf);
      return; // probably reached eof
    }

    // create the output message
    auto msgOut = pc.outputs().make<char>(Output{"ROUT", "RAWDATA"}, RDH_BLOCK_SIZE);
    if (msgOut.size() != RDH_BLOCK_SIZE) {
      free(buf);
      throw std::length_error("incorrect message payload");
    }

    auto bufferPtr = msgOut.data();
    // fill output buffer
    memcpy(bufferPtr, buf, RDH_BLOCK_SIZE);
    free(buf);
  }

private:
  std::ifstream mInputFile{}; ///< input file
  bool mPrint = false;        ///< print digits
};

//_________________________________________________________________________________________________
o2::framework::DataProcessorSpec getFileReaderSpec()
{
  return DataProcessorSpec{
    "FileReader",
    Inputs{},
    Outputs{ { { "readout" }, { "ROUT", "RAWDATA" } } },
    AlgorithmSpec{adaptFromTask<FileReaderTask>()},
    Options{{"infile", VariantType::String, "data.raw", {"input file name"}}}
  };
}



// clang-format off
WorkflowSpec defineDataProcessing(const ConfigContext&)
{
  WorkflowSpec specs;

  // The producer to generate some data in the workflow
  DataProcessorSpec producer{
    "FileReader",
    Inputs{},
    Outputs{ { { "readout" }, { "ROUT", "RAWDATA" } } },
    AlgorithmSpec{adaptFromTask<FileReaderTask>()},
    Options{{"infile", VariantType::String, "", {"input file name"}}}
  };
  specs.push_back(producer);

  return specs;
}
// clang-format on
