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
/// \file   ZDCRawDataCheck.h
/// \author Carlo Puggioni
///

#ifndef QC_MODULE_ZDC_ZDCZDCRAWDATACHECK_H
#define QC_MODULE_ZDC_ZDCZDCRAWDATACHECK_H

#include "QualityControl/CheckInterface.h"
#include <string>
#include <vector>

namespace o2::quality_control_modules::zdc
{

/// \brief  QC Check Data Raw. Check baseline mean values of each ZDC channel.
/// \author Carlo Puggioni
class ZDCRawDataCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  ZDCRawDataCheck() = default;
  /// Destructor
  ~ZDCRawDataCheck() override = default;

  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  ClassDefOverride(ZDCRawDataCheck, 2);
  struct sCheck {
    std::string ch;
    float minW;
    float maxW;
    float minE;
    float maxE;
    std::string typech;
  };

  void init();
  void setChName(std::string channel);
  void setChCheck(int i);
  std::vector<std::string> tokenLine(std::string Line, std::string Delimiter);
  void dumpVecParam(int numBinHisto, int num_ch);

 private:
  std::vector<sCheck> mVectParam;
  int mNumW = 0;
  int mNumE = 0;
  float mPosMsgX;
  float mPosMsgY;
  std::string mStringW = " ";
  std::string mStringE = "List channel Bad Quality: ";
};

} // namespace o2::quality_control_modules::zdc

#endif // QC_MODULE_ZDC_ZDCZDCRAWDATACHECK_H
