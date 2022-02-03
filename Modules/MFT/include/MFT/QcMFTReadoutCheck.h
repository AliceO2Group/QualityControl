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
/// \file   QcMFTReadoutCheck.h
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova
///

#ifndef QC_MFT_READOUT_CHECK_H
#define QC_MFT_READOUT_CHECK_H

// ROOT
#include <TLatex.h>
// Quality Control
#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::mft
{

/// \brief  MFT Readout Header Check
///
class QcMFTReadoutCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  QcMFTReadoutCheck() = default;
  /// Destructor
  ~QcMFTReadoutCheck() override = default;

  // Override interface
  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  std::vector<int> mVectorOfFaultBins;
  std::vector<int> mVectorOfErrorBins;
  std::vector<int> mVectorOfWarningBins;

  TLatex* drawLatex(double xmin, double ymin, Color_t color, TString text);
  void resetVector(std::vector<int>& vector);
  Quality checkQualityStatus(TH1F* histo, std::vector<int>& vector);
  void writeMessages(TH1F* histo, std::vector<int>& vector, Quality checkResult);

  ClassDefOverride(QcMFTReadoutCheck, 1);
};

} // namespace o2::quality_control_modules::mft

#endif // QC_MFT_READOUT_CHECK_H
