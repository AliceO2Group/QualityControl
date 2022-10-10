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
#include <TString.h>
#include <THnSparse.h>
#include <string>

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
  void updateOccMonitorPlots();

  static constexpr int NLayer = 7;
  static constexpr int NLayerIB = 3;
  TH2D* hClusterVsBunchCrossing;
  std::vector<TObject*> mPublishedObjects;
  TH1D* hClusterSizeSummaryIB[7][48][9];
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

  TH1D* hGroupedClusterSizeSummaryOB[7][48];
  TH1D* hClusterSizeSummaryOB[7][48];
  TH1D* hClusterTopologySummaryOB[7][48];

  TH2D* hAverageClusterOccupancySummaryOB[7];
  TH2D* hAverageClusterOccupancyMonitorOB[7]; // will be used in online data monitoring, showing occupation for the last N ROFs
  TH2D* hAverageClusterSizeSummaryOB[7];
  TH2D* hAverageClusterSizeMonitorOB[7];

  //  THnSparseD *sClustersSize[7];
  TH2D* mGeneralOccupancy;

  const int mOccUpdateFrequency = 100000;
  int mDoPublish1DSummary = 0;
  int mNThreads = 1;
  int mNRofs = 0;
  int mNRofsMonitor = 0;
  int nBCbins;
  long int mTimestamp;
  std::string mGeomPath;
  TString xLabel;

  const int mNStaves[7] = { 12, 16, 20, 24, 30, 42, 48 };
  const int mNHicPerStave[NLayer] = { 1, 1, 1, 8, 8, 14, 14 };
  const int mNChipsPerHic[NLayer] = { 9, 9, 9, 14, 14, 14, 14 };
  const int mNLanePerHic[NLayer] = { 3, 3, 3, 2, 2, 2, 2 };
  const int ChipBoundary[NLayer + 1] = { 0, 108, 252, 432, 3120, 6480, 14712, 24120 };
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  const std::string mYlabels[NLayer * 2] = { "L6B(S24#rightarrow47)", "L5B(S21#rightarrow41)", "L4B(S15#rightarrow29)", "L3B(S12#rightarrow23)", "L2B(S10#rightarrow19)", "L1B(S08#rightarrow15)", "L0B(S06#rightarrow11)", "L0T(S00#rightarrow05)", "L1T(S00#rightarrow07)", "L2T(S00#rightarrow09)", "L3T(S00#rightarrow11)", "L4T(S00#rightarrow14)", "L5T(S00#rightarrow20)", "L6T(S00#rightarrow23)" };

  int mEnableLayers[7];
  int mClusterSize[7][48][28] = { { { 0 } } }; //[#layers][max staves][max lanes / chips]
  double mClusterSizeMonitor[7][48][28] = { { { 0 } } };
  int nClusters[7][48][28] = { { { 0 } } };
  Int_t mClusterOccupancyOB[7][48][28] = { { { 0 } } };
  Int_t mClusterOccupancyOBmonitor[7][48][28] = { { { 0 } } };

  o2::itsmft::TopologyDictionary* mDict;
  o2::its::GeometryTGeo* mGeom;

  const char* OBLabel34[16] = { "HIC1L_B0_ln7", "HIC1L_A8_ln6", "HIC2L_B0_ln8", "HIC2L_A8_ln5", "HIC3L_B0_ln9", "HIC3L_A8_ln4", "HIC4L_B0_ln10", "HIC4L_A8_ln3", "HIC1U_B0_ln21", "HIC1U_A8_ln20", "HIC2U_B0_ln22", "HIC2U_A8_ln19", "HIC3U_B0_ln23", "HIC3U_A8_ln18", "HIC4U_B0_ln24", "HIC4U_A8_ln17" };
  const char* OBLabel56[28] = { "HIC1L_B0_ln7", "HIC1L_A8_ln6", "HIC2L_B0_ln8", "HIC2L_A8_ln5", "HIC3L_B0_ln9", "HIC3L_A8_ln4", "HIC4L_B0_ln10", "HIC4L_A8_ln3", "HIC5L_B0_ln11", "HIC5L_A8_ln2", "HIC6L_B0_ln12", "HIC6L_A8_ln1", "HIC7L_B0_ln13", "HIC7L_A8_ln0", "HIC1U_B0_ln21", "HIC1U_A8_ln20", "HIC2U_B0_ln22", "HIC2U_A8_ln19", "HIC3U_B0_ln23", "HIC3U_A8_ln18", "HIC4U_B0_ln24", "HIC4U_A8_ln17", "HIC5U_B0_ln25", "HIC5U_A8_ln16", "HIC6U_B0_ln26", "HIC6U_A8_ln15", "HIC7U_B0_ln27", "HIC7U_A8_ln14" };
};
} //  namespace o2::quality_control_modules::its

#endif
