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
#ifndef QC_MODULE_FOCAL_PADDECODER_H
#define QC_MODULE_FOCAL_PADDECODER_H

#include <array>
#include <vector>
#include <functional>
#include <cstdint>

#include <gsl/span>

#include "FOCAL/PadData.h"

namespace o2::quality_control_modules::focal
{
class PadDecoder
{
 public:
  PadDecoder() = default;
  ~PadDecoder() = default;

  void setTriggerWinDur(int windur) { mWin_dur = windur; }

  void reset();
  void decodeEvent(gsl::span<const PadGBTWord> padpayload);
  const PadData& getData() const { return mData; }

 protected:
 private:
  int mWin_dur = 20;
  PadData mData;
};

} // namespace o2::quality_control_modules::focal

#endif // QC_MODULE_FOCAL_PADDECODER_H