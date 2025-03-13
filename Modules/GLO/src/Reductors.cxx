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

#include "Common/Utils.h"

#include "GLO/Reductors.h"
#include "GLO/Helpers.h"

#include <TF1.h>
#include <TH1.h>
#include <TProfile.h>
#include <TFitResult.h>
#include <TFitResultPtr.h>

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

const char* MTCReductor::getBranchLeafList()
{
  mPt = common::internal::stringToType<Float_t>(mCustomParameters.atOrDefaultValue("pt", "0"));

  return "mtc/F";
};

void MTCReductor::update(TObject* obj)
{
  auto h = dynamic_cast<TH1*>(obj);
  if (!h) {
    return;
  }

  mStats.mtc = (float)h->GetBinContent(h->FindBin(mPt));
}

const char* PVITSReductor::getBranchLeafList()
{
  mR0 = common::internal::stringToType<Float_t>(mCustomParameters.atOrDefaultValue("r0", "0"));
  mR1 = common::internal::stringToType<Float_t>(mCustomParameters.atOrDefaultValue("r1", "0"));

  return "pol0/F:pol1/F";
};

void PVITSReductor::update(TObject* obj)
{
  auto p = dynamic_cast<TProfile*>(obj);
  if (!p) {
    return;
  }

  auto res = p->Fit("pol1", "QSNC", "", mR0, mR1);
  if ((Int_t)res != 0) {
    return;
  }

  mStats.pol0 = (Float_t)res->Parameter(0);
  mStats.pol1 = (Float_t)res->Parameter(1);
}

} // namespace o2::quality_control_modules::glo
