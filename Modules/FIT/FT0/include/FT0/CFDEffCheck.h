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
/// \file   CFDEffCheck.h
/// \author Sebastian Bysiak sbysiak@cern.ch
///

#ifndef QC_MODULE_FT0_FT0CFDEFFCHECK_H
#define QC_MODULE_FT0_FT0CFDEFFCHECK_H

#include <regex>
#include <numeric>

#include "QualityControl/CheckInterface.h"

#include "FT0Base/Geometry.h"
#include "DataFormatsFIT/DeadChannelMap.h"

namespace o2::quality_control_modules::ft0
{

/// \brief checks if CFD efficiency is below threshold
/// \author Sebastian Bysiak sbysiak@cern.ch
class CFDEffCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  CFDEffCheck() = default;
  ~CFDEffCheck() override = default;

  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  ClassDefOverride(CFDEffCheck, 2);

 private:
  template <typename Param_t,
            typename = typename std::enable_if<std::is_floating_point<Param_t>::value ||
                                               std::is_same<std::string, Param_t>::value || (std::is_integral<Param_t>::value && !std::is_same<bool, Param_t>::value)>::type>
  auto parseParameters(const std::string& param, const std::string& del)
  {
    std::regex reg(del);
    std::sregex_token_iterator first{ param.begin(), param.end(), reg, -1 }, last;
    std::vector<Param_t> vecResult;
    if (std::find_if(param.begin(), param.end(), ::isdigit) == param.end()) {
      return vecResult;
    }
    for (auto it = first; it != last; it++) {
      if constexpr (std::is_integral<Param_t>::value && !std::is_same<bool, Param_t>::value) {
        vecResult.push_back(std::stoi(*it));
      } else if constexpr (std::is_floating_point<Param_t>::value) {
        vecResult.push_back(std::stod(*it));
      } else if constexpr (std::is_same<std::string, Param_t>::value) {
        vecResult.push_back(*it);
      }
    }
    return vecResult;
  }

  constexpr static std::size_t sNCHANNELS = o2::ft0::Geometry::Nchannels;
  o2::fit::DeadChannelMap* mDeadChannelMap;
  std::string mDeadChannelMapStr;
  std::string mPathDeadChannelMap;
  float mThreshWarning;
  float mThreshError;
  int mNumWarnings;
  int mNumErrors;
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_FT0CFDEFFCHECK_H
