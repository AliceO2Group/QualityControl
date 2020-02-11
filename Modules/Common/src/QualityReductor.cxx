// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file  QualityReductor.cxx
/// \author Piotr Konopka
///

#include "Common/QualityReductor.h"

#include "QualityControl/QualityObject.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::common
{

void* QualityReductor::getBranchAddress()
{
  return &mQuality;
}

const char* QualityReductor::getBranchLeafList()
{
  return "level/i:name/C";
}

void QualityReductor::update(TObject* obj)
{
  auto qo = dynamic_cast<QualityObject*>(obj);
  if (qo) {
    auto quality = qo->getQuality();

    size_t last = quality.getName().length() < NAME_SIZE ? quality.getName().length() : NAME_SIZE - 1;
    strncpy(mQuality.name, quality.getName().c_str(), last + 1);
    mQuality.name[last] = '\0';

    mQuality.level = quality.getLevel();
  }
}

} // namespace o2::quality_control_modules::common
