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
/// \file   Utility.cxx
/// \author Thomas Klemenz
///

// O2 includes
#include "Framework/ProcessingContext.h"
#include "Framework/InputRecordWalker.h"
#include "DataFormatsTPC/ClusterNativeHelper.h"
#include "DataFormatsTPC/TPCSectorHeader.h"

// QC includes
#include "TPC/Utility.h"

// external includes
#include <Framework/Logger.h>
#include <bitset>

namespace o2::quality_control_modules::tpc
{

void addAndPublish(std::shared_ptr<o2::quality_control::core::ObjectsManager> objectsManager, std::vector<std::unique_ptr<TCanvas>>& canVec, std::vector<std::string_view> canvNames, const std::map<std::string, std::string>& metaData)
{
  for (const auto& canvName : canvNames) {
    canVec.emplace_back(std::make_unique<TCanvas>(canvName.data()));
    auto canvas = canVec.back().get();
    objectsManager->startPublishing(canvas);
    if (metaData.size() != 0) {
      for (const auto& [key, value] : metaData) {
        objectsManager->addMetadata(canvas->GetName(), key, value);
      }
    }
  }
}

std::vector<TCanvas*> toVector(std::vector<std::unique_ptr<TCanvas>>& input)
{
  std::vector<TCanvas*> output;
  for (auto& in : input) {
    output.emplace_back(in.get());
  }
  return output;
}

o2::tpc::ClusterNativeAccess clusterHandler(o2::framework::InputRecord& input)
{
  using namespace o2::tpc;
  using namespace o2::framework;

  ClusterNativeAccess clusterIndex;

  constexpr static size_t NSectors = o2::tpc::constants::MAXSECTOR;

  std::vector<gsl::span<const char>> inputs;
  struct InputRef {
    DataRef data;
    DataRef labels;
  };
  std::map<int, InputRef> inputrefs;

  std::bitset<NSectors> validInputs = 0;

  int operation = 0;
  std::vector<int> inputIds(36);
  std::iota(inputIds.begin(), inputIds.end(), 0);
  std::vector<InputSpec> filter = {
    { "input", ConcreteDataTypeMatcher{ "TPC", "CLUSTERNATIVE" }, Lifetime::Timeframe },
  };
  bool sampledData = true;
  for ([[maybe_unused]] auto const& ref : InputRecordWalker(input, filter)) {
    sampledData = false;
    break;
  }
  if (sampledData) {
    filter = {
      { "sampled-data", ConcreteDataTypeMatcher{ "DS", "CLUSTERNATIVE" }, Lifetime::Timeframe },
    };
    LOG(INFO) << "Using sampled data.";
  }
  for (auto const& ref : InputRecordWalker(input, filter)) {
    auto const* sectorHeader = DataRefUtils::getHeader<o2::tpc::TPCSectorHeader*>(ref);
    if (sectorHeader == nullptr) {
      // FIXME: think about error policy
      LOG(ERROR) << "sector header missing on header stack";
      return clusterIndex;
    }
    const int sector = sectorHeader->sector();
    std::bitset<o2::tpc::constants::MAXSECTOR> sectorMask(sectorHeader->sectorBits);
    LOG(INFO) << "Reading TPC cluster data, sector mask is " << sectorMask;

    if (sector < 0) {
      if (operation < 0 && operation != sector) {
        // we expect the same operation on all inputs
        LOG(ERROR) << "inconsistent lane operation, got " << sector << ", expecting " << operation;
      } else if (operation == 0) {
        // store the operation
        operation = sector;
      }
      continue;
    }
    if ((validInputs & sectorMask).any()) {
      // have already data for this sector, this should not happen in the current
      // sequential implementation, for parallel path merged at the tracker stage
      // multiple buffers need to be handled
      throw std::runtime_error("can only have one cluster data set per sector");
    }
    validInputs |= sectorMask;
    inputrefs[sector].data = ref;
  }

  for (auto const& refentry : inputrefs) {
    //auto& sector = refentry.first;
    auto& ref = refentry.second.data;
    inputs.emplace_back(gsl::span(ref.payload, DataRefUtils::getPayloadSize(ref)));
  }

  std::unique_ptr<ClusterNative[]> clusterBuffer;
  ClusterNativeHelper::ConstMCLabelContainerViewWithBuffer clustersMCBufferDummy;
  std::vector<o2::dataformats::ConstMCLabelContainerView> mcInputsDummy;
  memset(&clusterIndex, 0, sizeof(clusterIndex));
  ClusterNativeHelper::Reader::fillIndex(clusterIndex, clusterBuffer, clustersMCBufferDummy,
                                         inputs, mcInputsDummy);

  return clusterIndex;
}

} // namespace o2::quality_control_modules::tpc
