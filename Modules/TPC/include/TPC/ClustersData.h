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
/// \file   Clusters.h
/// \author Jens Wiechula
/// \author Thomas Klemenz
///

#ifndef QC_MODULE_TPC_CLUSTERSDATA_H
#define QC_MODULE_TPC_CLUSTERSDATA_H

// ROOT includes
#include "TObject.h"

// O2 includes
#include <Mergers/MergeInterface.h>
#include <Rtypes.h>
#include "TPCQC/Clusters.h"

using o2::mergers::MergeInterface;
using o2::tpc::qc::Clusters;

namespace o2::quality_control_modules::tpc
{

class ClustersData final : public TObject, public MergeInterface
{
 public:
  ClustersData()
    : o2::mergers::MergeInterface(), TObject()
  {
  }

  ClustersData(std::string_view nclName)
    : o2::mergers::MergeInterface(), TObject(), mClusters{ nclName }
  {
  }

  ~ClustersData() final = default;

  virtual void merge(MergeInterface* other) final;

  Clusters& getClusters() { return mClusters; }

  void setName(std::string_view name) { mName = name.data(); }

  virtual const char* GetName() const override { return mName.data(); }

 private:
  Clusters mClusters;
  std::string mName;

  ClassDefOverride(ClustersData, 1);
};

inline void ClustersData::merge(MergeInterface* other)
{
  auto otherCl = dynamic_cast<ClustersData* const>(other);
  if (otherCl) {
    mClusters.merge(otherCl->mClusters);
  }
}

} // namespace o2::quality_control_modules::tpc

#endif
