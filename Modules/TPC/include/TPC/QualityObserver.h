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
/// \file     QualityObserver.h
/// \author   Marcel Lesch
///

#ifndef QUALITYCONTROL_QUALITYOBSERVER_H
#define QUALITYCONTROL_QUALITYOBSERVER_H

#include "QualityControl/PostProcessingInterface.h"

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <TCanvas.h>

namespace o2::quality_control::repository
{
class DatabaseInterface;
} // namespace o2::quality_control::repository

using namespace o2::quality_control::postprocessing;
namespace o2::quality_control_modules::tpc
{
/// \brief  A post-processing task to generate an overview of multiple groups of qualities
///
/// A post-processing task which generates an overview panel for multiple groups of qualities.
/// It extracts the quality of the tasks/QOs passed to the object and creates a TPaveText
/// as ouput.
///

class QualityObserver : public PostProcessingInterface
{
 public:
  /// \brief Constructor.
  QualityObserver() = default;
  /// \brief Destructor.
  ~QualityObserver() final = default;

  /// \brief Post-processing methods inherited from 'PostProcessingInterface'.
  void configure(std::string name, const boost::property_tree::ptree& config) final;
  void initialize(Trigger, framework::ServiceRegistry&) final;
  void update(Trigger, framework::ServiceRegistry&) final;
  void finalize(Trigger, framework::ServiceRegistry&) final;

  struct Config {
    std::string GroupTitle;
    std::string Path;
    std::vector<std::string> QO;
    std::vector<std::string> QOTitle;
  };

 private:
  /// \brief Method to get the qualities of the QOs
  void getQualities(const Trigger& t, o2::quality_control::repository::DatabaseInterface&);
  /// \brief Method to create and publish the overview panel
  void generatePanel();

  std::vector<Config> mConfig;
  std::string mObserverName;
  std::map<std::string, std::vector<std::string>> Qualities;
  std::map<std::string, int> mColors;
  TCanvas* mCanvas = nullptr;
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_QUALITYOBSERVER_H
