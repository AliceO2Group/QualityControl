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
/// \file   ITSTrackSimCheck.cxx
/// \author Artem Isakov
/// \author Jian Liu
///

#include "ITS/ITSThresholdCalibrationCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <TList.h>
#include <TH1.h>
#include <TText.h>

namespace o2::quality_control_modules::its
{

void ITSThresholdCalibrationCheck::configure() {}

Quality ITSThresholdCalibrationCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  return result;
}

std::string ITSThresholdCalibrationCheck::getAcceptedType() { return "TH1D"; }

void ITSThresholdCalibrationCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{

}
} // namespace o2::quality_control_modules::its
