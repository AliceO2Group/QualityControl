// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.



// Fair
#include <fairlogger/Logger.h>
// ROOT
#include "TH1.h"
#include "TTree.h"
// Quality Control
#include "FT0/BasicDigitQcCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"

using namespace std;

namespace o2::quality_control_modules::ft0
{

void BasicDigitQcCheck::configure(std::string) {}

Quality BasicDigitQcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  
  return Quality::Good;

}

std::string BasicDigitQcCheck::getAcceptedType() { return "TH1"; }

void BasicDigitQcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{ 
  
}

} // namespace o2::quality_control_modules::mft
