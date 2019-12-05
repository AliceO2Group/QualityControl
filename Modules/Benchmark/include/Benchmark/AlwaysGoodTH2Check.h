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
/// \file   BenchmarkCheck.h
/// \author Piotr Konopka
///

#ifndef QC_MODULE_BENCHMARK_BENCHMARKCHECK_H
#define QC_MODULE_BENCHMARK_BENCHMARKCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::benchmark
{

/// \brief  Give always a good quality
///
/// \author Piotr Konopka
class AlwaysGoodTH2Check : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  AlwaysGoodTH2Check() = default;
  /// Destructor
  ~AlwaysGoodTH2Check() override = default;

  // Override interface
  void configure(std::string name) override;
  Quality check(const MonitorObject* mo) override;
  void beautify(MonitorObject* mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  ClassDefOverride(AlwaysGoodTH2Check, 1);
};

} // namespace o2::quality_control_modules::benchmark

#endif // QC_MODULE_BENCHMARK_BENCHMARKCHECK_H
