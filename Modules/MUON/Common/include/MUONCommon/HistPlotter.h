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

#ifndef QC_MODULE_MUON_COMMON_HIST_PLOTTER_H
#define QC_MODULE_MUON_COMMON_HIST_PLOTTER_H

#include "QualityControl/ObjectsManager.h"
#include <TObject.h>
#include <string>
#include <vector>

namespace o2::quality_control_modules::muon
{

class HistPlotter
{
 public:
  HistPlotter() = default;
  ~HistPlotter() = default;

 public:
  struct HistInfo {
    TObject* object;
    std::string drawOptions;
    std::string displayHints;
  };

  /** reset all histograms */
  void reset();

  std::vector<HistInfo>& histograms() { return mHistograms; }
  const std::vector<HistInfo>& histograms() const { return mHistograms; }

  virtual void publish(std::shared_ptr<o2::quality_control::core::ObjectsManager> objectsManager);

 private:
  std::vector<HistInfo> mHistograms;
};

} // namespace o2::quality_control_modules::muon

#endif
