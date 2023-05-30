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
/// \file   QcMFTClusterTask.h
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova
///

#ifndef QC_MFT_CLUSTER_TASK_H
#define QC_MFT_CLUSTER_TASK_H
#include <TH1F.h>
#include <TH2F.h>
#include <DataFormatsITSMFT/TopologyDictionary.h>
#include "ReconstructionDataFormats/BaseCluster.h"
#include "MFTBase/GeometryTGeo.h"
#include <CommonConstants/LHCConstants.h>

// Quality Control
#include "QualityControl/TaskInterface.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::mft
{

/// \brief MFT Cluster QC task
///
class QcMFTClusterTask /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  QcMFTClusterTask();
  /// Destructor
  ~QcMFTClusterTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

  double orbitToSeconds(uint32_t orbit, uint32_t refOrbit)
  {
    return (orbit - refOrbit) * o2::constants::lhc::LHCOrbitNS / 1E9;
  }

 private:
  std::unique_ptr<TH1F> mClusterLayerIndexH0 = nullptr;
  std::unique_ptr<TH1F> mClusterLayerIndexH1 = nullptr;
  std::unique_ptr<TH1F> mClusterDiskIndex = nullptr;

  std::unique_ptr<TH1F> mClusterOccupancy = nullptr;
  std::unique_ptr<TH1F> mClusterPatternIndex = nullptr;
  std::unique_ptr<TH1F> mClusterSizeSummary = nullptr;
  std::unique_ptr<TH1F> mGroupedClusterSizeSummary = nullptr;
  std::unique_ptr<TH2F> mClusterOccupancySummary = nullptr;

  std::unique_ptr<TH2F> mClusterPatternSensorIndices = nullptr;
  std::vector<std::unique_ptr<TH2F>> mClusterChipOccupancyMap;

  std::unique_ptr<TH1F> mClusterZ = nullptr;
  std::vector<std::unique_ptr<TH2F>> mClusterXYinLayer;
  std::vector<std::unique_ptr<TH1F>> mClusterRinLayer;

  std::unique_ptr<TH1F> mClustersROFSize = nullptr;
  std::unique_ptr<TH1F> mNOfClustersTime = nullptr;
  std::unique_ptr<TH1F> mClustersBC = nullptr;

  std::vector<o2::BaseCluster<float>> mClustersGlobal;

  // needed to construct the name and path of some histograms
  int mHalf[936] = { 0 };
  int mDisk[936] = { 0 };
  int mFace[936] = { 0 };
  int mZone[936] = { 0 };
  int mSensor[936] = { 0 };
  int mTransID[936] = { 0 };
  int mLadder[936] = { 0 };
  float mX[936] = { 0 };
  float mY[936] = { 0 };

  // internal functions
  void getChipMapData();

  // cluster size in pixels
  int mClusterSize = { 0 };

  // dictionary
  const o2::itsmft::TopologyDictionary* mDict = nullptr;

  // where the geometry file is stored
  std::string mGeomPath;

  // reference orbit used in relative time calculation
  uint32_t mRefOrbit = -1;
};

} // namespace o2::quality_control_modules::mft

#endif // QC_MFT_CLUSTER_TASK_H
