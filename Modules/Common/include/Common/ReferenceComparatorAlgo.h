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
/// \file   ReferenceComparatorAlgo.h
/// \author Andrea Ferrero
///

#ifndef QUALITYCONTROL_ReferenceComparatorAlgo_H
#define QUALITYCONTROL_ReferenceComparatorAlgo_H

#include "QualityControl/Quality.h"
class TObject;

namespace o2::quality_control_modules::common
{

enum MOCompMethod {
  MOCompChi2,
  MOCompDeviation
};

o2::quality_control::core::Quality compareTObjects(TObject* obj, TObject* objRef, MOCompMethod method, double threshold);

}  // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_ReferenceComparatorAlgo_H
