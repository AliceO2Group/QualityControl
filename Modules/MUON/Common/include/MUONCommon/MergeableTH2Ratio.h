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
/// \author Piotr Konopka, piotr.jan.konopka@cern.ch, Sebastien Perrin, Andrea Ferrero

#ifndef O2_MERGEABLETH2RATIO_H
#define O2_MERGEABLETH2RATIO_H

#include <sstream>
#include <iostream>
#include <TObject.h>
#include <TH2.h>
#include <TList.h>
#include "Mergers/MergeInterface.h"

using namespace std;
namespace o2::quality_control_modules::muon
{

class MergeableTH2Ratio : public TH2F, public o2::mergers::MergeInterface
{
 public:
  MergeableTH2Ratio() = default;

  MergeableTH2Ratio(MergeableTH2Ratio const& copymerge);

  MergeableTH2Ratio(const char* name, const char* title, int nbinsx, double xmin, double xmax, int nbinsy, double ymin, double ymax, double scaling = 1.);

  MergeableTH2Ratio(const char* name, const char* title, double scaling = 1.);

  ~MergeableTH2Ratio();

  void merge(MergeInterface* const other) override;

  TH2F* getNum() const
  {
    return mHistoNum;
  }

  TH2F* getDen() const
  {
    return mHistoDen;
  }

  double getScalingFactor() const
  {
    return mScalingFactor;
  }

  void update();

  void beautify();

 private:
  TH2F* mHistoNum{ nullptr };
  TH2F* mHistoDen{ nullptr };
  std::string mTreatMeAs = "TH2F";
  double mScalingFactor = 1.;

  ClassDefOverride(MergeableTH2Ratio, 1);
};

} // namespace o2::quality_control_modules::muon

#endif // O2_MERGEABLETH2RATIO_H
