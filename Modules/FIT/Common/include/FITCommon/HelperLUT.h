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
/// \file   HelperHist.h
/// \author Artur Furs afurs@cern.ch
/// \brief Temporary helper class for LookupTable of FIT detectors, shoud be moved into O2
#ifndef QC_MODULE_FIT_HELPERLUT_H
#define QC_MODULE_FIT_HELPERLUT_H

#include <map>
#include <string>

#include "Headers/DataHeader.h"

namespace o2::quality_control_modules::fit
{
namespace helperLUT
{
//(epID,linkID)->moduleName mapping
using FEEID_t = std::pair<int, int>;
using Fee2ModuleName_t = std::map<FEEID_t, std::string>;
template <typename EPID_t, typename LinkID_t>
inline FEEID_t getFEEID(EPID_t&& epID, LinkID_t&& linkID)
{
  return FEEID_t{ static_cast<typename FEEID_t::first_type>(std::forward<EPID_t>(epID)), static_cast<typename FEEID_t::second_type>(std::forward<LinkID_t>(linkID)) };
}

template <typename LUT>
inline Fee2ModuleName_t getMapFEE2ModuleName(const LUT& lut)
{
  Fee2ModuleName_t mapFEE2ModuleName{};
  const auto& vecFEE = lut.getVecMetadataFEE();
  for (const auto& entryFEE : vecFEE) {
    const auto& entryCRU = entryFEE.mEntryCRU;
    mapFEE2ModuleName.insert({ getFEEID(entryCRU.mEndPointID, entryCRU.mLinkID), entryFEE.mModuleName });
  }
  return mapFEE2ModuleName;
}

template <typename LUT>
inline void obtainAllParamsLUT(const LUT& lut, Fee2ModuleName_t& mapFEE2ModuleName)
{
  // Put here all LUT params you would like to have
  mapFEE2ModuleName = getMapFEE2ModuleName(lut);
}

void obtainFromLUT(const o2::header::DataOrigin& det, Fee2ModuleName_t& mapFEE2ModuleName, long timestamp = -1);
} // namespace helperLUT

} // namespace o2::quality_control_modules::fit
#endif
