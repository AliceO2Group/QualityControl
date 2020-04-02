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

// #define ENABLE_COUNTER_DEBUG_MODE // Flag used to enable more printing and more debug

// ROOT includes
#include "TObject.h"
#include "TH1.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"

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
  void Count(UInt_t v)
  {
    if (v > Tc::size) {
      ILOG(Error) << "Incrementing counter too far! " << v << "/" << Size() << ENDM;
    }
#ifdef ENABLE_COUNTER_DEBUG_MODE
    ILOG(Info) << "Incrementing " << v << "/" << Size() << " to " << counter[v] << ENDM;
#endif
    counter[v]++;
  }
  /// Function to reset counters
  void Reset()
  {
    ILOG(Info) << "Resetting Counter" << ENDM;
    for (UInt_t i = 0; i < Tc::size; i++) {
      counter[i] = 0;
    }
  }
  /// Function to get how many counts where observed
  uint32_t HowMany(UInt_t pos) const { return counter[pos]; }
  /// Function to make a histogram out of the counters
  void MakeHistogram(TH1* h) const
  {
    ILOG(Info) << "Making Histogram " << h->GetName() << " out of counter" << ENDM;
    h->Reset();
    h->GetXaxis()->Set(Tc::size, 0, Tc::size);
    UInt_t binx = 1;
    for (UInt_t i = 0; i < Tc::size; i++) {
      if (Tc::names[i].IsNull()) {
        continue;
      }
      h->GetXaxis()->SetBinLabel(binx++, Tc::names[i]);
    }
    h->Reset();
#ifdef ENABLE_COUNTER_DEBUG_MODE
    h->Print("All");
#endif
  }
  /// Function to fill a histogram with the counters
  void FillHistogram(TH1* h, UInt_t biny = 0, UInt_t binz = 0) const
  {
    ILOG(Info) << "Filling Histogram " << h->GetName() << " out of counter" << ENDM;
    UInt_t binx = 1;
    for (UInt_t i = 0; i < Tc::size; i++) {
      if (Tc::names[i].IsNull()) {
        continue;
      }
#ifdef ENABLE_COUNTER_DEBUG_MODE
      ILOG(Info) << "Filling bin " << binx << " of position " << i << " of label " << Tc::names[i] << " with " << counter[i] << ENDM;
#endif
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
#ifdef ENABLE_COUNTER_DEBUG_MODE
    h->Print("All");
#endif
  }
  /// Getter for the Tc::size
  UInt_t Size() const { return Tc::size; }

 private:
  static_assert(std::is_same<decltype(Tc::size), const UInt_t>::value, "size must be const UInt_t");
  static_assert(std::is_same<decltype(Tc::names), const TString[Tc::size]>::value, "names must be const TString arrays");
  /// Containers to fill
  uint32_t counter[Tc::size] = { 0 };
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_COUNTER_H
