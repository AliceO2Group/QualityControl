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
#include "Common/TH2Ratio.h"

#include <DataFormatsITSMFT/TopologyDictionary.h>
#include <ITSBase/GeometryTGeo.h>
#include <Framework/TimingInfo.h>

#include <TLine.h>
#include <TLatex.h>

class TH1F;
class TH2F;

using namespace o2::quality_control::core;
using namespace o2::quality_control_modules::common;

namespace o2::quality_control_modules::its
{

class ITSClusterTask : public TaskInterface
{

 public:
  ITSClusterTask();
  ~ITSClusterTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

  // Fine checks
  void setRphiBinningIB(std::vector<float> bins = { -0.75, -0.60, -0.45, -0.30, -0.15, 0, 0.15, 0.30, 0.45, 0.60, 0.76 });
  void setZBinningIB(std::vector<float> bins = { -1.5, -1.20, -0.9, -0.6, -0.3, 0, 0.3, 0.6, 0.9, 1.2, 1.51 });
  void setRphiBinningOB(std::vector<float> bins = { -0.75, -0.35, 0, 0.35, 0.76 });
  void setZBinningOB(std::vector<float> bins = { -1.5, -0.75, 0., 0.75, 1.51 });

 private:
  void publishHistos();
  template <class T>
  void formatAxes(T* obj, const char* xTitle, const char* yTitle, float xOffset, float yOffset)
  {
    obj->GetXaxis()->SetTitle(xTitle);
    obj->GetYaxis()->SetTitle(yTitle);
    obj->GetXaxis()->SetTitleOffset(xOffset);
    obj->GetYaxis()->SetTitleOffset(yOffset);
  }
  void addObject(TObject* aObject);
  void getJsonParameters();
  void createAllHistos();
  void addLines();

  float getHorizontalBin(float z, int chip, int layer, int lane = 0);
  float getVerticalBin(float rphi, int stave, int layer);

  static constexpr int NLayer = 7;
  static constexpr int NLayerIB = 3;

  std::vector<TObject*> mPublishedObjects;

  // Inner barrel
  TH1F* hClusterTopologySummaryIB[NLayer][48][9] = { { { nullptr } } };
  TH1F* hGroupedClusterSizeSummaryIB[NLayer][48][9] = { { { nullptr } } };

  std::shared_ptr<TH2FRatio> hAverageClusterOccupancySummaryIB[NLayer];
  std::shared_ptr<TH2FRatio> hAverageClusterSizeSummaryIB[NLayer];

  int mClusterOccupancyIB[NLayer][48][9] = { { { 0 } } };

  // Outer barrel
  TH1F* hGroupedClusterSizeSummaryOB[NLayer][48] = { { nullptr } };
  TH1F* hClusterSizeSummaryOB[NLayer][48] = { { nullptr } };
  TH1F* hClusterTopologySummaryOB[NLayer][48] = { { nullptr } };

  std::shared_ptr<TH2FRatio> hAverageClusterOccupancySummaryOB[NLayer];
  std::shared_ptr<TH2FRatio> hAverageClusterSizeSummaryOB[NLayer];

  int mClusterOccupancyOB[NLayer][48][28] = { { { 0 } } };
  int mNLaneEmpty[4] = { 0 }; // IB, ML, OL, TOTAL empty lane

  // Layer synnary
  TH1F* hClusterSizeLayerSummary[NLayer] = { nullptr };
  TH1F* hClusterTopologyLayerSummary[NLayer] = { nullptr };
  TH1F* hGroupedClusterSizeLayerSummary[NLayer] = { nullptr };

  // General
  TH2F* hClusterVsBunchCrossing = nullptr;
  std::unique_ptr<TH2FRatio> mGeneralOccupancy = nullptr;

  // Fine checks

  std::shared_ptr<TH2FRatio> hAverageClusterOccupancySummaryFine[NLayer];
  std::shared_ptr<TH2FRatio> hAverageClusterSizeSummaryFine[NLayer];

  std::shared_ptr<TH2FRatio> hAverageClusterOccupancySummaryZPhi[NLayer];
  std::shared_ptr<TH2FRatio> hAverageClusterSizeSummaryZPhi[NLayer];

  TH1D* hEmptyLaneFractionGlobal;

