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


  TEfficiency *hEfficiency_pt, *hEfficiency_phi, *hEfficiency_eta, *hEfficiency_z, *hEfficiency_r;
  TEfficiency *hFakeTrack_pt[5], *hFakeTrack_phi[5], *hFakeTrack_eta[5], *hFakeTrack_z[5], *hFakeTrack_r[5], *hFakeTrack_QoverPt[5];  //idxs 0 to 3 -> 4 to 7 cluster tracks, idx 4 - total
  TH1D *hNumRecoFake_pt[5], *hNumRecoFake_phi[5], *hNumRecoFake_eta[5], *hNumRecoFake_z[5], *hNumRecoFake_r[5], *hNumRecoFake_QoverPt[5];
  TH1D *hDenTrue_pt[5], *hDenTrue_phi[5], *hDenTrue_eta[5], *hDenTrue_z[5], *hDenTrue_r[5], *hDenTrue_QoverPt[5];
  
  TH1D* hNumRecoValid_pt;
  TH1D* hNumRecoValid_eta;
  TH1D* hNumRecoValid_phi;
  TH1D* hNumRecoValid_r;
  TH1D* hNumRecoValid_z;

  TH1F* hTrackImpactTransvFake;
  TH1F* hTrackImpactTransvValid;

  TH1D* hPrimaryReco_pt;
  TH1D* hPrimaryGen_pt;

  TH2D* hAngularDistribution;

  TEfficiency *hDuplicate_pt, *hDuplicate_phi, *hDuplicate_eta, *hDuplicate_z, *hDuplicate_r;
  TH1D *hNumDuplicate_pt, *hNumDuplicate_phi, *hNumDuplicate_eta, *hNumDuplicate_z, *hNumDuplicate_r;

  int mRunNumber = 0;
  std::string mO2GrpPath;
  std::string mCollisionsContextPath;

  o2::its::GeometryTGeo* mGeom;

  float bz;
};
} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSTRACKSIMTASK_H
