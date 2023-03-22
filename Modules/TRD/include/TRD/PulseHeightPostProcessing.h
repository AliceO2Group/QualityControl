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
/// \file    TRDTrending.h
/// \author    based on  Piotr Konopka work
///

#ifndef QUALITYCONTROL_PULSEHEIGHTPOSTPROCESSING_H
#define QUALITYCONTROL_PULSEHEIGHTPOSTPROCESSING_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/Reductor.h"
#include <memory>
#include <unordered_map>
#include <TTree.h>
#include <TCanvas.h>
#include <array>
#include <TH1.h>
#include <TProfile2D.h>
#include <TProfile.h>
#include "CCDB/CcdbApi.h"
namespace o2::quality_control::repository
{
class DatabaseInterface;
}

namespace o2::quality_control::postprocessing
{

class PulseHeightPostProcessing : public PostProcessingInterface
{
 public:
  PulseHeightPostProcessing() = default;
  ~PulseHeightPostProcessing() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;
  void PlotPulseHeightPerChamber();

  o2::ccdb::CcdbApi mCdbph;
  long mTimestamps;
  std::string mHost;
  std::string mPath;
  std::array<std::shared_ptr<TCanvas>, 18> cc;
  TProfile* h2[540];
  UInt_t mTime;
  std::vector<std::string> runlist;
};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_TRDTRENDING_H
