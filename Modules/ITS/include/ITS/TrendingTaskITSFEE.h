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
/// \file    TrendingTaskITSFEE.h
/// \author  Artem Kotliarov on the structure from Piotr Konopka
///

#ifndef QUALITYCONTROL_TRENDINGTASKITSFEE_H
#define QUALITYCONTROL_TRENDINGTASKITSFEE_H

#include "ITS/TrendingTaskConfigITS.h"
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/Reductor.h"

#include <TAxis.h>
#include <TColor.h>
#include <TGraph.h>
#include <TMultiGraph.h>
#include <TLegend.h>
#include <TCanvas.h>
#include <TTree.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

namespace o2::quality_control::postprocessing
{

/// \brief  A post-processing task which trends values, stores them in a TTree
/// and produces plots.
///
/// A post-processing task which trends objects inside QC database (QCDB). It
/// extracts some values of one or multiple
/// objects using the Reductor classes, then stores them inside a TTree. One can
/// generate plots out the TTree - the
/// class exposes the TTree::Draw interface to the user. The TTree and plots are
/// stored in the QCDB. The class is
/// configured with configuration files, see Framework/postprocessing.json as an
/// example.
///

class TrendingTaskITSFEE : public PostProcessingInterface
{
 public:
  TrendingTaskITSFEE() = default;
  ~TrendingTaskITSFEE() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

 private:
  // other functions; mainly style of plots
  void SetLegendStyle(TLegend* legend, const std::string& name, bool isRun);
  void SetCanvasSettings(TCanvas* canvas);
  void SetGraphStyle(TGraph* graph, const std::string& name, const std::string& title, int col, int mkr);
  void SetGraphName(TMultiGraph* graph, const std::string& name, const std::string& title);
  void SetGraphAxes(TMultiGraph* graph, const std::string& xtitle,
                    const std::string& ytitle, bool isTime);
  void SetHistoAxes(TH1* hist, const std::vector<std::string>& runlist,
                    const double& Ymin, const double& Ymax);

  struct MetaData {
    Int_t runNumber = 0;
  };

  void trendValues(const Trigger& t, repository::DatabaseInterface& qcdb);
  void storePlots(repository::DatabaseInterface& qcdb);
  void storeTrend(repository::DatabaseInterface& qcdb);

  TrendingTaskConfigITS mConfig;
  MetaData mMetaData;
  UInt_t mTime;
  Int_t nEntries = 0;

  std::vector<std::string> runlist;
  std::unique_ptr<TTree> mTrend;
  std::unordered_map<std::string, std::unique_ptr<Reductor>> mReductors;

  const int colors[14] = { 1, 2, kAzure + 3, 807, 797, 827, 417, 841, 868, 867, 860, 602, 921, 874 };
  const int markers[14] = { 20, 21, 22, 24, 25, 26, 27, 29, 30, 32, 33, 34, 43, 47 };

  static constexpr int nFlags = 3;
  static constexpr int nITSparts = 4;
  static constexpr int nTriggers = 13;

  const std::string mTriggerType[nTriggers] = { "ORBIT", "HB", "HBr", "HC", "PHYSICS", "PP", "CAL", "SOT", "EOT", "SOC", "EOC", "TF", "INT" };
  const std::string trend_titles[nFlags] = { "Warnings", "Errors", "Faults" };
  const std::string itsParts[nITSparts] = { "IB", "ML", "OL", "Global" };
};
} // namespace o2::quality_control::postprocessing
#endif // QUALITYCONTROL_TRENDINGTASKITSFEE_H