  int mClusterSize[NLayer][48][28] = { { { 0 } } }; //[#layers][max staves][max lanes / chips]
  int nClusters[NLayer][48][28] = { { { 0 } } };

  // Edges of space binning within chips (local frame coordinates)
  std::vector<float> vRphiBinsIB;
  std::vector<float> vZBinsIB;
  std::vector<float> vRphiBinsOB;
  std::vector<float> vZBinsOB;

  int nZBinsIB = 1;
  int nRphiBinsIB = 1;
  int nRphiBinsOB = 1;
  int nZBinsOB = 1;
  static constexpr int NFlags = 4;

  const int mOccUpdateFrequency = 100000;
  const int mNLanes[4] = { 432, 864, 2520, 3816 }; // IB, ML, OL, TOTAL lane
  int mDoPublish1DSummary = 0;
  int mNThreads = 1;
  int mNRofs = 0;
  int nBCbins = 103;
  long int mTimestamp = -1;
  TString xLabel;
  std::string mLaneStatusFlag[NFlags] = { "IB", "ML", "OL", "Total" };
  int mDoPublishDetailedSummary = 0;

  const int mNStaves[NLayer] = { 12, 16, 20, 24, 30, 42, 48 };
  const int mNHicPerStave[NLayer] = { 1, 1, 1, 8, 8, 14, 14 };
  const int mNChipsPerHic[NLayer] = { 9, 9, 9, 14, 14, 14, 14 };
  const int mNLanePerHic[NLayer] = { 3, 3, 3, 2, 2, 2, 2 };
  const int ChipBoundary[NLayer + 1] = { 0, 108, 252, 432, 3120, 6480, 14712, 24120 };
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  const float mLength[NLayer] = { 14., 14., 14., 43., 43., 74., 74. };
  const std::string mYlabels[NLayer * 2] = { "L6B(S24#rightarrow47)", "L5B(S21#rightarrow41)", "L4B(S15#rightarrow29)", "L3B(S12#rightarrow23)", "L2B(S10#rightarrow19)", "L1B(S08#rightarrow15)", "L0B(S06#rightarrow11)", "L0T(S00#rightarrow05)", "L1T(S00#rightarrow07)", "L2T(S00#rightarrow09)", "L3T(S00#rightarrow11)", "L4T(S00#rightarrow14)", "L5T(S00#rightarrow20)", "L6T(S00#rightarrow23)" };

  int mEnableLayers[NLayer] = { 0 };

  o2::itsmft::TopologyDictionary* mDict = nullptr;
  o2::its::GeometryTGeo* mGeom = nullptr;

  const char* OBLabel34[16] = { "HIC1L_B0_ln7", "HIC1L_A8_ln6", "HIC2L_B0_ln8", "HIC2L_A8_ln5", "HIC3L_B0_ln9", "HIC3L_A8_ln4", "HIC4L_B0_ln10", "HIC4L_A8_ln3", "HIC1U_B0_ln21", "HIC1U_A8_ln20", "HIC2U_B0_ln22", "HIC2U_A8_ln19", "HIC3U_B0_ln23", "HIC3U_A8_ln18", "HIC4U_B0_ln24", "HIC4U_A8_ln17" };
  const char* OBLabel56[28] = { "HIC1L_B0_ln7", "HIC1L_A8_ln6", "HIC2L_B0_ln8", "HIC2L_A8_ln5", "HIC3L_B0_ln9", "HIC3L_A8_ln4", "HIC4L_B0_ln10", "HIC4L_A8_ln3", "HIC5L_B0_ln11", "HIC5L_A8_ln2", "HIC6L_B0_ln12", "HIC6L_A8_ln1", "HIC7L_B0_ln13", "HIC7L_A8_ln0", "HIC1U_B0_ln21", "HIC1U_A8_ln20", "HIC2U_B0_ln22", "HIC2U_A8_ln19", "HIC3U_B0_ln23", "HIC3U_A8_ln18", "HIC4U_B0_ln24", "HIC4U_A8_ln17", "HIC5U_B0_ln25", "HIC5U_A8_ln16", "HIC6U_B0_ln26", "HIC6U_A8_ln15", "HIC7U_B0_ln27", "HIC7U_A8_ln14" };
};
} //  namespace o2::quality_control_modules::its

#endif
