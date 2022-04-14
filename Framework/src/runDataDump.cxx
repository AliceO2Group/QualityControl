// Copyright 2019-2022 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
//

///
/// \author Barthelemy von Haller
/// \file runDataDump.cxx
///

#include "QualityControl/DataDumpGui.h"
#include <fairmq/runDevice.h>

namespace bpo = boost::program_options;

void addCustomOptions(bpo::options_description& /*options*/) {}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions&) { return std::make_unqiue<o2::quality_control::core::DataDumpGui>(); }
