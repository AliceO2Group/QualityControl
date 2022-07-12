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

#ifndef QC_MODULE_MUON_COMMON_TRACKS_CHECK_H
#define QC_MODULE_MUON_COMMON_TRACKS_CHECK_H

#include "QualityControl/CheckInterface.h"
#include <vector>

namespace o2::quality_control_modules::muon
{

class TracksCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  TracksCheck();
  ~TracksCheck() override;

  void configure() override;

  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMa) override;

  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  std::vector<int> mBunchCrossings;

  ClassDefOverride(TracksCheck, 1);
};
} // namespace o2::quality_control_modules::muon

#endif
