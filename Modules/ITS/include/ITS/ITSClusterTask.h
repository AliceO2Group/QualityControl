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
#include <THnSparse.h>

#include <DataFormatsITSMFT/TopologyDictionary.h>
#include <ITSBase/GeometryTGeo.h>
#include <TH2Poly.h>
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
  void getStavePoint(int layer, int stave, double* px, double* py);
  void formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset = 1., float yOffset = 1.);
  void addObject(TObject* aObject);
  void getJsonParameters();
  void createAllHistos();
  void updateOccMonitorPlots();

  static constexpr int NLayer = 7;
  static constexpr int NLayerIB = 3;
  TH2D* hClusterVsBunchCrossing;
  std::vector<TObject*> mPublishedObjects;
  TH1D* hClusterSizeSummaryIB[7][48][9];
  TH1D* hClusterSizeMonitorIB[7][48][9];
  TH1D* hClusterTopologySummaryIB[7][48][9];
  TH1D* hGroupedClusterSizeSummaryIB[7][48][9];

  TH1D* hClusterSizeLayerSummary[7];
  TH1D* hClusterTopologyLayerSummary[7];
  TH1D* hGroupedClusterSizeLayerSummary[7];

  TH2D* hAverageClusterOccupancySummaryIB[7];
  TH2D* hAverageClusterOccupancyMonitorIB[7]; // will be used in online data monitoring, showing occupation for the last N ROFs
  TH2D* hAverageClusterSizeSummaryIB[7];
  TH2D* hAverageClusterSizeMonitorIB[7];

  Int_t mClusterOccupancyIB[7][48][9];
  Int_t mClusterOccupancyIBmonitor[7][48][9];

  TH1D* hClusterSizeOB[7][48][14];        // used to calculate hAverageClusterSizeSummaryIB
  TH1D* hClusterSizeMonitorOB[7][48][14]; // used to calculate hAverageClusterSizeMonitorIB

  TH1D* hGroupedClusterSizeSummaryOB[7][48];
  TH1D* hClusterSizeSummaryOB[7][48];
  TH1D* hClusterTopologySummaryOB[7][48];

  TH2D* hAverageClusterOccupancySummaryOB[7];
  TH2D* hAverageClusterOccupancyMonitorOB[7]; // will be used in online data monitoring, showing occupation for the last N ROFs
  TH2D* hAverageClusterSizeSummaryOB[7];
  TH2D* hAverageClusterSizeMonitorOB[7];

  //  THnSparseD *sClustersSize[7];
  TH2Poly* mGeneralOccupancy;
  Int_t mClusterOccupancyOB[7][48][14];
  Int_t mClusterOccupancyOBmonitor[7][48][14];

  const int mOccUpdateFrequency = 100000;
  int mNThreads = 1;
  int mNRofs = 0;
  int mNRofsMonitor = 0;
  int nBCbins;

  const int mNStaves[7] = { 12, 16, 20, 24, 30, 42, 48 };
  const int mNHicPerStave[NLayer] = { 1, 1, 1, 8, 8, 14, 14 };
  const int mNChipsPerHic[NLayer] = { 9, 9, 9, 14, 14, 14, 14 };
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  const float MidPointRad[7] = { 23.49, 31.586, 39.341, 197.598, 246.944, 345.348, 394.883 };
  const float StartAngle[7] = { 16.997 / 360 * (TMath::Pi() * 2.), 17.504 / 360 * (TMath::Pi() * 2.), 17.337 / 360 * (TMath::Pi() * 2.), 8.75 / 360 * (TMath::Pi() * 2.), 7 / 360 * (TMath::Pi() * 2.), 5.27 / 360 * (TMath::Pi() * 2.), 4.61 / 360 * (TMath::Pi() * 2.) }; // start angle of first stave in each layer
  //
  int mEnableLayers[7];
  o2::itsmft::TopologyDictionary mDict;
  o2::its::GeometryTGeo* mGeom;
};
} //  namespace o2::quality_control_modules::its

#endif
