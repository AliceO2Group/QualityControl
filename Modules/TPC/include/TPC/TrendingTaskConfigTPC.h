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
/// \file     TrendingTaskConfigTPC.h
/// \author   Marcel Lesch
/// \author   Cindy Mordasini
/// \author   Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_TRENDINGTASKCONFIGTPC_H
#define QUALITYCONTROL_TRENDINGTASKCONFIGTPC_H

#include "QualityControl/PostProcessingConfig.h"
#include <string>
#include <vector>

namespace o2::quality_control_modules::tpc
{
/// \brief  TrendingTask configuration structure, tuned for the need of the TPC
/// trending.
///
/// Configuration structure for the trending objects: the data sources to trend
/// and the plots to produce and publish on the QCG.
/// This configuration structure is specific to the TPC trending, as it allows
/// to input/output canvases, and slice TH objects along any axis.

struct TrendingTaskConfigTPC : public quality_control::postprocessing::PostProcessingConfig {
  /// \brief Constructors.
  TrendingTaskConfigTPC() = default;
  TrendingTaskConfigTPC(const std::string& name, const boost::property_tree::ptree& config);
  /// \brief Destructor.
  ~TrendingTaskConfigTPC() = default;

  struct Plot {
    std::string name;
    std::string title;
    std::string varexp;
    std::string selection;
    std::string option;
    std::string graphErrors;
    std::string graphYRange;
  };

  struct DataSource {
    std::string type;
    std::string path;
    std::string name;
    std::string reductorName;
    std::vector<std::vector<float>> axisDivision;
    std::string moduleName;
  };

  std::vector<Plot> plots;
  std::vector<DataSource> dataSources;
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_TRENDINGTASKCONFIGTPC_H