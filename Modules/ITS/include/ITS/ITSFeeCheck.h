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
/// \file   ITSFeeCheck.h
/// \auhtor Liang Zhang
/// \author Jian Liu
/// \author Antonio Palasciano
///

#ifndef QC_MODULE_ITS_ITSFEECHECK_H
#define QC_MODULE_ITS_ITSFEECHECK_H

#include "QualityControl/CheckInterface.h"
#include <TH2Poly.h>
#include <TLatex.h>
#include <string>
#include <vector>
#include <sstream>

namespace o2::quality_control_modules::its
{

/// \brief  Check the FAULT flag for the lanes

class ITSFeeCheck : public o2::quality_control::checker::CheckInterface
{

 public:
  /// Default constructor
  ITSFeeCheck() = default;
  /// Destructor
  ~ITSFeeCheck() override = default;

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
  bool checkReason(Quality checkResult, TString text);

 private:
  ClassDefOverride(ITSFeeCheck, 2);

  static constexpr int NLayer = 7;
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  const int NLanePerStaveLayer[NLayer] = { 9, 9, 9, 16, 16, 28, 28 };
  const int NStaves[NLayer] = { 12, 16, 20, 24, 30, 42, 48 };
  static constexpr int NFlags = 3;
  static constexpr int NTrg = 13;
  const double minTextPosY[NLayer] = { 0.45, 0.41, 0.37, 0.23, 0.20, 0.16, 0.13 }; // Text y coordinates in TH2Poly
  std::string mLaneStatusFlag[NFlags] = { "WARNING", "ERROR", "FAULT" };
  const int laneMax[NLayer] = { 108, 144, 180, 384, 480, 1176, 1344 };

  std::shared_ptr<TLatex> tInfo;
  std::shared_ptr<TLatex> tInfoLayers[7];
  std::shared_ptr<TLatex> tInfoIB;
  std::shared_ptr<TLatex> tInfoML;
  std::shared_ptr<TLatex> tInfoOL;
  std::shared_ptr<TLatex> tInfoPL[10];
  std::shared_ptr<TLatex> tInfoSummary;
  std::shared_ptr<TLatex> tInfoTrg[13];

  std::string skipbinstrg = "";
  std::string skipfeeids = "";
  int maxbadchipsIB = 2;
  int maxbadlanesML = 4;
  int maxbadlanesOL = 7;
  double maxfractionbadlanes = 0.1;
};

} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSFeeCheck_H
