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
/// \file   ITSTrackCheck.h
/// \auhtor Artem Isakov
/// \author Jian Liu
///

#ifndef QC_MODULE_ITS_ITSTRACKCHECK_H
#define QC_MODULE_ITS_ITSTRACKCHECK_H

#include "QualityControl/CheckInterface.h"
#include <TLatex.h>

namespace o2::quality_control_modules::its
{

/// \brief  Check the clusters on track

class ITSTrackCheck : public o2::quality_control::checker::CheckInterface
{

 public:
  /// Default constructor
  ITSTrackCheck() = default;
  /// Destructor
  ~ITSTrackCheck() override = default;

  // Override interface
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;
  int getDigit(int number, int digit);
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
  float mEtaRatio = 0.1, mPhiRatio = 0.1;
  std::shared_ptr<TLatex> tInfo;
  std::shared_ptr<TLatex> tMessage[10];
  ClassDefOverride(ITSTrackCheck, 2);
};

} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSTrackCheck_H
