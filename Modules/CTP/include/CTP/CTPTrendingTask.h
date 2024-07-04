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
/// \file     CTPTrendingTask.h
/// \author   Lucia Anna Tarasovicova
///

#ifndef QUALITYCONTROL_CTPTTRENDINGTASK_H
#define QUALITYCONTROL_CTPTTRENDINGTASK_H

#include "QualityControl/PostProcessingInterface.h"
#include "CTP/TrendingConfigCTP.h"
#include <DataFormatsCTP/Configuration.h>
#include "CTP/TH1ctpReductor.h"

#include <string>
#include <TTree.h>

namespace o2::quality_control::repository
{
class DatabaseInterface;
} // namespace o2::quality_control::repository

namespace o2::quality_control::postprocessing
{
/// \brief  A post-processing task for trending ctp input and class rates
///

class CTPTrendingTask : public PostProcessingInterface
{
 public:
  /// \brief Constructor.
  CTPTrendingTask() = default;
  /// \brief Destructor.
  ~CTPTrendingTask() override = default;

  /// \brief Post-processing methods inherited from 'PostProcessingInterface'.
  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;
  void initCTP(Trigger& t);

 private:
  struct MetaData {
    Int_t runNumber = 0;
  };

  void trendValues(const Trigger& t, quality_control::repository::DatabaseInterface& qcdb);
  void generatePlots();

  TrendingConfigCTP mConfig;
  MetaData mMetaData;
  UInt_t mTime; // this is not a specific time, used for x axis in trending plots, this name is used in all other modules, please suggest a new name if not appropriate

  std::string mInputNamesDefault[5] = { "TVX", "VBA", "DMC", "EMC", "PH0" };                                                                                                                                                                                                                                                                                                             // ctp inputs to be trended by default, unless modified in config.json
  std::string ctpinputs[49] = { " T0A", " T0C", " TVX", " TSC", " TCE", " VBA", " VOR", " VIR", " VNC", " VCH", "11", "12", " UCE", "DMC", " USC", " UVX", " U0C", " U0A", "COS", "LAS", "EMC", " PH0", "23", "24", "ZED", "ZNC", "PHL", "PHH", "PHM", "30", "31", "32", "33", "34", "35", "36", "EJ1", "EJ2", "EG1", "EG2", "DJ1", "DG1", "DJ2", "DG2", "45", "46", "47", "48", "49" }; // all ctp input names
  std::string mClassNamesDefault[5] = { "CMTVX-B-NOPF", "CMVBA-B-NOPF", "CTVXDMC-B-NOPF-EMC", "CTVXEMC-B-NOPF-EMC", "CTVXPH0-B-NOPF-PHSCPV" };                                                                                                                                                                                                                                           // ctp classes to be trended by default, unless modified in config.json
  std::string mClassNames[5] = { "", "", "", "", "" };                                                                                                                                                                                                                                                                                                                                   // the trended ctp classes will be filled in this array, either default or from config
  std::string mInputNames[5] = { "", "", "", "", "" };                                                                                                                                                                                                                                                                                                                                   // the trended ctp inputs will be filled in this array, either default or from config
  const int mNumberOfClasses = 5;                                                                                                                                                                                                                                                                                                                                                        // number of thrended ctp classes
  const int mNumberOfInputs = 5;                                                                                                                                                                                                                                                                                                                                                         // number of thrended ctp inputs
  int mClassIndex[5] = { 65, 65, 65, 65, 65 };                                                                                                                                                                                                                                                                                                                                           // indices of trended ctp classes, found in CTPconfig
  int mInputIndex[5] = { 49, 49, 49, 49, 49 };                                                                                                                                                                                                                                                                                                                                           // indices of trended ctp inputs, found in CTPconfig
  bool mCTPconfigFound = false;                                                                                                                                                                                                                                                                                                                                                          // bool telling whether the CTPconfing was already found

  std::map<std::string, TObject*> mPlots;

  std::unique_ptr<TTree> mTrend;
  std::unordered_map<std::string, std::unique_ptr<o2::quality_control_modules::ctp::TH1ctpReductor>> mReductors;
};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_CTPTTRENDINGTASK_H
