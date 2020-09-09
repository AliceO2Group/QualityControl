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
/// \file    TrendingTaskITSThr.h
/// \author  Ivan Ravasenga on the structure from Piotr Konopka
///

#ifndef QUALITYCONTROL_TRENDINGTASKITSTHR_H
#define QUALITYCONTROL_TRENDINGTASKITSTHR_H

#include "ITS/TrendingTaskConfigITS.h"
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/Reductor.h"

#include <TAxis.h>
#include <TColor.h>
#include <TGraph.h>
#include <TLegend.h>
#include <TTree.h>
#include <memory>
#include <string>
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
/// \author Ivan Ravasenga on the structure from Piotr Konopka
class TrendingTaskITSThr : public PostProcessingInterface
{
 public:
  TrendingTaskITSThr() = default;
  ~TrendingTaskITSThr() override = default;

  void configure(std::string name,
                 const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistry&) override;
  void update(Trigger, framework::ServiceRegistry&) override;
  void finalize(Trigger, framework::ServiceRegistry&) override;

  // other functions (mainly style)
  void SetLegendStyle(TLegend* leg);
  void SetGraphStyle(TGraph* g, int col, int mkr);
  void SetGraphNameAndAxes(TGraph* g, std::string name, std::string title,
                           std::string xtitle, std::string ytitle, double ymin,
                           double ymax, std::vector<std::string> runlist);
  void PrepareLegend(TLegend* leg, int layer);

 private:
  struct MetaData {
    Int_t runNumber = 0;
  };

  void trendValues(repository::DatabaseInterface& qcdb);
  void storePlots(repository::DatabaseInterface& qcdb);
  void storeTrend(repository::DatabaseInterface& qcdb);

  TrendingTaskConfigITS mConfig;
  MetaData mMetaData;
  UInt_t mTime;
  std::unique_ptr<TTree> mTrend;
  std::vector<std::string> runlist;
  Int_t ntreeentries = 0;
  std::unordered_map<std::string, std::unique_ptr<Reductor>> mReductors;

  const int col[7] = { 1, 2, 3, 4, 5, 6, 7 };
  const int mkr[3] = { 8, 29, 34 };
  static constexpr int NLAYERS = 3;
  static constexpr int NTRENDSTHR = 3;
  const int nStaves[NLAYERS] = { 12, 16, 20 };
  const std::string trendtitles[NTRENDSTHR] = { "Threshold mean",
                                                "Threshold rms", "Dead pixel" };
  const std::string trendnames[NTRENDSTHR] = { "mean", "rms", "deadpix" };
  const std::string ytitles[NTRENDSTHR] = {
    "Threshold mean (DAC)", "Threshold rms (DAC)", "# Dead Pixels"
  };
};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_TRENDINGTASKITSTHR_H
