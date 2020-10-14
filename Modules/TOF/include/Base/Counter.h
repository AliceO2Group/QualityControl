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

// #define ENABLE_COUNTER_DEBUG_MODE // Flag used to enable more printing and more debug
// #define ENABLE_PRINT_HISTOGRAMS_MODE // Flag used to enable more printing and more debug

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
template <typename Tc>
class Counter
{
 public:
  /// \brief Constructor
  Counter() = default;

  /// Destructor
  ~Counter() = default;

  /// Function to increment a counter
  /// @param v Index in the counter array to increment
  void Count(UInt_t v)
  {
    if (v > Tc::size) {
      LOG(FATAL) << "Incrementing counter too far! " << v << "/" << Size();
    }
#ifdef ENABLE_COUNTER_DEBUG_MODE
    LOG(INFO) << "Incrementing " << v << "/" << Size() << " to " << counter[v];
#endif
    counter[v]++;
  }

  /// Function to reset counters to zero
  void Reset()
  {
    LOG(INFO) << "Resetting Counter";
    for (UInt_t i = 0; i < Tc::size; i++) {
      counter[i] = 0;
    }
  }

  /// Function to get how many counts where observed
  /// @return Returns the number of counts observed for a particular index
  uint32_t HowMany(UInt_t pos) const { return counter[pos]; }

  /// Function to make a histogram out of the counters
  /// @param h histogram to shape in order to have room for the counter size
  void MakeHistogram(TH1* h) const
  {
    LOG(INFO) << "Making Histogram " << h->GetName() << " to accomodate counter of size " << Tc::size;
    TAxis* axis = h->GetXaxis();
    if (((UInt_t)axis->GetNbins()) < Tc::size) {
      LOG(FATAL) << "The histogram size (" << axis->GetNbins() << ") is not large enough to accomodate the counter size (" << Tc::size << ")";
    }
    h->Reset();
    UInt_t histo_size = Tc::size;
    for (UInt_t i = 0; i < Tc::size; i++) {
      if (Tc::names[i].IsNull()) {
        histo_size--;
      }
    }
    if (histo_size == 0) {
      LOG(FATAL) << "Asked to produce a histogram with size " << histo_size << ", check counter bin labels";
    }
    axis->Set(histo_size, 0, histo_size);
    UInt_t binx = 1;
    for (UInt_t i = 0; i < Tc::size; i++) {
      if (Tc::names[i].IsNull()) {
        continue;
      }
      LOG(INFO) << "Setting bin " << binx << "/" << histo_size << " to contain counter for " << Tc::names[i] << " (index " << i << "/" << Tc::size - 1 << ")";
      if (histo_size < binx) {
        LOG(FATAL) << "Making bin outside of histogram limits!";
      }
      axis->SetBinLabel(binx++, Tc::names[i]);
    }
    h->Reset();
#ifdef ENABLE_PRINT_HISTOGRAMS_MODE
    h->Print("All");
#endif
  }

  /// Function to fill a histogram with the counters
  /// @param h The histogram to fill
  /// @param biny Y offset to fill to histogram, useful for TH2 and TH3
  /// @param binz Z offset to fill to histogram, useful for TH3
  void FillHistogram(TH1* h, UInt_t biny = 0, UInt_t binz = 0) const
  {
    LOG(INFO) << "Filling Histogram " << h->GetName() << " with counter contents";
    UInt_t binx = 1;
    const UInt_t nbinsx = h->GetNbinsX();
    for (UInt_t i = 0; i < Tc::size; i++) {
      if (Tc::names[i].IsNull()) {
        if (counter[i] > 0) {
          LOG(WARNING) << "Counter at position " << i << " was non empty (" << counter[i] << ") but was discarded because of empty lables";
        }
        continue;
      }
#ifdef ENABLE_COUNTER_DEBUG_MODE
      LOG(INFO) << "Filling bin " << binx << " of position " << i << " of label " << Tc::names[i] << " with " << counter[i];
#endif
      if (binx > nbinsx) {
        LOG(FATAL) << "Filling histogram " << h->GetName() << " at position " << binx << " i.e. past its size (" << nbinsx << ")!";
      }
      if (!Tc::names[i].EqualTo(h->GetXaxis()->GetBinLabel(binx))) {
        LOG(FATAL) << "Bin " << binx << " does not have the expected label '" << h->GetXaxis()->GetBinLabel(binx) << "' vs '" << Tc::names[i] << "'";
      }
      if (biny > 0) {
        if (binz > 0) {
          h->SetBinContent(binx, biny, binz, counter[i]);
        } else {
          h->SetBinContent(binx, biny, counter[i]);
        }
      } else {
        h->SetBinContent(binx, counter[i]);
      }
      binx++;
    }
#ifdef ENABLE_PRINT_HISTOGRAMS_MODE
    h->Print("All");
#endif
  }
  /// Getter for the Tc::size
  /// @return Returns the size of the counter
  UInt_t Size() const { return Tc::size; }

 private:
  static_assert(std::is_same<decltype(Tc::size), const UInt_t>::value, "size must be const UInt_t");
  static_assert(std::is_same<decltype(Tc::names), const TString[Tc::size]>::value, "names must be const TString arrays");
  /// Containers to fill
  std::array<uint32_t, Tc::size> counter = { 0 };
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_COUNTER_H
