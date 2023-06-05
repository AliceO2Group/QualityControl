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

  /// @brief Make the histogram with the 2D representation of the fired boards
  /// @param name Histogram name
  /// @param title Histogram title
  /// @return Histogram 2D
  TH2F makeBoardMapHisto(std::string name, std::string title) const;

  /// @brief Count the number of fired strips
  /// @param col Column Data
  /// @param cathode Bending (0) or Non-bending (1) plane
  /// @return Number of fired strips
  unsigned long countDigits(const o2::mid::ColumnData& col, int cathode = -1) const;

  /// @brief Fill the 1D histogram with the number of time a strip was fired
  /// @param col Column Data
  /// @param histo Pointer to the histogram
  void fillStripHisto(const o2::mid::ColumnData& col, TH1* histo) const;

  /// @brief Fill the 2D representation of the fired strips/boards from the 1D histogram
  /// @param stripHisto Input 1D histogram
  /// @param stripHistosB Array with the 2D representation of the fired strips in the bending plane per chamber
  /// @param stripHistosNB Array with the 2D representation of the fired strips in the non-bending plane per chamber
  /// @param boardHistos Array with the 2D representation of the fired boards per chamber. Last histogram is the sum of the previous four
  void fillMapHistos(const TH1* stripHisto, std::array<std::unique_ptr<TH2F>, 4>& stripHistosB, std::array<std::unique_ptr<TH2F>, 4>& stripHistosNB, std::array<std::unique_ptr<TH2F>, 5>& boardHistos) const;

  /// @brief Fill the 2D representation of the fired strips from the 1D histogram
  /// @param stripHisto Input 1D histogram
  /// @param stripHistosB Array with the 2D representation of the fired strips in the bending plane per chamber
  /// @param stripHistosNB Array with the 2D representation of the fired strips in the non-bending plane per chamber
  void fillMapHistos(const TH1* stripHisto, std::array<std::unique_ptr<TH2F>, 4>& stripHistosB, std::array<std::unique_ptr<TH2F>, 4>& stripHistosNB) const;

  /// @brief Fill the 2D representation of the fired strips/boards from the 1D histogram for a specific chamber
  /// @param stripHisto Input 1D histogram
  /// @param stripHistosB 2D representation of the fired strips in the bending plane
  /// @param stripHistosNB 2D representation of the fired strips in the non-bending plane
  /// @param boardHistos  2D representation of the fired boards per chamber
  /// @param chamber Selected chamber
  void fillMapHistos(const TH1* stripHisto, TH2* stripHistosB, TH2* stripHistosNB, TH2* boardHistos, int chamber) const;

  struct StripInfo {
    int deId;     ///< Detection element ID
    int columnId; ///< Column ID
    int lineId;   ///< Line ID
    int stripId;  ///< Strip ID
    int cathode;  ///< Bending (0) or Non-bending (1) plane
    int xwidth;   ///< Width X
    int ywidth;   ///< Width y
  };

  struct ColumnInfo {
    int firstLine; ///< First line in column
    int lastLine;  ///< Last line in column
    int nStripsNB; ///< Number of strips in the NB plane
  };

 private:
  std::unordered_map<int, int> mStripsMap{};  ///! Map from id to index
  std::vector<StripInfo> mStripsInfo;         ///! Strips info
  std::array<ColumnInfo, 72 * 7> mColumnInfo; ///! Column info
  void fillMapHistos(const StripInfo& info, TH2* stripHistoB, TH2* stripHistoNB, TH2* boardHisto, int wgt) const;

  inline int getColumnIdx(int columnId, int deId) const { return 7 * deId + columnId; }
};

} // namespace o2::quality_control_modules::mid

#endif