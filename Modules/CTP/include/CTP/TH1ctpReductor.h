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
/// \file   TH1ctpReductor.h
/// \author Lucia Anna Tarasovicova
///
#ifndef QUALITYCONTROL_TH1CTPREDUCTOR_H
#define QUALITYCONTROL_TH1CTPREDUCTOR_H

#include "QualityControl/ReductorTObject.h"

namespace o2::quality_control_modules::ctp
{

/// \brief A Reductor which obtains the characteristics of TH1 including the bin contents.
/// Based on the example by Piotr Jan Konopka
/// It produces a branch in the format: "mean/D:stddev:entries:inputs[%i]:classes[%i]"
class TH1ctpReductor : public quality_control::postprocessing::ReductorTObject
{
 public:
  TH1ctpReductor() = default;
  ~TH1ctpReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

  void SetClassIndexes(int inMB1, int inMB2, int inDMC, int inEMC, int inPHO)
  {
    mMinBias1ClassIndex = inMB1;
    mMinBias2ClassIndex = inMB2;
    mPH0ClassIndex = inPHO;
    mDMCClassIndex = inDMC;
    mEMCClassIndex = inEMC;
  }
  void SetInputIndexes(int inMB1, int inMB2, int inDMC, int inEMC, int inPHO)
  {
    mMinBias1InputIndex = inMB1;
    mMinBias2InputIndex = inMB2;
    mPH0ClassIndex = inPHO;
    mDMCInputIndex = inDMC;
    mEMCInputIndex = inEMC;
  }
  void SetMinBias1ClassIndex(int in)
  {
    mMinBias1ClassIndex = in;
  }
  void SetMinBias2ClassIndex(int in)
  {
    mMinBias2ClassIndex = in;
  }
  void SetDMCClassIndex(int in)
  {
    mDMCClassIndex = in;
  }
  void SetPHOClassIndex(int in)
  {
    mPH0ClassIndex = in;
  }
  void SetEMCClassIndex(int in)
  {
    mEMCClassIndex = in;
  }
  void SetMinBias1InputIndex(int in)
  {
    mMinBias1InputIndex = in;
  }
  void SetMinBias2InputIndex(int in)
  {
    mMinBias2InputIndex = in;
  }
  void SetDMCInputIndex(int in)
  {
    mDMCInputIndex = in;
  }
  void SetPHOInputIndex(int in)
  {
    mPHOInputIndex = in;
  }
  void SetEMCInputIndex(int in)
  {
    mEMCInputIndex = in;
  }

  int GetMinBias1ClassIndex()
  {
    return mMinBias1ClassIndex;
  }
  int GetMinBias1InputIndex()
  {
    return mMinBias1InputIndex;
  }

 private:
  static constexpr int nInputs = 48;
  int mMinBias1ClassIndex = 65;
  int mMinBias2ClassIndex = 65;
  int mDMCClassIndex = 65;
  int mEMCClassIndex = 65;
  int mPH0ClassIndex = 65;
  int mMinBias1InputIndex = 49;
  int mMinBias2InputIndex = 49;
  int mDMCInputIndex = 49;
  int mEMCInputIndex = 49;
  int mPHOInputIndex = 49;

  struct {
    Double_t mean;
    Double_t stddev;
    Double_t entries;
    Double_t classContentMinBias1;
    Double_t classContentMinBias2;
    Double_t classContentDMC;
    Double_t classContentEMC;
    Double_t classContentPHO;
    Double_t inputContentMinBias1;
    Double_t inputContentMinBias2;
    Double_t inputContentDMC;
    Double_t inputContentEMC;
    Double_t inputContentPHO;
  } mStats;
};

} // namespace o2::quality_control_modules::ctp

#endif // QUALITYCONTROL_TH1CTPREDUCTOR_H
