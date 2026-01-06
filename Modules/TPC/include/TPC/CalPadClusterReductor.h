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

//
// file   CalPadClusterReductor.h
// author Marcel Lesch
//
#ifndef QC_MODULE_TPC_CALPADCLUSTERDUCTOR_H
#define QC_MODULE_TPC_CALPADCLUSTERDUCTOR_H

#include "QualityControl/ReductorTObject.h"
#if __has_include("TPCBase/CalDet.h")
#include "TPCBase/CalDet.h"
#else
#include "TPCBaseRecSim/CalDet.h"
#endif
#include "TPCBase/CalArray.h"
#include "TPCQC/Clusters.h"

namespace o2::quality_control_modules::tpc
{

/// \brief A Reductor of cluster data stored as CalPads.
///
/// A Reductor of cluster data stored as CalPads. Stores number of entries, mean, standard deviation, median and rms
/// for each NClusters, QMax, QTot, SigmaTime, SigmaPad and TimeBin individually.
/// It produces a branch in the format: "NClusters[4][72]/F:QMax[4][72]:QTot[4][72]:SigmaTime[4][72]:SigmaPad[4][72]:TimeBin[4][72]"
/// First iterator holds entries [0], mean [1], standard deviation [2] and median [3]
/// Second iterator runs over all 72 ROCs

class CalPadClusterReductor : public quality_control::postprocessing::ReductorTObject
{
 public:
  CalPadClusterReductor() = default;
  ~CalPadClusterReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  struct {
    Float_t NClusters[4][72];
    Float_t QMax[4][72];
    Float_t QTot[4][72];
    Float_t SigmaTime[4][72];
    Float_t SigmaPad[4][72];
    Float_t TimeBin[4][72];
  } mCalPad;

  o2::tpc::CalPad& GetCalPad(o2::tpc::qc::Clusters& clusters, int dataType);
  typedef Float_t (*pointer_to_arrays)[72];
  pointer_to_arrays getArrayPointer(int dataType);

}; // class CalPadClusterReductor : public quality_control::postprocessing::Reductor

} // namespace o2::quality_control_modules::tpc

#endif // QC_MODULE_TPC_CALPADCLUSTERDUCTOR_H
