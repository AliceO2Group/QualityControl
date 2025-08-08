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
/// \file    QualityAggregatorTask.h
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Post-processing of the MCH summary qualities
/// \since   21/06/2022
///

#ifndef QC_MODULE_MCH_PP_QUALITY_H
#define QC_MODULE_MCH_PP_QUALITY_H

#include "QualityControl/PostProcessingInterface.h"
#include "CCDB/CcdbApi.h"

#include <set>

using namespace o2::framework;

using namespace o2::quality_control;
using namespace o2::quality_control::postprocessing;

class TH2F;

namespace o2::quality_control_modules::muonchambers
{

/// \brief  A post-processing task which processes and trends MCH pre-clusters and produces plots.
class QualityAggregatorTask : public PostProcessingInterface
{
 public:
  using CcdbApi = o2::ccdb::CcdbApi;

  QualityAggregatorTask() = default;
  ~QualityAggregatorTask() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

 private:
  CcdbApi mAPI;
  std::string mCCDBpath{ "http://ccdb-test.cern.ch:8080" }; // CCDB path

  std::string mObjectPathBadDE{ "MCH/Calib/BadDE" };
  std::string mObjectPathBadSOLAR{ "MCH/Calib/BadSOLAR" };

  std::vector<std::string> mDEPlotPaths;
  std::optional<std::set<int>> mPreviousBadDEs;

  std::vector<std::string> mSOLARPlotPaths;
  std::optional<std::set<int>> mPreviousBadSolarBoards;

  std::unique_ptr<TH2F> mHistogramQualityPerDE;    ///< quality flags for each DE, to be filled by checker task
  std::unique_ptr<TH2F> mHistogramQualityPerSolar; ///< quality flags for each SOLAR, to be filled by checker task
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_PP_QUALITY_H
