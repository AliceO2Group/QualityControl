// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

//
// file   ROCReductor.h
// author Cindy Mordasini
// author Marcel Lesch
//
#ifndef QC_MODULE_TPC_ROCREDUCTOR_H
#define QC_MODULE_TPC_ROCREDUCTOR_H

#include "QualityControl/Reductor.h"
#include "TPCBase/CalDet.h"
//#include "CCDB/TObjectWrapper.h"

namespace o2::quality_control_modules::tpc
{

/// \brief A Reductor of ROC, stores mean, standard deviation and median for each ROC.
///
/// A Reductor of ROC, stores mean, standard deviation and median for each ROC.
/// It produces a branch in the format: "mean[72]/F:stddev[72]:median[72]"
class ROCReductor : public quality_control::postprocessing::Reductor
{
 public:
  ROCReductor() = default;
  ~ROCReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  struct {
    Float_t mean[72];
    Float_t stddev[72];
    Float_t median[72];
  } mCalPad;

}; // class ROCReductor : public quality_control::postprocessing::Reductor

} // namespace o2::quality_control_modules::tpc

#endif //QC_MODULE_TPC_ROCREDUCTOR_H
