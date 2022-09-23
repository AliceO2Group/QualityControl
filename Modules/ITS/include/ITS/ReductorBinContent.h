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
/// \file   ReductorBinContent.h
/// \author Artem Kotliarov
///
#ifndef QUALITYCONTROL_REDUCTORBINCONTENT_H
#define QUALITYCONTROL_REDUCTORBINCONTENT_H

#include "QualityControl/Reductor.h"
#include <vector>

namespace o2::quality_control_modules::its
{

class ReductorBinContent : public quality_control::postprocessing::Reductor
{
   public:
      ReductorBinContent() = default;
      virtual ~ReductorBinContent() = default;

      void* getBranchAddress() override;
      const char* getBranchLeafList() override;
      void update(TObject* obj) override;

   private:
      static constexpr int nFlags = 3;
      static constexpr int nTriggers = 13;

      struct mystat {
         Double_t binContent[nFlags]; // Bin content in a specified slice
         Double_t integral[nTriggers]; // Integral over all Fee ID
      };

      mystat mStats;
};
} // namespace o2::quality_control_modules::its

#endif // QUALITYCONTROL_REDUCTORBINCONTENT_H
