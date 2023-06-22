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
/// \file   ITSTrackTask.h
/// \author Artem Isakov
///

#ifndef QC_MODULE_ITS_ITSTRACKTASK_H
#define QC_MODULE_ITS_ITSTRACKTASK_H

#include "QualityControl/TaskInterface.h"
#include <TH1D.h>
#include <TH2D.h>
#include <DataFormatsITSMFT/TopologyDictionary.h>
#include <ITSBase/GeometryTGeo.h>
#include <Framework/TimingInfo.h>
#include <TTree.h>
#include <TLine.h>
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"

class TH1D;
class TH2D;

using namespace o2::quality_control::core;
using namespace o2::quality_control_modules::common;

namespace o2::quality_control_modules::its
{

class ITSTrackTask : public TaskInterface
{

 public:
  ITSTrackTask();
  ~ITSTrackTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  void publishHistos();
  void formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset = 1., float yOffset = 1.);
  void addObject(TObject* aObject);
  void createAllHistos();
  template <class T>
  void formatAxes(T* obj, const char* xTitle, const char* yTitle, float xOffset, float yOffset)
  {
    obj->GetXaxis()->SetTitle(xTitle);
    obj->GetYaxis()->SetTitle(yTitle);
    obj->GetXaxis()->SetTitleOffset(xOffset);
    obj->GetYaxis()->SetTitleOffset(yOffset);
  }

  static constexpr int NLayer = 7;
  static constexpr int NLayerIB = 3;

  const int ChipBoundary[NLayer + 1] = { 0, 108, 252, 432, 3120, 6480, 14712, 24120 };

  std::vector<TObject*> mPublishedObjects;
  TH1D* hNClusters;
  TH1D* hNClustersReset;
  std::unique_ptr<TH1DRatio> hTrackEta;
  std::unique_ptr<TH1DRatio> hTrackPhi;
  TH1D* hVerticesRof;
  std::unique_ptr<TH2DRatio> hAngularDistribution;
  TH2D* hVertexCoordinates;
  TH2D* hVertexRvsZ;
  TH1D* hVertexZ;
  TH1D* hVertexContributors;
  TH1D* hAssociatedClusterFraction;
  TH1D* hNtracks;
  TH1D* hNtracksReset;
  std::unique_ptr<TH2DRatio> hNClustersPerTrackEta;
  std::unique_ptr<TH2DRatio> hNClustersPerTrackPhi;
  std::unique_ptr<TH2DRatio> hHitFirstLayerPhiAll;
  std::unique_ptr<TH2DRatio> hHitFirstLayerPhi4cls;
  std::unique_ptr<TH2DRatio> hHitFirstLayerPhi5cls;
  std::unique_ptr<TH2DRatio> hHitFirstLayerPhi6cls;
  std::unique_ptr<TH2DRatio> hHitFirstLayerPhi7cls;
  TH2D* hClusterVsBunchCrossing;
  TH2D* hNClusterVsChipITS;

  float mVertexXYsize = 0.5;
  float mVertexZsize = 15.;
  float mVertexRsize = 0.8;
  Int_t mNtracksMAX = 100;
  Int_t mDoTTree = 0;
  // mDoNorm: 0 = no normalization, 1 = normalization by nVertices, 2 = normalization by nRofs
  Int_t mDoNorm = 1;
  Int_t mNRofs = 0;
  bool isNewCycle = true;
  int nBCbins = 103;
  long int mTimestamp = -1;
  int nVertices = 0;
  double mChipBins[2125]; // x bins for cos(lambda) plot
  double mCoslBins[25];   // y bins for cos(lambda) plot

  TTree* tClusterMap;
  //  Int_t mNtracksInROF;
  std::vector<UInt_t> vMap;
  std::vector<Float_t> vPhi;
  std::vector<Float_t> vEta;

  o2::itsmft::TopologyDictionary* mDict;
};
} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSTRACKTASK_H
