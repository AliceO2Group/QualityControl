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
/// \file   TH2SBitmask.h
/// \author Dmitri Peresunko
///

#ifndef QC_MODULE_PHOS_TH2FBITMASK_H
#define QC_MODULE_PHOS_TH2FBITMASK_H

#include "QualityControl/TaskInterface.h"
#include <TH2S.h>
#include "Mergers/MergeInterface.h"

namespace o2::quality_control_modules::phos
{

/// \brief Custom TH2S class with special merger combining bit masks

class TH2SBitmask : public TH2S, public o2::mergers::MergeInterface
{
 public:
  /// \brief Constructor.
  TH2SBitmask() = default;
  TH2SBitmask(const char* name, const char* title, Int_t nbinsx, Double_t xlow, Double_t xup, Int_t nbinsy, Double_t ylow, Double_t yup) : TH2S(name, title, nbinsx, xlow, xup, nbinsy, ylow, yup) {}
  /// \brief Default destructor
  ~TH2SBitmask() = default;

  void merge(MergeInterface* const other) override;

 private:
  std::string mTreatMeAs = "TH2S"; // the name of the class this object should be considered as when drawing in QCG.

  ClassDefOverride(TH2SBitmask, 1);
};

} // namespace o2::quality_control_modules::phos

#endif // QC_MODULE_PHOS_TH2FBITMASK_H
