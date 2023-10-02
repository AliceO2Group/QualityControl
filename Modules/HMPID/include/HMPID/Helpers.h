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
/// \file   Helpers.h
/// \author Nicola Nicassio
///

#ifndef QC_MODULE_HMPID_HELPERS_H
#define QC_MODULE_HMPID_HELPERS_H

#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/Quality.h"
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TLegend.h>
#include <TText.h>
#include <gsl/span>
#include <utility>
#include <optional>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::hmpid
{

constexpr int getNumDDL() { return 14; }

constexpr int getNumHV() { return 42; }

bool matchHistName(std::string hist, std::string name);

//_________________________________________________________________________________________

// check plots vs DDL
struct QualityCheckerDDL {
  QualityCheckerDDL();

  void resetDDL();
  void addCheckResultDDL(gsl::span<o2::quality_control::core::Quality> result);
  o2::quality_control::core::Quality getQualityDDL();
  std::array<o2::quality_control::core::Quality, getNumDDL()> mQualityDDL;
  int mMaxBadDDLForMedium;
  int mMaxBadDDLForBad;
};

//_________________________________________________________________________________________

// check plots vs HV
struct QualityCheckerHV {
  QualityCheckerHV();

  void resetHV();
  void addCheckResultHV(gsl::span<o2::quality_control::core::Quality> result);
  o2::quality_control::core::Quality getQualityHV();
  std::array<o2::quality_control::core::Quality, getNumHV()> mQualityHV;
  int mMaxBadHVForMedium;
  int mMaxBadHVForBad;
};

//_________________________________________________________________________________________

} // namespace o2::quality_control_modules::hmpid

#endif // QC_MODULE_HMPID_HELPERS_H