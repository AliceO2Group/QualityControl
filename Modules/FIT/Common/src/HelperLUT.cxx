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
/// \file   HelperHist.cxx
/// \author Artur Furs afurs@cern.ch

#include "FITCommon/HelperLUT.h"

#include "DataFormatsFIT/LookUpTable.h"
#include "DataFormatsFDD/LookUpTable.h"
#include "DataFormatsFT0/LookUpTable.h"
#include "DataFormatsFV0/LookUpTable.h"
namespace o2::quality_control_modules::fit
{
namespace helperLUT
{
void obtainFromLUT(const o2::header::DataOrigin& det, Fee2ModuleName_t& mapFEE2ModuleName, long timestamp)
{
  if (det == o2::header::gDataOriginFDD) {
    obtainAllParamsLUT(o2::fdd::SingleLUT::Instance(nullptr, timestamp), mapFEE2ModuleName);
  } else if (det == o2::header::gDataOriginFT0) {
    obtainAllParamsLUT(o2::ft0::SingleLUT::Instance(nullptr, timestamp), mapFEE2ModuleName);
  } else if (det == o2::header::gDataOriginFV0) {
    obtainAllParamsLUT(o2::fv0::SingleLUT::Instance(nullptr, timestamp), mapFEE2ModuleName);
  }
}
} // namespace helperLUT

} // namespace o2::quality_control_modules::fit
