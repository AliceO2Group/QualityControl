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
#include <string>
#include <unordered_map>

namespace o2::quality_control_modules::common
{

struct BigScreenElement;

class BigScreenCanvas : public TCanvas
{
 public:
  BigScreenCanvas(std::string name, std::string title, int nRows, int nCols, int borderWidth);
  ~BigScreenCanvas() = default;

  /// \brief add a box in the canvas at a given index, with "name" displayed above the box
  ///
  /// The boxes are arranged in a regular grid of mNRows x mNCols in the canvas.
  /// The index proceeds from left to right and from top to bottom in the grid, starting from zero and up to (mNRows * mNCols - 1)
  void addBox(std::string boxName, int index);
  /// \brief set the text message and color of the box identified by "name".
  ///
  /// The color value follows the ROOT TColor indexing conventions (https://root.cern.ch/doc/master/classTColor.html)
  void setText(std::string boxName, int color, std::string text);
  /// \brief set the text message and color of the box identified by "name", based on the specified quality flag
  void setQuality(std::string boxName, o2::quality_control::core::Quality quality);
  /// \brief refresh the canvas and draw all the boxes and labels
  void update();

 private:
  /// \brief size of the grid of boxes in the canvas
  int mNRows{ 1 };
  int mNCols{ 1 };
  /// \brief size of the border around the colored boxes
  int mBorderWidth{ 5 };
  /// \brief empty space between the boxes
  float mPadding{ 0.2 };
  /// \brief offset of the label above the boxes
  float mLabelOffset{ 0.05 };
  /// \brief colors associated to each quality state (Good/Medium/Bad/Null)
  std::unordered_map<std::string, int> mColors;
  /// \brief elements (colored boxes + labels) displayed in the canvas
  std::unordered_map<std::string, std::shared_ptr<BigScreenElement>> mBoxes;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_BIGSCREENCANVAS_H
