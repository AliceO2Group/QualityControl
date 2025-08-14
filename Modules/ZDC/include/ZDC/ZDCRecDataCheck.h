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
/// \file   ZDCRecDataCheck.h
/// \author Carlo Puggioni
///

#ifndef QC_MODULE_ZDC_ZDCZDCRECDATACHECK_H
#define QC_MODULE_ZDC_ZDCZDCRECDATACHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::zdc
{

/// \brief  QC Check Data Rec. Check ADC and TDC mean values of each ZDC channel.
/// \author alienv enter Readout/latest
class ZDCRecDataCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  ZDCRecDataCheck() = default;
  /// Destructor
  ~ZDCRecDataCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  ClassDefOverride(ZDCRecDataCheck, 2);

  struct sCheck {
    std::string ch;
    float minW;
    float maxW;
    float minE;
    float maxE;
    std::string typech;
  };
  void startOfActivity(const Activity& activity) override;
  void init(const Activity& activity);
  void setChName(std::string channel, std::string type);
  void setChCheck(int i, std::string type, const Activity& activity);
  std::vector<std::string> tokenLine(std::string Line, std::string Delimiter);
  void dumpVecParam(int numBinHisto, int num_ch);
  void setQualityInfo(std::shared_ptr<MonitorObject> mo, int color, std::string text);
  std::string getCurrentDataTime();

 private:
  std::vector<sCheck> mVectParamADC;
  std::vector<sCheck> mVectParamTDC;
  std::vector<sCheck> mVectParamTDCA;
  std::vector<sCheck> mVectParamPeak1n;
  std::vector<sCheck> mVectParamPeak1p;

  int mNumWADC = 0;
  int mNumEADC = 0;
  int mNumWTDC = 0;
  int mNumETDC = 0;
  int mNumWTDCA = 0;
  int mNumETDCA = 0;
  int mNumWPeak1n = 0;
  int mNumEPeak1n = 0;
  int mNumWPeak1p = 0;
  int mNumEPeak1p = 0;

  float mPosMsgADCX;
  float mPosMsgADCY;
  float mPosMsgTDCX;
  float mPosMsgTDCY;
  float mPosMsgTDCAX;
  float mPosMsgTDCAY;
  float mPosMsgPeak1nX;
  float mPosMsgPeak1nY;
  float mPosMsgPeak1pX;
  float mPosMsgPeak1pY;

  int mQADC = 0;
  int mQTDC = 0;
  int mQTDCA = 0;
  int mQPeak1n = 0;
  int mQPeak1p = 0;

  std::string mStringWADC = "";
  std::string mStringEADC = "";
  std::string mStringWTDC = "";
  std::string mStringETDC = "";
  std::string mStringWTDCA = "";
  std::string mStringETDCA = "";
  std::string mStringWPeak1n = "";
  std::string mStringEPeak1n = "";
  std::string mStringWPeak1p = "";
  std::string mStringEPeak1p = "";
};

} // namespace o2::quality_control_modules::zdc

#endif // QC_MODULE_ZDC_ZDCZDCRECDATACHECK_H
