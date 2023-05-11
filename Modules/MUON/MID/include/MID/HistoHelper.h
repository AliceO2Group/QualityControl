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

/// \file   HistoHelper.h
/// \author Diego Stocco

#ifndef QC_MODULE_MID_HISTOHELPER_H
#define QC_MODULE_MID_HISTOHELPER_H

#include <string>
#include <unordered_map>

#include "QualityControl/Quality.h"

class TH1;
class TLatex;

namespace o2::quality_control_modules::mid
{
class HistoHelper
{

 public:
  /// @brief Default ctr
  HistoHelper();

  /// Default destructor
  ~HistoHelper() = default;

  /// @brief Sets the number of analyzed TFs
  /// @param nTFs Number of analyzed TFs
  void setNTFs(unsigned long int nTFs) { mNTFs = nTFs; }

  /// @brief Gets number of analyzed TFs
  /// @return Number of analyzed TFs
  unsigned long int getNTFs() const { return mNTFs; }

  /// @brief Sets the number of orbits per TF
  /// @param nOrbitsPerTF number of orbits per TF
  void setNOrbitsPerTF(unsigned long int nOrbitsPerTF) { mNOrbitsPerTF = nOrbitsPerTF; }

  /// @brief Convert the number of analyzed TFs in seconds
  /// @return Duration in seconds of the analyzed TFs
  double getNTFsAsSeconds() const { return mNTFs * mNOrbitsPerTF * 0.0000891; }

  /// @brief Scale the histogram to the inverse of the analyzed TF duration (result in Hz)
  /// @param histo Histogram to normalize
  void normalizeHistoToHz(TH1* histo) const;

  /// @brief Scale the histogram to the inverse of the analyzed TF duration (result in kHz)
  /// @param histo Histogram to normalize
  void normalizeHistoTokHz(TH1* histo) const;

  /// @brief Updates the histogram title
  /// @param histo Histogram to update
  /// @param append Text to append
  void updateTitle(TH1* histo, std::string suffix) const;

  /// @brief Returns the current time
  /// @return Current time
  std::string getCurrentTime() const;

  /// @brief Adds a TLatex to the histogram list of functions
  /// @param histo Histogram to modify
  /// @param xmin Bottom-left x position
  /// @param ymin Bottom-left y position
  /// @param color Text color
  /// @param text Text
  void addLatex(TH1* histo, double xmin, double ymin, int color, std::string text) const;

  /// @brief Gets the color for this quality
  /// @param quality QC Quality
  /// @return Color as integer
  int getColor(const o2::quality_control::core::Quality& quality) const;

  /// @brief Add number of TFs in the title
  /// @param histo Histogram to update
  void updateTitleWithNTF(TH1* histo) const;

 private:
  /// @brief Normalizes the histogram to the inverse of the analyzed TF duration
  /// @param histo Histogram to normalize
  /// @param scale Additional scaling factor
  bool normalizeHisto(TH1* histo, double scale) const;

  unsigned long int mNTFs = 0;          ///! Number of TFs
  unsigned long int mNOrbitsPerTF = 32; ///! Number of orbits per TF
  std::unordered_map<unsigned int, int> mColors;
};

} // namespace o2::quality_control_modules::mid

#endif