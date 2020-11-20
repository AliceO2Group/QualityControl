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
/// \since 24/03/2020
/// \brief Utilities to count events and fill histograms at the end of the main processing loops
///

#ifndef QC_MODULE_TOF_COUNTER_H
#define QC_MODULE_TOF_COUNTER_H

// ROOT includes
#include "TH1.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"

// Fairlogger includes
#include <fairlogger/Logger.h>

namespace o2::quality_control_modules::tof
{

/// \brief Class to count events
/// \author Nicolo' Jacazio
template <const unsigned int size, const char* labels[size]>
class Counter
{
 public:
  /// \brief Constructor
  Counter() = default;

  /// Destructor
  ~Counter() = default;

  /// Function to increment a counter
  /// @param v Index in the counter array to increment
  void Count(const unsigned int& v);

  /// Function to reset counters to zero
  void Reset();

  /// Function to print the counter content
  void Print();

  /// Function to get the total number of counts
  uint32_t Total();

  /// Function to get the total number of counts added since last call
  uint32_t TotalNew();

  /// Function to get the total number of counts and reset the counts
  uint32_t TotalAndReset();

  /// Function to get how many counts where observed
  /// @return Returns the number of counts observed for a particular index
  uint32_t HowMany(const unsigned int& pos) const { return counter[pos]; }

  /// Function to make a histogram out of the counters
  /// @param h histogram to shape in order to have room for the counter size
  void MakeHistogram(TH1* h) const;

  /// Function to fill a histogram with the counters
  /// @param h The histogram to fill
  /// @param biny Y offset to fill to histogram, useful for TH2 and TH3
  /// @param binz Z offset to fill to histogram, useful for TH3
  void FillHistogram(TH1* h, const unsigned int& biny = 0, const unsigned int& binz = 0) const;

  /// Getter for the size
  /// @return Returns the size of the counter
  unsigned int Size() const { return size; }

 private:
  // static_assert((sizeof(labels) / sizeof(const char*)) == size, "size of the counter and the one of the labels must coincide");
  /// Containers to fill
  std::array<uint32_t, size> counter = { 0 };
  uint32_t mTotal = 0;
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_COUNTER_H
