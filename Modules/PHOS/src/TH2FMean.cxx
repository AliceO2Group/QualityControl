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
/// \file   TH2FMean.cxx
/// \author Dmitri Peresunko
///

#include "PHOS/TH2FMean.h"

namespace o2::quality_control_modules::phos
{

void TH2FMean::merge(MergeInterface* const other)
{
  // special merge method to approximately combine two histograms
  auto otherHisto = dynamic_cast<const TH2FMean* const>(other);
  if (otherHisto) {
    // Make approximate averaging with weights proportional to total number of entries
    double sum = this->GetEntries() + otherHisto->GetEntries();
    if (sum > 0) {
      double w1 = this->GetEntries() / sum;
      double w2 = otherHisto->GetEntries() / sum;
      this->Scale(w1);
      this->Add(otherHisto, w2);
    }
  }
}
} // namespace o2::quality_control_modules::phos
