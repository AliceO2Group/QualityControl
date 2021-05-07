// Copyright CERN and copyright holders of ALICE O2. This software is
// // distributed under the terms of the GNU General Public License v3 (GPL
// // Version 3), copied verbatim in the file "COPYING".
// //
// // See http://alice-o2.web.cern.ch/license for full licensing information.
// //
// // In applying this license CERN does not waive the privileges and immunities
// // granted to it by virtue of its status as an Intergovernmental Organization
// // or submit itself to any jurisdiction.
//

///
/// \file   ITSClusterTask.h
/// \author Artem Isakov
///

#ifndef QC_MODULE_ITS_ITSCLUSTERTASK_H
#define QC_MODULE_ITS_ITSCLUSTERTASK_H

#include "QualityControl/TaskInterface.h"
#include <TH1.h>
#include <TH2.h>
#include <DataFormatsITSMFT/TopologyDictionary.h>
#include <ITSBase/GeometryTGeo.h>

class TH1D;
class TH2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::its
{

class ITSClusterTask : public TaskInterface
{

 public:
  ITSClusterTask();
  ~ITSClusterTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  void publishHistos();
  void formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset = 1., float yOffset = 1.);
  void addObject(TObject* aObject);
  void getJsonParameters();
  void createAllHistos();

  static constexpr int NLayer = 7;
  static constexpr int NLayerIB = 3;

  std::vector<TObject*> mPublishedObjects;
  TH1D* hClusterSizeIB[7][48][9];
  TH1D* hClusterTopologyIB[7][48][9];
  TH2D* hOccupancyIB[7];
  TH2D* hAverageClusterIB[7];
  Int_t mClasterOccupancyIB[7][48][9];
  TH1D* hClusterSizeOB[7][48][14];
  TH1D* hClusterTopologyOB[7][48][14];
  TH2D* hOccupancyOB[7];
  TH2D* hAverageClusterOB[7];
  Int_t mClasterOccupancyOB[7][48][14];

  const int mOccUpdateFrequency = 100000;
  int mNThreads = 1;
  int mNRofs = 0;
  int ChipIDprev = 0;
  std::string mDictPath;
  std::string mRunNumberPath;
  std::string mGeomPath;
  std::string mRunNumber = "000000";

  const int mNStaves[7] = { 12, 16, 20, 24, 30, 42, 48 };
  const int mNHicPerStave[NLayer] = { 1, 1, 1, 8, 8, 14, 14 };
  const int mNChipsPerHic[NLayer] = { 9, 9, 9, 14, 14, 14, 14 };
  int mEnableLayers[7];
  o2::itsmft::TopologyDictionary mDict;
  o2::its::GeometryTGeo* mGeom;
};
} //  namespace o2::quality_control_modules::its

#endif
