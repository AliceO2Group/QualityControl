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
/// \file   ITSDecodingErrorCheck.h
/// \auhtor Zhen Zhang
///

#ifndef QC_MODULE_ITS_ITSDECODINGERRORCHECK_H
#define QC_MODULE_ITS_ITSDECODINGERRORCHECK_H

#include "QualityControl/CheckInterface.h"
#include <TH2Poly.h>
#include <TLatex.h>
#include <string>
#include <vector>
#include <sstream>

namespace o2::quality_control_modules::its
{

/// \brief  Check the FAULT flag for the lanes

class ITSDecodingErrorCheck : public o2::quality_control::checker::CheckInterface
{

 public:
  /// Default constructor
  ITSDecodingErrorCheck() = default;
  /// Destructor
  ~ITSDecodingErrorCheck() override = default;

  // Override interface
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;
  std::vector<int> convertToIntArray(std::string input);

 private:
  ClassDefOverride(ITSDecodingErrorCheck, 1);

  std::shared_ptr<TLatex> tInfo;
};

} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSDecodingErrorCheck_H
