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
/// \file   QcMFTTrackCheck.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Diana Maria Krupova
/// \author Katarina Krizkova Gajdosova

// Fair
#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
// Quality Control
#include "MFT/QcMFTTrackCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

using namespace std;

namespace o2::quality_control_modules::mft
{

Quality QcMFTTrackCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  return result;
}

std::string QcMFTTrackCheck::getAcceptedType() { return "TH1"; }

void QcMFTTrackCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
}

} // namespace o2::quality_control_modules::mft
