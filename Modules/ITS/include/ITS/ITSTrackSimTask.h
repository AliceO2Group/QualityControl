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
/// \file   ITSTrackSimTask.h
/// \author Artem Isakov
///

#ifndef QC_MODULE_ITS_ITSTRACKSIMTASK_H
#define QC_MODULE_ITS_ITSTRACKSIMTASK_H

#include "QualityControl/TaskInterface.h"
#include <TH1D.h>
#include <TH2D.h>
#include <DataFormatsITSMFT/TopologyDictionary.h>
#include <ITSBase/GeometryTGeo.h>

#include "SimulationDataFormat/MCTrack.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include "Field/MagneticField.h"
#include "TGeoGlobalMagField.h"
#include "DataFormatsITS/TrackITS.h"
#include <TEfficiency.h>
class TH1D;
class TH2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::its
{

class ITSTrackSimTask : public TaskInterface
{

 public:
  ITSTrackSimTask();
  ~ITSTrackSimTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

  struct InfoStruct {
    unsigned short clusters = 0;
    bool isFilled = 0;
    int isReco = 0;
    bool isPrimary = 0;
    float r;
    float pt;
    float eta;
    float phi;
    float z;
  };

 private:
  void publishHistos();
  void formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset = 1., float yOffset = 1.);
  void addObject(TObject* aObject);
  void createAllHistos();

  std::vector<std::vector<InfoStruct>> info;
  std::vector<TObject*> mPublishedObjects;

  TH1D* hNumRecoValid_pt;
  TH1D* hNumRecoFake_pt;
  TH1D* hDenTrue_pt;
  TEfficiency *hEfficiency_pt, *hEfficiency_phi, *hEfficiency_eta, *hEfficiency_z, *hEfficiency_r;
  TEfficiency *hFakeTrack_pt, *hFakeTrack_phi, *hFakeTrack_eta, *hFakeTrack_z, *hFakeTrack_r, *hFakeTrack_QoverPt;

  TH1D* hNumRecoValid_eta;
  TH1D* hNumRecoFake_eta;
  TH1D* hDenTrue_eta;

  TH1D* hNumRecoValid_phi;
  TH1D* hNumRecoFake_phi;
  TH1D* hDenTrue_phi;

  TH1D* hNumRecoValid_r;
  TH1D* hNumRecoFake_r;
  TH1D* hDenTrue_r;

  TH1D* hNumRecoValid_z;
  TH1D* hNumRecoFake_z;
  TH1D* hDenTrue_z;

  TH1D* hNumRecoFake_QoverPt;
  TH1D* hDenTrue_QoverPt;

  TH1F* hTrackImpactTransvFake;
  TH1F* hTrackImpactTransvValid;

  TH1D* hPrimaryReco_pt;
  TH1D* hPrimaryGen_pt;

  TH2D* hAngularDistribution;

  TEfficiency *hDuplicate_pt, *hDuplicate_phi, *hDuplicate_eta, *hDuplicate_z, *hDuplicate_r;
  TH1D *hNumDuplicate_pt, *hNumDuplicate_phi, *hNumDuplicate_eta, *hNumDuplicate_z, *hNumDuplicate_r;

  TEfficiency *hFakeTrack_4Cluster_pt, *hFakeTrack_4Cluster_phi, *hFakeTrack_4Cluster_eta, *hFakeTrack_4Cluster_z, *hFakeTrack_4Cluster_r, *hFakeTrack_4Cluster_QoverPt;
  TEfficiency *hFakeTrack_5Cluster_pt, *hFakeTrack_5Cluster_phi, *hFakeTrack_5Cluster_eta, *hFakeTrack_5Cluster_z, *hFakeTrack_5Cluster_r, *hFakeTrack_5Cluster_QoverPt;
  TEfficiency *hFakeTrack_6Cluster_pt, *hFakeTrack_6Cluster_phi, *hFakeTrack_6Cluster_eta, *hFakeTrack_6Cluster_z, *hFakeTrack_6Cluster_r, *hFakeTrack_6Cluster_QoverPt;
  TEfficiency *hFakeTrack_7Cluster_pt, *hFakeTrack_7Cluster_phi, *hFakeTrack_7Cluster_eta, *hFakeTrack_7Cluster_z, *hFakeTrack_7Cluster_r, *hFakeTrack_7Cluster_QoverPt;

  TH1D *hNumRecoFake_4Cluster_pt, *hNumRecoFake_4Cluster_phi, *hNumRecoFake_4Cluster_eta, *hNumRecoFake_4Cluster_z, *hNumRecoFake_4Cluster_r, *hNumRecoFake_4Cluster_QoverPt;
  TH1D *hNumRecoFake_5Cluster_pt, *hNumRecoFake_5Cluster_phi, *hNumRecoFake_5Cluster_eta, *hNumRecoFake_5Cluster_z, *hNumRecoFake_5Cluster_r, *hNumRecoFake_5Cluster_QoverPt;
  TH1D *hNumRecoFake_6Cluster_pt, *hNumRecoFake_6Cluster_phi, *hNumRecoFake_6Cluster_eta, *hNumRecoFake_6Cluster_z, *hNumRecoFake_6Cluster_r, *hNumRecoFake_6Cluster_QoverPt;
  TH1D *hNumRecoFake_7Cluster_pt, *hNumRecoFake_7Cluster_phi, *hNumRecoFake_7Cluster_eta, *hNumRecoFake_7Cluster_z, *hNumRecoFake_7Cluster_r, *hNumRecoFake_7Cluster_QoverPt;

  TH1D *hDenTrue_4Cluster_pt, *hDenTrue_4Cluster_phi, *hDenTrue_4Cluster_eta, *hDenTrue_4Cluster_z, *hDenTrue_4Cluster_r, *hDenTrue_4Cluster_QoverPt;
  TH1D *hDenTrue_5Cluster_pt, *hDenTrue_5Cluster_phi, *hDenTrue_5Cluster_eta, *hDenTrue_5Cluster_z, *hDenTrue_5Cluster_r, *hDenTrue_5Cluster_QoverPt;
  TH1D *hDenTrue_6Cluster_pt, *hDenTrue_6Cluster_phi, *hDenTrue_6Cluster_eta, *hDenTrue_6Cluster_z, *hDenTrue_6Cluster_r, *hDenTrue_6Cluster_QoverPt;
  TH1D *hDenTrue_7Cluster_pt, *hDenTrue_7Cluster_phi, *hDenTrue_7Cluster_eta, *hDenTrue_7Cluster_z, *hDenTrue_7Cluster_r, *hDenTrue_7Cluster_QoverPt;

  int mRunNumber = 0;
  std::string mO2GrpPath;
  std::string mCollisionsContextPath;

  o2::its::GeometryTGeo* mGeom;

  float bz;
};
} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSTRACKSIMTASK_H
