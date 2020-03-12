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
/// \file   Counter.h
/// \author Nicolo' Jacazio
///

#ifndef QC_MODULE_TOF_COUNTER_H
#define QC_MODULE_TOF_COUNTER_H

// ROOT includes
#include "TObject.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"

namespace o2::quality_control_modules::tof
{

// Enums
enum ECrateCounter_t {
  kCrateCounter_Data,
  kCrateCounter_Error,
  kNCrateCounters
};

enum ETRMCounter_t {
  kTRMCounter_Data,
  kTRMCounter_Error,
  kNTRMCounters
};

enum ETRMChainCounter_t {
  kTRMChainCounter_Data,
  kTRMChainCounter_Error,
  kNTRMChainCounters
};

/// \brief Class to count events
/// \author Nicolo' Jacazio
template <typename Tc, Tc cdim>
class Counter
//  : TObject/*final*/
// todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  Counter() = default;
  /// Destructor
  ~Counter() = default;
  /// Function to increment a counter
  void Count(Tc v);
  /// Function to reset counters
  void Reset();

 private:
  /// Containers to fill
  static const Tc size = cdim;
  uint32_t counter[size] = { 0 };
};

/// \brief Class for linear container for counters
/// \author Nicolo' Jacazio
template <UInt_t dim, typename Tc, Tc cdim>
class CounterList
//  : TObject/*final*/
// todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  CounterList() = default;
  /// Destructor
  ~CounterList() = default;

  /// Function to increment a counter
  void Count(UInt_t c, Tc v);

  /// Function to reset counters
  void Reset();

 private:
  /// Containers to fill
  static const UInt_t size = dim;
  Counter<Tc, cdim> counter[size];
};

/// \brief Class for matrix container for counters
/// \author Nicolo' Jacazio
template <UInt_t dimX, UInt_t dimY, typename Tc, Tc cdim>
class CounterMatrix
//  : TObject/*final*/
// todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  CounterMatrix() = default;
  /// Destructor
  ~CounterMatrix() = default;

  /// Function to increment a counter
  void Count(UInt_t c, UInt_t cc, Tc v);

  /// Function to reset counters
  void Reset();

 private:
  /// Containers to fill
  static const UInt_t size = dimX;
  CounterList<dimY, Tc, cdim> counter[size];
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_COUNTER_H
