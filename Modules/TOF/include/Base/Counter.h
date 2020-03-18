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

namespace o2::quality_control_modules::tof
{

/// \brief Class to count events
/// \author Nicolo' Jacazio
template <typename Tc, Tc cdim>
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
  void Count(Tc v)
  {
    ILOG(Info) << "Incrementing " << v << " to " << counter[v] << ENDM;
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
  void MakeHistogram(TH1* h) const
  {
    ILOG(Info) << "Making Histogram out of counter" << ENDM;
    h->Reset();
    h->GetXaxis()->Set(size, 0, size);
    h->GetXaxis()->Print("All");
    h->Print("All");
  }
  void FillHistogram(TH1* h) const
  {
    ILOG(Info) << "Filling Histogram out of counter" << ENDM;
    for (UInt_t i = 0; i < size; i++) {
      h->SetBinContent(i + 1, counter[i]);
    }
    h->Print("All");
  }

  /// Size of the counter
  static const Tc size = cdim;

 private:
  /// Containers to fill
  uint32_t counter[size] = { 0 };
};


} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_COUNTER_H
