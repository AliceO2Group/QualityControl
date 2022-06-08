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
/// \file   TH2FMean.h
/// \author Dmitri Peresunko
///

#ifndef QC_MODULE_PHOS_TH2FMEAN_H
#define QC_MODULE_PHOS_TH2FMEAN_H

#include "QualityControl/TaskInterface.h"
#include <TH2F.h>
#include "Mergers/MergeInterface.h"

namespace o2::quality_control_modules::phos
{

/// \brief Custom TH2F class with special merger

class TH2FMean : public TH2F, public o2::mergers::MergeInterface
{
 public:
  /// \brief Constructor.
  TH2FMean() = default;
  TH2FMean(const char* name, const char* title, Int_t nbinsx, Double_t xlow, Double_t xup, Int_t nbinsy, Double_t ylow, Double_t yup) : TH2F(name, title, nbinsx, xlow, xup, nbinsy, ylow, yup) {}
  /// \brief Default destructor
  virtual ~TH2FMean() = default;

  void merge(MergeInterface* const other) override;

 private:
  std::string mTreatMeAs = "TH2F"; // the name of the class this object should be considered as when drawing in QCG.

  ClassDefOverride(TH2FMean, 1);
};

} // namespace o2::quality_control_modules::phos

#endif // QC_MODULE_PHOS_TH2FMEAN_H
