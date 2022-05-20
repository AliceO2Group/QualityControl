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
#include "TPCBase/CalDet.h"
#include "TPCBase/Painter.h"

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

void fillCanvases(const o2::tpc::CalDet<float>& calDet, std::vector<std::unique_ptr<TCanvas>>& canvases, const std::unordered_map<std::string, std::string>& params, const std::string paramName)
{
  const std::string parNBins = paramName + "NBins";
  const std::string parXMin = paramName + "XMin";
  const std::string parXMax = paramName + "XMax";
  int nbins = 300;
  float xmin = 0;
  float xmax = 0;
  const auto last = params.end();
  const auto itNBins = params.find(parNBins);
  const auto itXMin = params.find(parXMin);
  const auto itXMax = params.find(parXMax);
  if ((itNBins == last) || (itXMin == last) || (itXMax == last)) {
    LOGP(warning, "missing parameter {}, {} or {}, falling back to auto scaling", parNBins, parXMin, parXMax);
    LOGP(warning, "Please add '{}': '<value>', '{}': '<value>', '{}': '<value>' to the 'taskParameters'.", parNBins, parXMin, parXMax);
  } else {
    nbins = std::stoi(itNBins->second);
    xmin = std::stof(itXMin->second);
    xmax = std::stof(itXMax->second);
  }
  auto vecPtr = toVector(canvases);
  o2::tpc::painter::makeSummaryCanvases(calDet, nbins, xmin, xmax, false, &vecPtr);
}

void clearCanvases(std::vector<std::unique_ptr<TCanvas>>& canvases)
{
  for (const auto& canvas : canvases) {
    canvas->Clear();
  }
}

std::unique_ptr<o2::tpc::internal::getWorkflowTPCInput_ret> clusterHandler(o2::framework::InputRecord& inputs, int verbosity, unsigned long tpcSectorMask)
{
  auto retVal = std::make_unique<o2::tpc::internal::getWorkflowTPCInput_ret>();

  std::vector<o2::framework::InputSpec> filter = {
    { "input", o2::framework::ConcreteDataTypeMatcher{ "TPC", "CLUSTERNATIVE" }, o2::framework::Lifetime::Timeframe },
  };
  bool sampledData = true;
  for ([[maybe_unused]] auto const& ref : o2::framework::InputRecordWalker(inputs, filter)) {
    sampledData = false;
    break;
  }
  if (sampledData) {
    filter = {
      { "sampled-data", o2::framework::ConcreteDataTypeMatcher{ "DS", "CLUSTERNATIVE" }, o2::framework::Lifetime::Timeframe },
    };
    LOG(info) << "Using sampled data.";
  }

  unsigned long recvMask = 0;
  for (auto const& ref : o2::framework::InputRecordWalker(inputs, filter)) {
    auto const* sectorHeader = o2::framework::DataRefUtils::getHeader<o2::tpc::TPCSectorHeader*>(ref);
    if (sectorHeader == nullptr) {
      throw std::runtime_error("sector header missing on header stack");
    }
    const int sector = sectorHeader->sector();
    if (sector < 0) {
      continue;
    }
    if (recvMask & sectorHeader->sectorBits) {
      throw std::runtime_error("can only have one cluster data set per sector");
    }
    recvMask |= (sectorHeader->sectorBits & tpcSectorMask);
    retVal->internal.inputrefs[sector].data = ref;
  }
  if (recvMask != tpcSectorMask) {
    throw std::runtime_error("Incomplete set of clusters/digits received");
  }

  for (auto const& refentry : retVal->internal.inputrefs) {
    auto& sector = refentry.first;
    auto& ref = refentry.second.data;
    if (ref.payload == nullptr) {
      // skip zero-length message
      continue;
    }
    if (!(tpcSectorMask & (1ul << sector))) {
      continue;
    }
    if (refentry.second.labels.header != nullptr && refentry.second.labels.payload != nullptr) {
      retVal->internal.mcInputs.emplace_back(o2::dataformats::ConstMCLabelContainerView(inputs.get<gsl::span<char>>(refentry.second.labels)));
    }
    retVal->internal.inputs.emplace_back(gsl::span(ref.payload, o2::framework::DataRefUtils::getPayloadSize(ref)));
    if (verbosity > 1) {
      LOG(info) << "received " << *(ref.spec) << ", size " << o2::framework::DataRefUtils::getPayloadSize(ref) << " for sector " << sector;
    }
  }

  memset(&retVal->clusterIndex, 0, sizeof(retVal->clusterIndex));
  o2::tpc::ClusterNativeHelper::Reader::fillIndex(retVal->clusterIndex, retVal->internal.clusterBuffer, retVal->internal.clustersMCBuffer, retVal->internal.inputs, retVal->internal.mcInputs, tpcSectorMask);

  return std::move(retVal);
}

} // namespace o2::quality_control_modules::tpc
