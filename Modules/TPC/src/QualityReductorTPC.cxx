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
/// \file     QualityReductorTPC.cxx
/// \author   Marcel Lesch
/// \author   Based on the work from Piotr Konopka
///

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/QualityObject.h"
#include "TPC/QualityReductorTPC.h"

namespace o2::quality_control_modules::tpc
{
void QualityReductorTPC::updateQuality(TObject* obj, SliceInfoQuality& reducedQualitySource, std::vector<std::string>& ranges)
{
  auto qo = dynamic_cast<quality_control::core::QualityObject*>(obj); // Get Quality Object

  if (qo) {

    auto quality = qo->getQuality();
    std::string qualityName = qo->getPath();
    ranges.push_back(qualityName);

    reducedQualitySource.qualitylevel = quality.getLevel();
  } else {
    ILOG(Error, Support) << "Error: 'Qualityobject' not found." << ENDM;
  }
}

} // namespace o2::quality_control_modules::tpc
