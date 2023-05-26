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

/// \file TH1Ratio.h
/// \brief A generic TH1Ratio inheriting MergeInterface
///
/// \author Piotr Konopka, piotr.jan.konopka@cern.ch, Andrea Ferrero

#ifndef QUALITYCONTROL_TH1RATIO_H
#define QUALITYCONTROL_TH1RATIO_H

#include "Mergers/MergeInterface.h"
#include <TH1F.h>
#include <TH1D.h>

namespace o2::quality_control_modules::common
{

template <class T>
class TH1Ratio : public T, public o2::mergers::MergeInterface
{
 public:
  TH1Ratio() = default;
  TH1Ratio(TH1Ratio const& copymerge);
  TH1Ratio(const char* name, const char* title, int nbinsx, double xmin, double xmax, bool uniformScaling = false);
  TH1Ratio(const char* name, const char* title, bool uniformScaling = false);

  ~TH1Ratio();

  void init();

  void merge(MergeInterface* const other) override;

  T* getNum() const
  {
    return mHistoNum;
  }

  T* getDen() const
  {
    return mHistoDen;
  }

  bool hasUniformScaling() const
  {
    return mUniformScaling;
  }

  void update();

  // functions inherited from TH1x
  void Reset(Option_t* option = "") override;
  void Copy(TObject& obj) const override;
  Bool_t Add(const TH1* h1, const TH1* h2, Double_t c1 = 1, Double_t c2 = 1) override;
  Bool_t Add(const TH1* h1, Double_t c1 = 1) override;
  void SetBins(Int_t nx, Double_t xmin, Double_t xmax) override;

 private:
  T* mHistoNum{ nullptr };
  T* mHistoDen{ nullptr };
  bool mUniformScaling{ true };
  std::string mTreatMeAs{ T::Class_Name() };

  ClassDefOverride(TH1Ratio, 1);
};

typedef TH1Ratio<TH1F> TH1FRatio;
typedef TH1Ratio<TH1D> TH1DRatio;

} // namespace o2::quality_control_modules::common

#include "Common/TH1Ratio.inl"

#endif // QUALITYCONTROL_TH1RATIO_H
