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
/// \file   CalibReductorTRD.h
/// \author Salman Malik
///

#ifndef QUALITYCONTROL_CALIBREDUCTORTRD_H
#define QUALITYCONTROL_CALIBREDUCTORTRD_H

#include "QualityControl/ReductorConditionAny.h"

#include <DataFormatsTRD/Constants.h>

namespace o2::quality_control_modules::trd
{

/// \brief A Reductor which obtains the calibration parameters of TRD from ccdb
///
class CalibReductorTRD : public quality_control::postprocessing::ReductorConditionAny
{
 public:
  CalibReductorTRD() = default;
  ~CalibReductorTRD() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  bool update(ConditionRetriever& retriever) override;

 private:
  struct {
    Float_t vdrift[o2::trd::constants::MAXCHAMBER];
    Float_t vdriftmean;
    Float_t vdrifterr;
    Float_t exbmean;
    Float_t exberr;
  } mStats;
};

} // namespace o2::quality_control_modules::trd

#endif // QUALITYCONTROL_CALIBREDUCTORTRD_H
