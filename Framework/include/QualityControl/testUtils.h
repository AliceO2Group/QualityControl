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
/// \file   testUtils.h
/// \author Barthelemy von Haller
///

#ifndef QC_TEST_UTILS_H
#define QC_TEST_UTILS_H

#include <iostream>
#include <Common/Exceptions.h>

namespace o2::quality_control::test
{
bool do_nothing(AliceO2::Common::FatalException const& fe)
{
  std::cout << boost::diagnostic_information(fe) << std::endl;
  return true;
}
} // namespace o2::quality_control::test

#endif // QC_TEST_UTILS_H
