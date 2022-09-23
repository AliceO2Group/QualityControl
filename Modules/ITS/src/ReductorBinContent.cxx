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
/// \file   ReductorBinContent.cxx
/// \author Artem Kotliarov
///

#include "QualityControl/QcInfoLogger.h"
#include <TH1.h>
#include <TH2.h>
#include <TH2I.h>
#include <TObject.h>
#include <TClass.h>
#include "ITS/ReductorBinContent.h"
#include <iostream>
#include <fmt/format.h>

namespace o2::quality_control_modules::its
{
void* ReductorBinContent::getBranchAddress()
{
   return &mStats;
}

const char* ReductorBinContent::getBranchLeafList()
{
   return Form("binContent[%i]/D:integral[%i]", nFlags, nTriggers);
}

void ReductorBinContent::update(TObject* obj)
{
   // initialize arrays
   for(int j = 0; j < nFlags; j++){
      mStats.binContent[j] = -1.0;
   }

   for(int j = 0; j < nTriggers; j++){
      mStats.integral[j] = -1.0;
   }

   // Access the histograms embedded in 'obj'.
   std::string histoClass = obj->IsA()->GetName();

   if(histoClass.find("TH1") != std::string::npos){
      TH1 *histo = (TH1*) obj->Clone("LaneStatusSummary_Flags");
      int numberOfBins = histo->GetXaxis()->GetNbins();

      for(int j = 1; j <= numberOfBins; j++){
         mStats.binContent[j - 1] = histo->GetBinContent(j);
      }
      delete histo;
   } else if (histoClass.find("TH2") != std::string::npos){

      TH2 *histo = (TH2*) obj->Clone("TriggerCount");

      int numberOfBinsX = histo->GetXaxis()->GetNbins();
      int numberOfBinsY = histo->GetYaxis()->GetNbins();

      for(int j = 1; j <= numberOfBinsY; j++){
         mStats.integral[j - 1] = histo->Integral(1, numberOfBinsX, j, j); // Summation over all Fee IDs for a given trigger
      }
      delete histo;
   } else {
      ILOG(Error, Support) << "Error: 'histo' not found." << ENDM;
   }
}
} // namespace o2::quality_control_modules::its
