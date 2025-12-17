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
/// \author David Grund
/// \author Jakub Juracka
///

#ifndef QC_MFT_CLUSTER_TASK_H
#define QC_MFT_CLUSTER_TASK_H

#include <TH1F.h>
#include <TH2F.h>
#include <DataFormatsITSMFT/TopologyDictionary.h>
#include "ReconstructionDataFormats/BaseCluster.h"
#include "MFTBase/GeometryTGeo.h"
#include <CommonConstants/LHCConstants.h>
#include <vector>

// Quality Control
#include "QualityControl/TaskInterface.h"
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"

using namespace o2::quality_control::core;
using namespace o2::quality_control_modules::common;

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
  std::unique_ptr<TH1FRatio> mClusterLayerIndexH0 = nullptr;
  std::unique_ptr<TH1FRatio> mClusterLayerIndexH1 = nullptr;

  std::unique_ptr<TH1FRatio> mClusterOccupancy = nullptr;
  std::unique_ptr<TH1FRatio> mClusterPatternIndex = nullptr;
  std::unique_ptr<TH1FRatio> mClusterSizeSummary = nullptr;
  std::unique_ptr<TH1FRatio> mGroupedClusterSizeSummary = nullptr;
  std::unique_ptr<TH2FRatio> mClusterOccupancySummary = nullptr;

  std::unique_ptr<TH2FRatio> mClusterPatternSensorIndices = nullptr;
  std::vector<std::unique_ptr<TH2FRatio>> mClusterChipOccupancyMap;

  std::unique_ptr<TH1FRatio> mClusterZ = nullptr;
  std::vector<std::unique_ptr<TH2FRatio>> mClusterXYinLayer;
  std::vector<std::unique_ptr<TH1FRatio>> mClusterRinLayer;

  std::unique_ptr<TH1FRatio> mClustersROFSize = nullptr;
  std::unique_ptr<TH1FRatio> mClustersBC = nullptr;

  std::vector<o2::BaseCluster<float>> mClustersGlobal;

  int mOnlineQC;

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

  std::vector<float> mROFBins = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                                 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                                 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                                 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
                                 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
                                 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
                                 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
                                 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
                                 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
                                 110, 120, 130, 140, 150, 160, 170, 180, 190, 200,
                                 210, 220, 230, 240, 250, 260, 270, 280, 290, 300,
                                 310, 320, 330, 340, 350, 360, 370, 380, 390, 400,
                                 410, 420, 430, 440, 450, 460, 470, 480, 490, 500,
                                 510, 520, 530, 540, 550, 560, 570, 580, 590, 600,
                                 610, 620, 630, 640, 650, 660, 670, 680, 690, 700,
                                 710, 720, 730, 740, 750, 760, 770, 780, 790, 800,
                                 810, 820, 830, 840, 850, 860, 870, 880, 890, 900,
                                 910, 920, 930, 940, 950, 960, 970, 980, 990, 1000,
                                 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000,
                                 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900, 3000,
                                 3100, 3200, 3300, 3400, 3500, 3600, 3700, 3800, 3900, 4000,
                                 4100, 4200, 4300, 4400, 4500, 4600, 4700, 4800, 4900, 5000,
                                 5100, 5200, 5300, 5400, 5500, 5600, 5700, 5800, 5900, 6000,
                                 6100, 6200, 6300, 6400, 6500, 6600, 6700, 6800, 6900, 7000,
                                 7100, 7200, 7300, 7400, 7500, 7600, 7700, 7800, 7900, 8000,
                                 8100, 8200, 8300, 8400, 8500, 8600, 8700, 8800, 8900, 9000,
                                 9100, 9200, 9300, 9400, 9500, 9600, 9700, 9800, 9900, 10000,
                                 11000, 12000, 13000, 14000, 15000, 16000, 17000, 18000, 19000, 20000,
                                 21000, 22000, 23000, 24000, 25000, 26000, 27000, 28000, 29000, 30000,
                                 31000, 32000, 33000, 34000, 35000, 36000, 37000, 38000, 39000, 40000,
                                 41000, 42000, 43000, 44000, 45000, 46000, 47000, 48000, 49000, 50000};

  // dictionary
  const o2::itsmft::TopologyDictionary* mDict = nullptr;

  o2::mft::GeometryTGeo* mGeom = nullptr;
  // where the geometry file is stored
  std::string mGeomPath;

  // reference orbit used in relative time calculation
  uint32_t mRefOrbit = -1;
};

} // namespace o2::quality_control_modules::mft

#endif // QC_MFT_CLUSTER_TASK_H
