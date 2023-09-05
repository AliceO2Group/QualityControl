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
/// \file    ZDCRecDataPostProcessing.h
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Post-processing of the ZDC ADC and TDC plots
/// \since   30/08/2023
///

#ifndef QC_MODULE_ZDC_ZDCZDCRECDATAPP_H
#define QC_MODULE_ZDC_ZDCZDCRECDATAPP_H

#include "QualityControl/PostProcessingInterface.h"

#include <TH1F.h>
#include <memory>
#include <string>

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

using namespace o2::quality_control;
using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::zdc
{

struct MOHelper {
  MOHelper();
  MOHelper(std::string p, std::string n);

  bool update(o2::quality_control::repository::DatabaseInterface* qcdb,
              long timeStamp = -1,
              const o2::quality_control::core::Activity& activity = {});
  void setStartIme();
  long getTimeStamp() { return mTimeStamp; }
  template <typename T>
  T* get()
  {
    if (!mObject) {
      return nullptr;
    }
    if (!mObject->getObject()) {
      return nullptr;
    }

    // Get histogram object
    T* h = dynamic_cast<T*>(mObject->getObject());
    return h;
  }

  std::shared_ptr<o2::quality_control::core::MonitorObject> mObject;
  std::string mPath;
  std::string mName;
  uint64_t mTimeStart{ 0 };
  uint64_t mTimeStamp{ 0 };
};

/// \brief  A post-processing task which processes the ADC and TDC plots from ZDC
class ZDCRecDataPostProcessing : public PostProcessingInterface
{
 public:
  ZDCRecDataPostProcessing() = default;
  ~ZDCRecDataPostProcessing() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

 private:
  template <typename T>
  void publishHisto(T* h, bool statBox = false,
                    const char* drawOptions = "",
                    const char* displayHints = "");
  void createSummaryADCHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void createSummaryTDCHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void updateSummaryADCHistos(Trigger t, repository::DatabaseInterface* qcdb);
  void updateSummaryTDCHistos(Trigger t, repository::DatabaseInterface* qcdb);

  // CCDB object accessors
  std::map<size_t, MOHelper> mMOsADC;
  std::map<size_t, MOHelper> mMOsTDC;

  // Hit rate histograms ===============================================
  std::vector<std::string> mBinLabelsADC;
  std::vector<std::string> mBinLabelsTDC;
  std::unique_ptr<TH1F> mSummaryADCHisto;
  std::unique_ptr<TH1F> mSummaryTDCHisto;
};

template <typename T>
void ZDCRecDataPostProcessing::publishHisto(T* h, bool statBox,
                                            const char* drawOptions,
                                            const char* displayHints)
{
  h->LabelsOption("v");
  h->SetLineColor(kBlack);
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

} // namespace o2::quality_control_modules::zdc

#endif // QC_MODULE_ZDC_ZDCZDCRECDATAPP_H
