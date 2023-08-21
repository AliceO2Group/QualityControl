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
/// \file    TrendingTaskITSError.h
/// \author  Mariia Selina on the structure from Piotr Konopka
///

#ifndef QUALITYCONTROL_TRENDINGTASKITSError_H
#define QUALITYCONTROL_TRENDINGTASKITSError_H

#include "ITS/TrendingTaskConfigITS.h"
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/Reductor.h"
#include "ITSMFTReconstruction/DecodingStat.h"

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

class TrendingTaskITSError : public PostProcessingInterface
{
 public:
  TrendingTaskITSError() = default;
  ~TrendingTaskITSError() override = default;

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

  const int colors[30] = { 1, 46, kAzure + 3, 807, 797, 827, 417, 841, 868, 867, 860, 602, 921, 874, 600, 820, 400, 840, 920, 616, 632, 432, 880, 416, 29, 900, kMagenta - 9, kOrange + 4, kGreen - 5, kPink - 9 };
  const int markers[30] = { 8, 20, 21, 22, 23, 25, 26, 27, 29, 30, 32, 33, 34, 39, 41, 43, 45, 47, 48, 49, 105, 107, 112, 114, 116, 117, 118, 119, 120, 121 };

  // const std::string trend_titles[o2::itsmft::GBTLinkDecodingStat::NErrorsDefined] = { "NoRDHAtStart", "PageNotStopped", "StopPageNotEmpty", "PageCounterDiscontinuity", "RDHvsGBTHPageCnt", "MissingGBTTrigger", "MissingGBTHeader", "MissingGBTTrailer", "NonZeroPageAfterStop", "DataForStoppedLane", "NoDataForActiveLane", "IBChipLaneMismatch", "CableDataHeadWrong", "InvalidActiveLanes", "PacketCounterJump", "PacketDoneMissing", "MissingDiagnosticWord", "GBTWordNotRecognized", "WrongeCableID", "WrongAlignmentWord", "MissingROF", "OldROF" };
};
} // namespace o2::quality_control::postprocessing
#endif // QUALITYCONTROL_TRENDINGTASKITSError_H
