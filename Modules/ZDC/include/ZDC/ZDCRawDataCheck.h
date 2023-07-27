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
    std::string param;
  };

  struct sHistoCheck {
    std::string nameHisto;
    std::string typecheck;
    std::string typeHisto;
    std::string paramPosMsgX;
    std::string paramPosMsgY;
    float posMsgX;
    float posMsgY;
    int numW = 0;
    int numE = 0;
    int quality = 0; // 1 Good 2 warning 3 bad
    std::string stringW = "List channels Warning Quality: ";
    std::string stringE = "List channels Bad Quality: ";
    std::vector<sCheck> paramch;
  };
  void init();
  void setChName(std::string channel);
  void setChCheck(std::string histoName, std::string typeHisto, std::string typeCheck, std::string paramPosMsgX, std::string paramPosMsgY);
  std::vector<std::string> tokenLine(std::string Line, std::string Delimiter);
  void dumpVecParam(int id_histo, int numBinHisto, int num_ch);
  std::string getCurrentDataTime();
  void dumpStruct();

 private:
  std::vector<sHistoCheck> mVectHistoCheck;
  std::vector<std::string> mVectch;

  // std::string mStringW = "List channels Warning Quality: ";
  // std::string mStringE = "List channels Bad Quality: ";
};

} // namespace o2::quality_control_modules::zdc

#endif // QC_MODULE_ZDC_ZDCZDCRAWDATACHECK_H
