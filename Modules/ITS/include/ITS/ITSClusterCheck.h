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
/// \file   ITSClusterCheck.h
/// \author Artem Isakov
/// \auhtor Liang Zhang
/// \author Jian Liu
///

#ifndef QC_MODULE_ITS_ITSCLUSTERCHECK_H
#define QC_MODULE_ITS_ITSCLUSTERCHECK_H

#include "QualityControl/CheckInterface.h"
#include <TLatex.h>
#include <TLine.h>
#include <TH2Poly.h>
#include <vector>
#include <string>
#include <sstream>

namespace o2::quality_control_modules::its
{

/// \brief  Check the average cluster size

class ITSClusterCheck : public o2::quality_control::checker::CheckInterface
{

 public:
  /// Default constructor
  ITSClusterCheck() = default;
  /// Destructor
  ~ITSClusterCheck() override = default;

  // Override interface
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;
  template <typename T>
  std::vector<T> convertToArray(std::string input)
  {

    std::istringstream ss{ input };

    std::vector<T> result;
    std::string token;

    while (std::getline(ss, token, ',')) {

      if constexpr (std::is_same_v<T, int>) {
        result.push_back(std::stoi(token));
      } else if constexpr (std::is_same_v<T, std::string>) {
        result.push_back(token);
      }
    }
    return result;
  }

 private:
  ClassDefOverride(ITSClusterCheck, 2);

  std::shared_ptr<TLatex> tInfoSummary;
  std::shared_ptr<TLatex> tInfo;
  std::shared_ptr<TLine> tInfoLine;
  std::shared_ptr<TLatex> msg;
  std::shared_ptr<TLatex> text[14];
  std::shared_ptr<TLatex> text2[14];
  static constexpr int NLayer = 7;
  const int mNStaves[NLayer] = { 12, 16, 20, 24, 30, 42, 48 };
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  float maxcluocc[NLayer] = { 5, 4, 3, 2, 1, 1, 1 };
  double MaxEmptyLaneFraction = 0.1;
};

} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSClusterCheck_H
