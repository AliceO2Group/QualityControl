// Copyright 2023 CERN and copyright holders of ALICE O2.
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
/// \file   BcCheck.cxx
/// \author Dawid Skora dawid.mateusz.skora@cern.ch
///

#include "FV0/BcCheck.h"

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::fv0
{

constexpr int kBinSwOnly = 1;
constexpr int kBinTcmOnly = 2;

void BcCheck::configure()
{

}

Quality BcCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

}

std::string BcCheck::getAcceptedType() { return "TH2"; }

void BcCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{

}

} // namespace o2::quality_control_modules::fv0
