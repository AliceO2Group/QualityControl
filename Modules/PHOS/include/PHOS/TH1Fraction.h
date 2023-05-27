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
/// \file   TH1Fraction.h
/// \author Sergey Evdokimov
///

#ifndef QC_MODULE_PHOS_TH1FRACTION_H
#define QC_MODULE_PHOS_TH1FRACTION_H

#include "QualityControl/TaskInterface.h"
#include <TH1.h>
#include "Mergers/MergeInterface.h"

namespace o2::quality_control_modules::phos
{

/// \brief Custom TH2F class with special merger

class TH1Fraction : public TH1D, public o2::mergers::MergeInterface
{
 public:
  /// \brief Constructor.
  TH1Fraction() = default;
  TH1Fraction(const char* name, const char* title, Int_t nbinsx, Double_t xlow, Double_t xup);
  TH1Fraction(TH1Fraction const& copymerge);
  /// \brief Destructor
  virtual ~TH1Fraction()
  {
    delete mUnderlyingCounts;
  }

  void increaseEventCounter(int increment) { mEventCounter += increment; }
  void fillUnderlying(double x) { mUnderlyingCounts->Fill(x); }
  void update();
  TH1D* getUnderlyingCounts() const { return mUnderlyingCounts; }
  unsigned long long getEventCounter() const { return mEventCounter; }
  void Reset(Option_t* option = "") override;

  void merge(MergeInterface* const other) override;

 private:
  std::string mTreatMeAs = "TH1F";      // the name of the class this object should be considered as when drawing in QCG.
  unsigned long long mEventCounter = 0; // event counter
  TH1D* mUnderlyingCounts{ nullptr };   // underlying histogram with counts

  ClassDefOverride(TH1Fraction, 1);
};

} // namespace o2::quality_control_modules::phos

#endif // QC_MODULE_PHOS_TH1FRACTION_H
