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
/// \file    BigScreenCanvas.h
/// \author  Andrea Ferrero
/// \brief   Canvas showing the aggregated quality of configurable groups
///

#ifndef QUALITYCONTROL_BIGSCREENCANVAS_H
#define QUALITYCONTROL_BIGSCREENCANVAS_H

#include "QualityControl/Quality.h"
#include <TCanvas.h>
#include <TPaveText.h>
#include <TText.h>
#include <string>
#include <map>

namespace o2::quality_control_modules::common
{

class BigScreenCanvas : public TCanvas
{
 public:
  BigScreenCanvas(std::string name, std::string title, int nRows, int nCols, int borderWidth);
  ~BigScreenCanvas() = default;

  void add(std::string name, int index);
  void set(std::string name, o2::quality_control::core::Quality);
  void set(std::string name, int color, std::string text);
  void update();

 private:
  int mNRows{ 1 };
  int mNCols{ 1 };
  int mBorderWidth{ 5 };
  /// \brief colors associated to each quality state (Good/Medium/Bad/Null)
  std::map<std::string, int> mColors;
  /// \brief
  std::map<std::string, std::unique_ptr<TText>> mLabels;
  /// \brief
  std::map<std::string, std::unique_ptr<TPaveText>> mPaves;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_BIGSCREENCANVAS_H
