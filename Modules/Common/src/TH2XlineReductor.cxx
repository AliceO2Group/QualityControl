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
/// \file   TH2XlineReductor.cxx
/// \author Ivan Ravasenga on the model from Piotr Konopka
///

#include <TH2.h>
#include <TMath.h>
#include "Common/TH2XlineReductor.h"

namespace o2::quality_control_modules::common
{

void* TH2XlineReductor::getBranchAddress()
{
  return &mStats;
}

/*std::vector<Double_t> TH2XlineReductor::getMean()
{
  return mStats.mean;
}*/

const char* TH2XlineReductor::getBranchLeafList()
{
  return " ";
}

void TH2XlineReductor::update(TObject* obj)
{
  auto histo = dynamic_cast<TH2*>(obj);
  if (histo) {
    Double_t entriesx = 0.;
    Double_t sum = 0.;
    for(int iy=1; iy<=histo->GetNbinsY(); iy++){
      for(int ix=1; ix<=histo->GetNbinsX(); ix++){
        if(histo->GetBinContent(ix,iy) > 0.) entriesx++;
        sum+=histo->GetBinContent(ix,iy);
      }
      Double_t meanx = sum/entriesx;
      mStats.mean.push_back(meanx);//mean
      mStats.entries.push_back(entriesx);//entries
      mStats.mean_scaled.push_back(meanx*512.*1024.);
      sum = 0.;
      for(int ix=1; ix<=histo->GetNbinsX(); ix++){
        Double_t binc = histo->GetBinContent(ix,iy);
        if(binc > 0.) sum+=(binc-meanx)*(binc-meanx);
      }
      mStats.stddev.push_back(TMath::Sqrt(sum/(entriesx-1)));//std dev
      entriesx = 0.;
      sum = 0.;
    }//end loop on y bins
  }//end if
}

} // namespace o2::quality_control_modules::common
