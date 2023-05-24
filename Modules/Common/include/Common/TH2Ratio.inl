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

/// \file TH2Ratio.inl
/// \brief An generic TH2Ratio inheriting MergeInterface
///
/// \author Piotr Konopka, piotr.jan.konopka@cern.ch, Andrea Ferrero

#include "QualityControl/QcInfoLogger.h"

namespace o2::quality_control_modules::common
{

template<class T>
TH2Ratio<T>::TH2Ratio(TH2Ratio const& copymerge)
  : T(copymerge.GetName(), copymerge.GetTitle(),
         copymerge.getNum()->GetXaxis()->GetNbins(), copymerge.getNum()->GetXaxis()->GetXmin(), copymerge.getNum()->GetXaxis()->GetXmax(),
         copymerge.getNum()->GetYaxis()->GetNbins(), copymerge.getNum()->GetYaxis()->GetXmin(), copymerge.getNum()->GetYaxis()->GetXmax()),
    o2::mergers::MergeInterface(),
    mUniformScaling(copymerge.hasUniformScaling())
{
  Bool_t bStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  mHistoNum = (T*)copymerge.getNum()->Clone();
  mHistoDen = (T*)copymerge.getDen()->Clone();
  TH1::AddDirectory(bStatus);

  init();
}

template<class T>
TH2Ratio<T>::TH2Ratio(const char* name, const char* title, int nbinsx, double xmin, double xmax, int nbinsy, double ymin, double ymax, bool uniformScaling)
  : T(name, title, nbinsx, xmin, xmax, nbinsy, ymin, ymax),
    o2::mergers::MergeInterface(),
    mUniformScaling(uniformScaling)
{
  Bool_t bStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  mHistoNum = new T("num", "num", nbinsx, xmin, xmax, nbinsy, ymin, ymax);
  if (mUniformScaling) {
    mHistoDen = new T("den", "den", 1, -1, 1, 1, -1, 1);
  } else {
    mHistoDen = new T("den", "den", nbinsx, xmin, xmax, nbinsy, ymin, ymax);
  }
  TH1::AddDirectory(bStatus);

  init();
}

template<class T>
TH2Ratio<T>::TH2Ratio(const char* name, const char* title, bool uniformScaling)
  : T(name, title, 10, 0, 10, 10, 0, 10),
    o2::mergers::MergeInterface(),
    mUniformScaling(uniformScaling)
{
  Bool_t bStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);
  mHistoNum = new T("num", "num", 10, 0, 10, 10, 0, 10);
  if (mUniformScaling) {
    mHistoDen = new T("den", "den", 1, -1, 1, 1, -1, 1);
  } else {
    mHistoDen = new T("den", "den", 10, 0, 10, 10, 0, 10);
  }
  TH1::AddDirectory(bStatus);

  init();
}

template<class T>
TH2Ratio<T>::~TH2Ratio()
{
  if (mHistoNum) {
    delete mHistoNum;
  }

  if (mHistoDen) {
    delete mHistoDen;
  }
}

template<class T>
void TH2Ratio<T>::init()
{
  if (!mHistoNum || !mHistoDen) {
    return;
  }
  mHistoNum->Sumw2();
  mHistoDen->Sumw2();
  T::Sumw2();

  update();
}

template<class T>
void TH2Ratio<T>::merge(MergeInterface* const other)
{
  if (!mHistoNum || !mHistoDen) {
    return;
  }

  mHistoNum->Add(dynamic_cast<const TH2Ratio* const>(other)->getNum());
  mHistoDen->Add(dynamic_cast<const TH2Ratio* const>(other)->getDen());
  update();
}

template<class T>
void TH2Ratio<T>::update()
{
  if (!mHistoNum || !mHistoDen) {
    return;
  }

  const char* name = this->GetName();
  const char* title = this->GetTitle();

  T::Reset();
  T::SetNameTitle(name, title);
  T::GetXaxis()->Set(mHistoNum->GetXaxis()->GetNbins(), mHistoNum->GetXaxis()->GetXmin(), mHistoNum->GetXaxis()->GetXmax());
  T::GetYaxis()->Set(mHistoNum->GetYaxis()->GetNbins(), mHistoNum->GetYaxis()->GetXmin(), mHistoNum->GetYaxis()->GetXmax());
  T::SetBinsLength();

  T::Add(mHistoNum);

  if (mUniformScaling) {
    double entries = mHistoDen->GetBinContent(1, 1);
    double norm = (entries > 0) ? 1.0 / entries : 0;
    T::Scale(norm);
  } else {
    T::Divide(mHistoDen);
  }
}

template<class T>
void TH2Ratio<T>::Reset(Option_t* option)
{
  if (mHistoNum) {
    mHistoNum->Reset(option);
  }

  if (mHistoDen) {
    mHistoDen->Reset(option);
  }

  T::Reset(option);
}

template<class T>
void TH2Ratio<T>::Copy(TObject& obj) const
{
  auto dest = dynamic_cast<TH2Ratio*>(&obj);
  if (!dest) {
    return;
  }

  getNum()->Copy(*(dest->getNum()));
  getDen()->Copy(*(dest->getDen()));
  T::Copy(obj);

  dest->update();
}

template<class T>
Bool_t TH2Ratio<T>::Add(const TH1* h1, const TH1* h2, Double_t c1, Double_t c2)
{
  auto m1 = dynamic_cast<const TH2Ratio*>(h1);
  if (!m1) {
    return kFALSE;
  }

  auto m2 = dynamic_cast<const TH2Ratio*>(h2);
  if (!m2) {
    return kFALSE;
  }

  if (!getNum()->Add(m1->getNum(), m2->getNum(), c1, c2)) {
    return kFALSE;
  }
  if (!getDen()->Add(m1->getDen(), m2->getDen(), c1, c2)) {
    return kFALSE;
  }

  update();
  return kTRUE;
}

template<class T>
Bool_t TH2Ratio<T>::Add(const TH1* h1, Double_t c1)
{
  auto m1 = dynamic_cast<const TH2Ratio*>(h1);
  if (!m1) {
    return kFALSE;
  }

  if (!getNum()->Add(m1->getNum(), c1)) {
    return kFALSE;
  }
  if (!getDen()->Add(m1->getDen(), c1)) {
    return kFALSE;
  }

  update();
  return kTRUE;
}

template<class T>
void TH2Ratio<T>::SetBins(Int_t nx, Double_t xmin, Double_t xmax,
                       Int_t ny, Double_t ymin, Double_t ymax)
{
  getNum()->SetBins(nx, xmin, xmax, ny, ymin, ymax);
  getDen()->SetBins(nx, xmin, xmax, ny, ymin, ymax);
  T::SetBins(nx, xmin, xmax, ny, ymin, ymax);
}

} // namespace o2::quality_control_modules::common
