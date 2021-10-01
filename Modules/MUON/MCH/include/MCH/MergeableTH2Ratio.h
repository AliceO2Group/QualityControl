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

/// \file MergeableTH2Ratio.h
/// \brief An example of a custom TH2Quotient inheriting MergeInterface
///
/// \author Piotr Konopka, piotr.jan.konopka@cern.ch, Sebastien Perrin

#ifndef O2_MERGEABLETH2RATIO_H
#define O2_MERGEABLETH2RATIO_H

#include <sstream>
#include <iostream>
#include <TObject.h>
#include <TH2.h>
#include <TList.h>
#include "Mergers/MergeInterface.h"

using namespace std;
namespace o2::quality_control_modules::muonchambers
{

class MergeableTH2Ratio : public TH2F, public o2::mergers::MergeInterface
{
 public:
  MergeableTH2Ratio() = default;

  MergeableTH2Ratio(MergeableTH2Ratio const& copymerge)
    : TH2F(*(copymerge.getNum())), o2::mergers::MergeInterface(), mhistoNum(copymerge.getNum()), mhistoDen(copymerge.getDen())
  {
  }

  MergeableTH2Ratio(const char* name, const char* title, TH2F* histonum, TH2F* histoden)
    : TH2F(*histonum), o2::mergers::MergeInterface(), mhistoNum(histonum), mhistoDen(histoden)
  {
    SetNameTitle(name, title);
    update();
  }

  ~MergeableTH2Ratio() override = default;

  void merge(MergeInterface* const other) override
  {
    mhistoNum->Add(dynamic_cast<const MergeableTH2Ratio* const>(other)->getNum());
    mhistoDen->Add(dynamic_cast<const MergeableTH2Ratio* const>(other)->getDen());
    update();
  }

  TH2F* getNum() const
  {
    return mhistoNum;
  }

  TH2F* getDen() const
  {
    return mhistoDen;
  }

  void update()
  {
    static constexpr double sOrbitLengthInNanoseconds = 3564 * 25;
    static constexpr double sOrbitLengthInMicroseconds = sOrbitLengthInNanoseconds / 1000;
    static constexpr double sOrbitLengthInMilliseconds = sOrbitLengthInMicroseconds / 1000;
    const char* name = this->GetName();
    const char* title = this->GetTitle();
    Reset();
    Divide(mhistoNum, mhistoDen);
    SetNameTitle(name, title);
    // convertion to KHz units
    Scale(1. / sOrbitLengthInMilliseconds);
    SetOption("colz");
  }

 private:
  TH2F* mhistoNum{ nullptr };
  TH2F* mhistoDen{ nullptr };
  std::string mTreatMeAs = "TH2F";

  ClassDefOverride(MergeableTH2Ratio, 1);
};

} // namespace o2::quality_control_modules::muonchambers

#endif //O2_MERGEABLETH2RATIO_H
