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

//
// \file   LtrCalibReductor.h
// \author Cindy Mordasini
// \author Marcel Lesch
//
#ifndef QC_MODULE_TPC_LTRCALIBREDUCTOR_H
#define QC_MODULE_TPC_LTRCALIBREDUCTOR_H

#include "QualityControl/Reductor.h"
//#include "DataFormatsTPC/LtrCalibData.h"
#include <TText.h>

namespace o2::quality_control_modules::tpc
{

/// \brief <insert some text here>
///
/// <insert more text here>
class LtrCalibReductor : public quality_control::postprocessing::Reductor
{
 public:
  LtrCalibReductor() = default;
  ~LtrCalibReductor() = default;

  void* getBranchAddress() final;
  const char* getBranchLeafList() final;
  void update(TObject* obj) final;

 private:
  struct {
    double processedTFs;
    double dvCorrectionA;
    double dvCorrectionC;
    double dvCorrection;
    double dvOffsetA;
    double dvOffsetC;
    double nTracksA;
    double nTracksC;
  } mLtrCalib;

  double getValue(TText* line);
};

}   // namespace o2::quality_control_modules::tpc
#endif //QC_MODULE_TPC_LTRCALIBREDUCTOR_H