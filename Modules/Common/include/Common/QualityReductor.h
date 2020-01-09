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
/// \file   QualityReductor.h
/// \author Piotr Konopka
///
#ifndef QUALITYCONTROL_QUALITYREDUCTOR_H
#define QUALITYCONTROL_QUALITYREDUCTOR_H

#include "QualityControl/Reductor.h"
#include "QualityControl/Quality.h"

namespace o2::quality_control_modules::common
{

// todo fix plots with qualities names - ugly axes with names
/// \brief A Reductor of QualityObjects, stores a name and level of a Quality
///
/// A Reductor of QualityObjects, stores a name and level of a Quality
/// It produces a branch in the format: "level/i:name/C"
class QualityReductor : public quality_control::postprocessing::Reductor
{
 public:
  QualityReductor() = default;
  ~QualityReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

  static constexpr size_t NAME_SIZE = 8;
  private:
  struct {
    UInt_t level = quality_control::core::Quality::NullLevel;
    char name[NAME_SIZE];
  } mQuality;
};

} // namespace o2::quality_control_modules::common

#endif //QUALITYCONTROL_QUALITYREDUCTOR_H
