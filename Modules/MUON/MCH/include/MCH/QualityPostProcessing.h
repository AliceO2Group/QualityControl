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
/// \file    QualityPostProcessing.h
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Post-processing of the MCH quality flags
/// \since   21/06/2022
///

#ifndef QC_MODULE_MCH_PP_QUALITY_H
#define QC_MODULE_MCH_PP_QUALITY_H

#include "QualityControl/PostProcessingInterface.h"
#include "MCH/Helpers.h"
#include "MCH/PostProcessingConfigMCH.h"
#include <TH1F.h>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::muonchambers
{

/// \brief  A post-processing task which combines and trends the MCH quality flags
class QualityPostProcessing : public PostProcessingInterface
{
 public:
  QualityPostProcessing() = default;
  ~QualityPostProcessing() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

 private:
  /** create one histogram with relevant drawing options / stat box status.*/
  template <typename T>
  void publishHisto(T* h, bool statBox = false,
                    const char* drawOptions = "",
                    const char* displayHints = "");

  PostProcessingConfigMCH mConfig;

  std::string mAggregatedQualityName{ "Aggregator/MCHQuality" };
  std::string mMessageGood;
  std::string mMessageMedium;
  std::string mMessageBad;
  std::string mMessageNull;

  // CCDB object accessors
  std::vector<QualityObjectHelper> mCcdbObjects;
  std::vector<std::string> mCheckerMessages;

  // Quality histograms ===============================================

  std::unique_ptr<TH1F> mHistogramQualityDigits;
  std::unique_ptr<TH1F> mHistogramQualityPreclusters;
  std::unique_ptr<TH1F> mHistogramQualityMCH;
  std::map<std::string, std::unique_ptr<TH1F>> mHistogramsQuality;

  // Quality trends ===================================================

  std::unique_ptr<QualityTrendGraph> mTrendQualityDigits;
  std::unique_ptr<QualityTrendGraph> mTrendQualityPreclusters;
  std::unique_ptr<QualityTrendGraph> mTrendQualityMCH;
  std::map<std::string, std::unique_ptr<QualityTrendGraph>> mTrendsQuality;

  std::unique_ptr<TCanvas> mCanvasCheckerMessages;
};

template <typename T>
void QualityPostProcessing::publishHisto(T* h, bool statBox,
                                         const char* drawOptions,
                                         const char* displayHints)
{
  if (!statBox) {
    h->SetStats(0);
  }
  getObjectsManager()->startPublishing(h);
  if (drawOptions) {
    getObjectsManager()->setDefaultDrawOptions(h, drawOptions);
  }
  if (displayHints) {
    getObjectsManager()->setDisplayHint(h, displayHints);
  }
}

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MCH_PP_DIGITS_H
