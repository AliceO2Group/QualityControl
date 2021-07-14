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
/// \file   CustomTH2F.cxx
/// \author Barthelemy von Haller
///

#include "Example/CustomTH2F.h"
#include "QualityControl/QcInfoLogger.h"

namespace o2::quality_control_modules::example
{

CustomTH2F::CustomTH2F(std::string name) : TH2F(name.c_str(), "Custom object inheriting from TH2F", 100, 0, 99, 100, 0, 99)
{
}

} // namespace o2::quality_control_modules::example
