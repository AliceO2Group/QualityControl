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
/// \file   FakeCheck.cxx
/// \author Barthelemy von Haller
///

#include "Example/FakeCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

using namespace std;

ClassImp(o2::quality_control_modules::example::FakeCheck)

  namespace o2::quality_control_modules::example
{

  void FakeCheck::configure() {}

  Quality FakeCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>> * /*moMap*/)
  {
    Quality result = Quality::Null;

    return result;
  }

  std::string FakeCheck::getAcceptedType() { return "TH1"; }

  void FakeCheck::beautify(std::shared_ptr<MonitorObject> /*mo*/, Quality /*checkResult*/)
  {
    // NOOP
  }

} // namespace daq::quality_control_modules::example
