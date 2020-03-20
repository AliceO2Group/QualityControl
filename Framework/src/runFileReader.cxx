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



struct CRUheader
{
  uint8_t header_version;
  uint8_t header_size;
  uint16_t block_length;
  uint16_t fee_id;
  uint8_t priority_bit;
  uint8_t reserved_1;
  uint16_t next_packet_offset;
  uint16_t memory_size;
  uint8_t link_id;
  uint8_t packet_counter;
  uint16_t cru_id : 12;
  uint8_t dpw_id : 4;
  uint32_t hb_orbit;
  //uint16_t cru_id;
  //uint8_t dummy1;
  //uint64_t dummy2;
};


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
      throw std::invalid_argument("Cannot open input file \"" + inputFileName + "\"");
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
    uint32_t CRUbuf[4*4];
    CRUheader CRUh;
   /// send one RDH block via DPL

    int RDH_BLOCK_SIZE = 8192;

    mInputFile.read((char*)(&CRUbuf), sizeof(CRUbuf));
    memcpy(&CRUh,CRUbuf,sizeof(CRUheader));
    if( CRUh.header_version != 4 || CRUh.header_size != 64 ) return;

    RDH_BLOCK_SIZE = CRUh.next_packet_offset;

    printf("%d\t%d\t%d\t%d\n", (int)(CRUh.hb_orbit), (int)(CRUh.packet_counter), (int)(CRUh.memory_size), (int)(CRUh.memory_size-CRUh.header_size));

    char* buf = (char*)malloc(RDH_BLOCK_SIZE);
    memcpy(buf,CRUbuf,CRUh.header_size);

    mInputFile.read(buf+CRUh.header_size, RDH_BLOCK_SIZE-CRUh.header_size);
    if (mInputFile.fail()) {
      if (mPrint) {
        LOG(INFO) << "end of file reached";
      }
      free(buf);
      return; // probably reached eof
    }

    // create the output message
    auto freefct = [](void* data, void* /*hint*/) { free(data); };
    pc.outputs().adoptChunk(Output{ "ROUT", "RAWDATA" }, buf, RDH_BLOCK_SIZE, freefct, nullptr);
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
    AlgorithmSpec{ adaptFromTask<FileReaderTask>() },
    Options{ { "infile", VariantType::String, "data.raw", { "input file name" } } }
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
    Options{ { "infile", VariantType::String, "", { "input file name" } } }
  };
  specs.push_back(producer);

  return specs;
}
// clang-format on
