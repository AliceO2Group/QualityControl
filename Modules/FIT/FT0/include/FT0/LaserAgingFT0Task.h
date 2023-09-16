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
/// \file   LaserAgingFT0Task.h
/// \author My Name
///
#ifndef QC_MODULE_FT0_FT0LASERAGINGFT0TASK_H
#define QC_MODULE_FT0_FT0LASERAGINGFT0TASK_H

#include <memory>
#include <regex>
#include <type_traits>
#include <set>
#include <map>
#include <vector>
#include <array>
#include <boost/algorithm/string.hpp>

#include "TH1.h"
#include "TH2.h"
#include "TList.h"
#include "Rtypes.h"

#include <Framework/InputRecord.h>
#include "CommonConstants/LHCConstants.h"
#include "QualityControl/TaskInterface.h"
#include "QualityControl/QcInfoLogger.h"
#include "FT0Base/Constants.h"
#include "FT0Base/Geometry.h"
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"

#include "TH1.h"
#include "TH2.h"
#include "TList.h"
#include "Rtypes.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::ft0
{
class LaserAgingFT0Task final : public TaskInterface
{
 public:
  /// \brief Constructor
  LaserAgingFT0Task() = default;
  /// Destructor
  ~LaserAgingFT0Task() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

  constexpr static std::size_t sNCHANNELS_PM = o2::ft0::Constants::sNCHANNELS_PM;

 private:
  template <typename Param_t,
            typename = typename std::enable_if<std::is_floating_point<Param_t>::value ||
                                               std::is_same<std::string, Param_t>::value || (std::is_integral<Param_t>::value && !std::is_same<bool, Param_t>::value)>::type>

  auto parseParameters(const std::string& param, const std::string& del)
  {
    std::regex reg(del);
    std::sregex_token_iterator first{ param.begin(), param.end(), reg, -1 }, last;
    std::vector<Param_t> vecResult;
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

  std::unique_ptr<TH2F> mHistAmp2ADC0;
  std::unique_ptr<TH2F> mHistAmp2ADC1;
  std::map<unsigned int, TH2F*> mMapHistAmpVsBCADC0;
  std::map<unsigned int, TH2F*> mMapHistAmpVsBCADC1;
  std::set<unsigned int> mSetRefPMTChIDs;
  std::set<unsigned int> mSetAllowedChIDs;
  std::set<unsigned int> mSetAmpCut;

}; // namespace o2::quality_control_modules::ft0

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_FT0LASERAGINGFT0TASK_H
