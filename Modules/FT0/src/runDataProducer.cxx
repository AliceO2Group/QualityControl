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
/// \file   runDataProducer.cxx
/// \author Milosz Filus
///

#include <vector>
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include "TTree.h"
#include "TFile.h"
#include <Framework/CallbackService.h>
#include <Framework/ControlService.h>
#include <Framework/runDataProcessing.h>
#include <Framework/Task.h>
#include <DataFormatsITSMFT/Cluster.h>
#include <DataFormatsITSMFT/ROFRecord.h>

using namespace o2;
using namespace o2::framework;

namespace o2::quality_control_modules::ft0
{

class FT0DataProducer : public Task
{
 public:
  void init(InitContext& ic) final
  {

    auto filename = ic.options().get<std::string>("ft0-input-digit-file");
    mFile = std::make_unique<TFile>(filename.c_str(), "OLD");
    if (!mFile->IsOpen()) {
      LOG(ERROR) << "Cannot open the " << filename.c_str() << " file !";
      throw std::runtime_error("cannot open input digits file");
    }
    mTree.reset((TTree*)mFile->Get("o2sim"));
    if (!mTree) {
      LOG(ERROR) << "Did not find o2sim tree in " << filename.c_str();
      throw std::runtime_error("Did not fine o2sim file in FT0 digits tree");
    }
  }
  void run(ProcessingContext& pc) final
  {
    std::vector<o2::ft0::Digit> digits, *pdigits = &digits;
    std::vector<o2::ft0::ChannelData> channels, *pchannels = &channels;
    mTree->SetBranchAddress("FT0DIGITSBC", &pdigits);
    mTree->SetBranchAddress("FT0DIGITSCH", &pchannels);

    for (int tFrame = 0; tFrame < mTree->GetEntries(); ++tFrame) {
      mTree->GetEntry(tFrame);
      pc.outputs().snapshot(Output{ "FT0", "DIGITSBC", 0, Lifetime::Timeframe }, digits);
      pc.outputs().snapshot(Output{ "FT0", "DIGITSCH", 0, Lifetime::Timeframe }, channels);
    }

    pc.services().get<ControlService>().endOfStream();
    pc.services().get<ControlService>().readyToQuit(QuitRequest::Me);
  }

 private:
  std::unique_ptr<TTree> mTree;
  std::unique_ptr<TFile> mFile;
};

} // namespace o2::quality_control_modules::ft0

WorkflowSpec defineDataProcessing(ConfigContext const&)
{

  WorkflowSpec workflow;
  DataProcessorSpec spec{
    "FT0Producer",
    Inputs{},
    Outputs{
      OutputSpec{ "FT0", "DIGITSBC", 0, Lifetime::Timeframe },
      OutputSpec{ "FT0", "DIGITSCH", 0, Lifetime::Timeframe } },
    AlgorithmSpec{ adaptFromTask<o2::quality_control_modules::ft0::FT0DataProducer>() },
    Options{
      { "ft0-input-digit-file", VariantType::String, "ft0digits.root", { "path to digits file (with separated timeframes)" } } }
  };

  workflow.push_back(spec);
  return workflow;
}
