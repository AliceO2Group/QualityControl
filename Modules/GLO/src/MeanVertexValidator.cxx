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
/// \file   MeanVertexValidator.cxx
/// \author Andrea Ferrero
///

#include "GLO/MeanVertexValidator.h"
#include "DataFormatsCalibration/MeanVertexObject.h"

#include <TClass.h>

#include <iostream>

using namespace o2::quality_control_modules::common;

namespace o2::quality_control_modules::glo
{

const std::type_info& MeanVertexValidator::getTinfo() const
{
  return typeid(o2::dataformats::MeanVertexObject);
}

bool MeanVertexValidator::validate(void* obj)
{
  if (!obj) {
    return false;
  }
  auto meanVtx = static_cast<o2::dataformats::MeanVertexObject*>(obj);
  auto x = meanVtx->getX();
  auto y = meanVtx->getY();
  auto z = meanVtx->getZ();
  auto sx = meanVtx->getSigmaX();
  auto sy = meanVtx->getSigmaY();
  auto sz = meanVtx->getSigmaZ();

  // do some sanity check using conservative limits for the vertex parameters
  // X-Y: 1 cm
  // Z: 10 cm
  // sX-sY: 1 cm
  // sZ: 100 cm
  // aigmas not negative
  if (std::fabs(x) > 1 || std::fabs(y) > 1 || std::fabs(z) > 10) {
    return false;
  }
  if (sx < 0 || sy < 0 || sz < 0) {
    return false;
  }
  if (sx > 1 || sy > 1 || sz > 100) {
    return false;
  }
  return true;
}

} // namespace o2::quality_control_modules::glo
