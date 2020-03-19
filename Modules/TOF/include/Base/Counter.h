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
#include "TH1.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"

#define ENABLE_COUNTER_DEBUG_MODE

namespace o2::quality_control_modules::tof
{

/// \brief Class to count events
/// \author Nicolo' Jacazio
template <typename Tc, Tc size, const TString* names>
class Counter
// : TObject /*final*/
// todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  Counter() = default;
  /// Destructor
  ~Counter() = default;
  /// Function to increment a counter
  void Count(UInt_t v)
  {
#ifdef ENABLE_COUNTER_DEBUG_MODE
    if (v > size) {
      ILOG(Error) << "Incrementing counter too far! " << v << "/" << size << ENDM;
    }
#endif
    ILOG(Info) << "Incrementing " << v << "/" << size << " to " << counter[v] << ENDM;
    counter[v]++;
  }
  /// Function to reset counters
  void Reset()
  {
    ILOG(Info) << "Resetting Counter" << ENDM;
    for (UInt_t i = 0; i < size; i++) {
      counter[i] = 0;
    }
  }
  /// Function to get how many counts where observed
  uint32_t HowMany(UInt_t pos) const { return counter[pos]; };
  /// Function to make a histogram out of the counters
  void MakeHistogram(TH1* h) const
  {
    ILOG(Info) << "Making Histogram out of counter" << ENDM;
    h->Reset();
    h->GetXaxis()->Set(size, 0, size);
    Int_t binx = 1;
    for (Int_t i = 0; i < size; i++) {
      if (names[i].IsNull()) {
        continue;
      }
      h->GetXaxis()->SetBinLabel(binx++, names[i]);
    }
    h->Reset();
    h->GetXaxis()->Print("All");
    h->Print("All");
  }
  /// Function to fill a histogram with the counters
  void FillHistogram(TH1* h, Int_t biny = 0, Int_t binz = 0) const
  {
    ILOG(Info) << "Filling Histogram out of counter" << ENDM;
    Int_t binx = 1;
    for (UInt_t i = 0; i < size; i++) {
      if (names[i].IsNull()) {
        continue;
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
    h->Print("All");
  }
  /// Getter for the size
  Tc Size() const { return size; };

 private:
  /// Containers to fill
  uint32_t counter[size] = { 0 };
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_COUNTER_H
