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
#include <TTree.h>
#include <TLine.h>

class TH1D;
class TH2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::its
{

class ITSTrackTask : public TaskInterface
{

 public:
  ITSTrackTask();
  ~ITSTrackTask() override;

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
  void createAllHistos();

  static constexpr int NLayer = 7;
  static constexpr int NLayerIB = 3;

  const int ChipBoundary[NLayer + 1] = { 0, 108, 252, 432, 3120, 6480, 14712, 24120 };

  std::vector<TObject*> mPublishedObjects;
  TH1D* hNClusters;
  TH1D* hTrackEta;
  TH1D* hTrackPhi;
  TH2D* hAngularDistribution;
  TH2D* hVertexCoordinates;
  TH2D* hVertexRvsZ;
  TH1D* hVertexZ;
  TH1D* hVertexContributors;
  TH1D* hAssociatedClusterFraction;
  TH1D* hNtracks;
  TH2D* hNClustersPerTrackEta;
  TH2D* hClusterVsBunchCrossing;
  TH2D* hNClusterVsChip[NLayer];
  TH2D* hNClusterVsChipITS;

  float mVertexXYsize;
  float mVertexZsize;
  float mVertexRsize;
  Int_t mNtracksMAX;
  Int_t mDoTTree;
  Int_t mNTracks = 0;
  Int_t mNRofs = 0;
  int nBCbins;
  long int mTimestamp;

  const int NROFOCCUPANCY = 100;
  Int_t mNClusters = 0;

  TTree* tClusterMap;
  //  Int_t mNtracksInROF;
  std::vector<UInt_t> vMap;
  std::vector<Float_t> vPhi;
  std::vector<Float_t> vEta;

  o2::itsmft::TopologyDictionary mDict;
};
} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSTRACKTASK_H
