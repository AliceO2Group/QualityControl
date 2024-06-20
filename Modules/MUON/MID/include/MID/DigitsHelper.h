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

/// \file   DigitsHelper.h
/// \author Diego Stocco

#ifndef QC_MODULE_MID_DIGITSHELPER_H
#define QC_MODULE_MID_DIGITSHELPER_H

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include "DataFormatsMID/ColumnData.h"

class TH1F;
class TH2F;
class TH1;
class TH2;

namespace o2::quality_control_modules::mid
{
class DigitsHelper
{
 public:
  /// Default ctr
  DigitsHelper();

  /// Default destructor
  ~DigitsHelper() = default;

  /// @brief Make the 1D histogram with the number of time a strip was fired
  /// @param name Histogram name
  /// @param title Histogram title
  /// @return Histogram 1D
  TH1F makeStripHisto(const std::string name, const std::string title) const;

  /// @brief Make the histogram with the 2D representation of the fired strips
  /// @param name Histogram name
  /// @param title Histogram title
  /// @param cathode Bending (0) or Non-bending (1) plane
  /// @return Histogram 2D
  TH2F makeStripMapHisto(std::string name, std::string title, int cathode) const;

  /// @brief Make 4 histograms with the 2D representation of the fired strips per chamber
  /// @param name Base histogram name
  /// @param title Base histogram title
  /// @param cathode Bending (0) or Non-bending (1) plane
  /// @return Array of unique pointer to histograms
  std::array<std::unique_ptr<TH2F>, 4> makeStripMapHistos(std::string name, std::string title, int cathode) const;

  /// @brief Make the histogram with the 2D representation of the fired boards
  /// @param name Histogram name
  /// @param title Histogram title
  /// @return Histogram 2D
  TH2F makeBoardMapHisto(std::string name, std::string title) const;

  /// @brief Make 4 histograms with the 2D representation of the fired boards per chamber
  /// @param name Base histogram name
  /// @param title Base histogram title
  /// @param cathode Bending (0) or Non-bending (1) plane
  /// @return Array of unique pointer to histograms
  std::array<std::unique_ptr<TH2F>, 4> makeBoardMapHistos(std::string name, std::string title) const;

  /// @brief Count the number of fired strips
  /// @param col Column Data
  /// @param cathode Bending (0) or Non-bending (1) plane
  /// @return Number of fired strips
  unsigned long countDigits(const o2::mid::ColumnData& col, int cathode = -1) const;

  /// @brief Fill the 1D histogram with the number of time a strip was fired
  /// @param col Column Data
  /// @param histo Pointer to the histogram
  void fillStripHisto(const o2::mid::ColumnData& col, TH1* histo) const;

  /// @brief Fill the 2D representation of the fired boards from the 1D strip histogram
  /// @param histo Input 1D histogram with fired strips
  /// @param histosB Array with the 2D representation of the fired boards per chamber. Last histogram is the sum of the previous four
  void fillBoardMapHistosFromStrips(const TH1* histo, std::array<std::unique_ptr<TH2F>, 4>& histosB, std::array<std::unique_ptr<TH2F>, 4>& histosNB) const;

  /// @brief Fill the 2D representation of the fired strips from the 1D histogram
  /// @param histo Input 1D histogram with fired strips
  /// @param histosB Array with the 2D representation of the fired strips in the bending plane per chamber
  /// @param histosNB Array with the 2D representation of the fired strips in the non-bending plane per chamber
  void fillStripMapHistos(const TH1* histo, std::array<std::unique_ptr<TH2F>, 4>& stripHistosB, std::array<std::unique_ptr<TH2F>, 4>& stripHistosNB) const;

  struct MapInfo {
    int cathode = 0;         ///! Cathode
    int chamber = 0;         ///! Chamber
    std::vector<int> bins{}; ///! Bins in the 2D map histograms
  };

  struct ColumnInfo {
    int firstLine; ///! First line in column
    int lastLine;  ///! Last line in column
    int nStripsNB; ///! Number of strips in the NB plane
  };

 private:
  std::unordered_map<int, int> mStripsMap{}; ///! Strip id to strip idx

  std::vector<MapInfo> mStripIdxToStripMap{}; ///! Strip index to strip map bins
  std::vector<MapInfo> mStripIdxToBoardMap{}; ///! Strip index to board map bins

  std::array<ColumnInfo, 72 * 7> mColumnInfo; ///! Column info

  /// @brief Initializes inner maps
  void initMaps();

  /// @brief Fills one bin in a quick way
  /// @param ibin Bin to fill
  /// @param wgt Weight
  /// @param histo Histogram to fill
  void FillBin(TH1* histo, int ibin, double wgt = 1.) const;

  /// @brief Fill the 2D map histogram from the 1D histogram
  /// @param histo 1D histogram
  /// @param histoMapB 2D map histogram for the bending plane
  /// @param histoMapNB 2D map histogram for the non-bending plane
  /// @param infoMap Correspondence between histogram bins
  void fillMapHistos(const TH1* histo, std::array<std::unique_ptr<TH2F>, 4>& histoMapB, std::array<std::unique_ptr<TH2F>, 4>& histoMapNB, const std::vector<MapInfo>& infoMap) const;

  inline int getColumnIdx(int columnId, int deId) const { return 7 * deId + columnId; }
};

} // namespace o2::quality_control_modules::mid

#endif