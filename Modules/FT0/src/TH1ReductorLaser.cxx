/// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
/// See https://alice-o2.web.cern.ch/copyright for details of the copyright
/// holders. All rights not expressly granted are reserved.
//
/// This software is distributed under the terms of the GNU General Public
/// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
/// In applying this license CERN does not waive the privileges and immunities
/// granted to it by virtue of its status as an Intergovernmental Organization
/// or submit itself to any jurisdiction.

///
/// \file   TH1ReductorLaser.cxx
/// \author Piotr Konopka, developed to laser QC by Sandor Lokos
/// (sandor.lokos@cern.ch)
///

#include "FT0/TH1ReductorLaser.h"
#include "QualityControl/QcInfoLogger.h"
#include <TF2.h>
#include <TH2.h>
#include <sstream>

namespace o2::quality_control_modules::ft0
{
void* TH1ReductorLaser::getBranchAddress() { return &mStats; }
const char* TH1ReductorLaser::getBranchLeafList()
{
  return Form("validity1/D:validity2/D:mean1/D:mean2/D:mean[%i]/D:stddev1:stddev2:stddev[%i]", NChannel, NChannel);
}

void TH1ReductorLaser::update(TObject* obj)
{
  if (auto histo = dynamic_cast<TH2F*>(obj)) {
    int channel = -1;
    sscanf(histo->GetName(), "%*[^0-9]%d", &channel);
    if (channel < NChannel) {
      for (int ichannel = 1; ichannel < NChannel; ichannel++) {
        TH1* bc_projection = histo->ProjectionY(Form("first peak in BC #%d", ichannel), ichannel, ichannel + 1);
        mStats.mean[ichannel] = bc_projection->GetMean();
      }
    } else {
      TH1* bc_projection = histo->ProjectionY("bc_projection", 0, -1);
      int ibc = 0;
      int ibc_max = 0;
      if (bc_projection->GetEntries() > 0) {
        ibc = bc_projection->GetMean() - 2. * bc_projection->GetStdDev();
        ibc_max = bc_projection->GetMean() + 2. * bc_projection->GetStdDev();
      }

      TH1* slice_first_peak;
      bool gotFirstPeak = false;
      mStats.mean1 = 0.;
      mStats.stddev1 = 0.;
      mStats.validity1 = 0.;
      while (!gotFirstPeak && ibc < ibc_max) {
        slice_first_peak = histo->ProjectionX(Form("first peak in BC #%d", ibc), ibc, ibc + 1);
        if (slice_first_peak->GetEntries() > 1000) {
          mStats.mean1 = slice_first_peak->GetMean();
          mStats.stddev1 = slice_first_peak->GetStdDev();
          mStats.validity1 = 1.;
          gotFirstPeak = true;
          ibc += 2;
          break;
        } else
          ibc++;
      }

      TH1* slice_second_peak;
      bool gotSecondPeak = false;
      mStats.mean2 = 0.;
      mStats.stddev2 = 0.;
      mStats.validity2 = 0.;
      while (!gotSecondPeak && gotFirstPeak && ibc < ibc_max) {
        slice_second_peak = histo->ProjectionX(Form("second peak in BC #%d", ibc), ibc, ibc + 1);
        if (slice_second_peak->GetEntries() > 1000) {
          mStats.mean2 = slice_second_peak->GetMean();
          mStats.stddev2 = slice_second_peak->GetStdDev();
          mStats.validity2 = 1.;
          gotSecondPeak = true;
          break;
        } else
          ibc++;
      }

      if (!gotSecondPeak)
        ILOG(Warning) << "TH1ReductorLaser: one of the peaks of the reference PMT is missing!" << ENDM;
      if (!gotFirstPeak && !gotSecondPeak)
        ILOG(Warning) << "TH1ReductorLaser: cannot find peaks of the reference PMT distribution at all !" << ENDM;
    }
  }
}
} // namespace o2::quality_control_modules::ft0
