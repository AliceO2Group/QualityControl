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
/// \file   BenchmarkCheck.cxx
/// \author Piotr Konopka
///

#include "Benchmark/BenchmarkCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

// ROOT
#include <TH1.h>

using namespace std;

namespace o2::quality_control_modules::benchmark
{

void BenchmarkCheck::configure(std::string) {}

Quality BenchmarkCheck::check(const MonitorObject* mo)
{
  return Quality::Good;
}

std::string BenchmarkCheck::getAcceptedType() { return "TH1"; }

void BenchmarkCheck::beautify(MonitorObject* mo, Quality checkResult)
{

}

} // namespace o2::quality_control_modules::benchmark
