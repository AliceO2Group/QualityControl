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
/// \file   ReductorIntegralContent.cxx
/// \author Artem Kotliarov
///

#include "QualityControl/QcInfoLogger.h"
#include <TH1.h>
#include <TH2.h>
#include <TH2I.h>
#include <TObject.h>
#include <TClass.h>
#include "ITS/ReductorIntegralContent.h"
#include <iostream>
#include <fmt/format.h>

namespace o2::quality_control_modules::its
{
void* ReductorIntegralContent::getBranchAddress()
{
  return &mStats;
}

const char* ReductorIntegralContent::getBranchLeafList()
{
  return Form("integral[%i]/D", nBins);
}

void ReductorIntegralContent::update(TObject* obj)
{

  // initialize arrays
  //

  for (int j = 0; j < 100; j++) {
    mStats.integral[j] = 0;
  }

  // Access the histograms embedded in 'obj'.
  std::string histoClass = obj->IsA()->GetName();
  TH2* histo = (TH2*)obj->Clone("clone");
  int numberOfBinsX = histo->GetXaxis()->GetNbins();
  int numberOfBinsY = histo->GetYaxis()->GetNbins();
  std::string name = obj->GetName();
  if (histoClass.find("TH2") != std::string::npos) {
    if (name.find("PayloadSize") != std::string::npos) {

      for (int id = 0; id < 9; id++) {
        for (int ix = FeeBoundary[id] + 1; ix <= FeeBoundary[id + 1]; ix++) {
          for (int iy = 1; iy <= numberOfBinsY; iy++) {
            mStats.integral[id] += histo->GetBinContent(ix, iy) * histo->GetYaxis()->GetBinCenter(iy);
          }
        }
      }
    } else if (name.find("TriggerVsFeeid") != std::string::npos) {
      for (int j = 1; j <= numberOfBinsY; j++) {
        mStats.integral[j - 1] = histo->Integral(1, numberOfBinsX, j, j); // Summation over all Fee IDs for a given trigger
      }
    }
    delete histo;
  } else {
    ILOG(Error, Support) << "Error: 'histo' not found." << ENDM;
  }
}
} // namespace o2::quality_control_modules::its
