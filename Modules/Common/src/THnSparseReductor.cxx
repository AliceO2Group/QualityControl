// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   THnSparseReductor.cxx
/// \author Ivan Ravasenga from the model by Piotr Konopka
///

#include <THnSparse.h>
#include <TH1.h>
#include "Common/THnSparseReductor.h"

namespace o2::quality_control_modules::common
{

void* THnSparseReductor::getBranchAddress()
{
  return &mStats;
}

const char* THnSparseReductor::getBranchLeafList()
{
  return Form("mean[%i]/D:stddev[%i]:entries[%i]", nDim, nDim, nDim);
}

void THnSparseReductor::update(TObject* obj)
{
  // todo: use GetStats() instead?
  auto sparsehisto = dynamic_cast<THnSparse*>(obj);
  if (sparsehisto) {
    Int_t dim = sparsehisto->GetNdimensions();
    for (int i = 0; i < nDim; i++) {
      if (i < dim) {
        TH1D* hproj = (TH1D*)sparsehisto->Projection(i);
        mStats.entries[i] = hproj->GetEntries();
        mStats.mean[i] = hproj->GetMean();
        mStats.stddev[i] = hproj->GetStdDev();
        delete hproj;
      } else {
        mStats.entries[i] = -1;
        mStats.mean[i] = -1;
        mStats.stddev[i] = -1;
      }
    }
  }
}

} // namespace o2::quality_control_modules::common
