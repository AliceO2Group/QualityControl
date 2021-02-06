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
/// \file   Utility.h
/// \author Thomas Klemenz
///

#ifndef QUALITYCONTROL_TPCUTILITY_H
#define QUALITYCONTROL_TPCUTILITY_H

#include "QualityControl/ObjectsManager.h"

#include "DataFormatsTPC/ClusterNative.h"
#include "Framework/ProcessingContext.h"

class TCanvas;

namespace o2::quality_control_modules::tpc
{

/// \brief Prepare canvases to publish DalPad data
/// This function creates canvases for CalPad data and registers them to be published on the QCG.
/// \param objectsManager ObjectsManager of the underlying PostProcessingInterface
/// \param canVec Vector which holds TCanvas pointers persisting for the entire runtime of the task
/// \param canvNames Names of the canvases
/// \param Optional std::map to set meta data for the publishing
auto addAndPublish = [](std::shared_ptr<o2::quality_control::core::ObjectsManager> objectsManager, auto& canVec, std::vector<std::string_view> canvNames, const std::map<std::string, std::string>& metaData = std::map<std::string, std::string>()) {
  for (const auto& canvName : canvNames) {
    canVec.emplace_back(std::make_unique<TCanvas>(canvName.data()));
    auto canvas = canVec.back().get();
    objectsManager->startPublishing(canvas);
    if (metaData.size() == 0) {
      objectsManager->addMetadata(canvas->GetName(), "custom", "42");
    } else {
      for (const auto& [key, value] : metaData) {
        objectsManager->addMetadata(canvas->GetName(), key, value);
      }
    }
  }
};

/// \brief Converts std::vector<std::unique_ptr<TCanvas>> to std::vector<TCanvas*>
/// \param input std::vector<std::unique_ptr<TCanvas>> to be converted to std::vector<TCanvas*>
/// \return std::vector<TCanvas*>
auto toVector = [](auto& input) {
  std::vector<TCanvas*> output;
  for (auto& in : input) {
    output.emplace_back(in.get());
  }
  return output;
};

/// \brief Converts CLUSTERNATIVE from InputRecord to ClusterNativeAccess
/// Convenience funtion to make native clusters accessible when receiving them from the DPL
/// \param input InputReconrd from the ProcessingContext
/// \return ClusterNativeAccess object for easy cluster access
o2::tpc::ClusterNativeAccess clusterHandler(o2::framework::InputRecord& input);
} //namespace o2::quality_control_modules::tpc
#endif //QUALITYCONTROL_TPCUTILITY_H