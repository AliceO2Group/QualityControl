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

#include "GLO/Reductors.h"
#include "GLO/Helpers.h"

#include <TF1.h>
#include <TH1.h>

namespace o2::quality_control_modules::glo
{

void K0sFitReductor::update(TObject* obj)
{
  auto f = dynamic_cast<TF1*>(obj);
  if (!f) {
    return;
  }

  mStats.mean = f->GetParameter(helpers::K0sFitter::Parameters::Mass);
  mStats.sigma = f->GetParameter(helpers::K0sFitter::Parameters::Sigma);
}

void MTCReductor::update(TObject* obj)
{
  auto h = dynamic_cast<TH1*>(obj);
  if (!h) {
    return;
  }

  mStats.pt = (float)h->GetBinContent(h->FindBin(1.5));
}

} // namespace o2::quality_control_modules::glo
