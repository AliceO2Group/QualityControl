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
/// \file   AlwaysGoodCheck.cxx
/// \author Piotr Konopka
///

#include "Benchmark/AlwaysGoodCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

// ROOT
#include <TH1.h>

using namespace std;

namespace o2::quality_control_modules::benchmark
{

void AlwaysGoodCheck::configure(std::string) {}

Quality AlwaysGoodCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>*)
{
  return Quality::Good;
}

std::string AlwaysGoodCheck::getAcceptedType() { return ""; }

void AlwaysGoodCheck::beautify(std::shared_ptr<MonitorObject>, Quality)
{
}

} // namespace o2::quality_control_modules::benchmark
