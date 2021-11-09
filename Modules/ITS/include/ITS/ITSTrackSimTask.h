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

  struct DataFrames {
    void update(int frame, long index)
    {
      if (frame < firstFrame) {
        firstFrame = frame;
        firstIndex = index;
      }
      if (frame > lastFrame) {
        lastFrame = frame;
        lastIndex = index;
      }
      if (frame == firstFrame && index < firstIndex) {
        firstIndex = index;
      }
      if (frame == lastFrame && index > lastIndex) {
        lastIndex = index;
      }
    }

    long firstFrame = 10000;
    int firstIndex = 1;
    long lastFrame = -10000;
    int lastIndex = -1;
  };



  static constexpr int NLayer = 7;
  static constexpr int NLayerIB = 3;

  std::vector<TObject*> mPublishedObjects;
  TH1D* hNClusters;
  TH1D* hTrackEta;
  TH1D* hTrackPhi;
  TH1D* hTrackPt;
  TH1D* hOccupancyROF;
  TH1F* hTrackImpactTransv;
  TH1D* hClusterUsage;
  TH2D* hAngularDistribution;

  TH1D*  hNumRecoValid_pt;
  TH1D* hNumRecoFake_pt;
  TH1D* hDenTrue_pt;
  TH1D* hFakeTrack_pt;
  TH1D* hEfficiency_pt;

  TH1D*  hNumRecoValid_eta;
  TH1D* hNumRecoFake_eta;
  TH1D* hDenTrue_eta;
  TH1D* hFakeTrack_eta;
  TH1D* hEfficiency_eta;

  TH1D*  hNumRecoValid_phi;
  TH1D* hNumRecoFake_phi;
  TH1D* hDenTrue_phi;
  TH1D* hFakeTrack_phi;
  TH1D* hEfficiency_phi;

  TH1D*  hNumRecoValid_r;
  TH1D* hNumRecoFake_r;
  TH1D* hDenTrue_r;
  TH1D* hFakeTrack_r;
  TH1D* hEfficiency_r;

  TH1D*  hNumRecoValid_z;
  TH1D* hNumRecoFake_z;
  TH1D* hDenTrue_z;
  TH1D* hFakeTrack_z;
  TH1D* hEfficiency_z;

  

  TH1F* hTrackImpactTransvFake;
  TH1F* hTrackImpactTransvValid;

  std::string mRunNumber;
  std::string mRunNumberPath;

  Int_t mNTracks = 0;
  Int_t mNRofs = 0;

  const int NROFOCCUPANCY = 100;
  Int_t mNClustersInTracks = 0;
  Int_t mNClusters = 0;

  o2::itsmft::TopologyDictionary mDict;
  float bz;
};
} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSTRACKTASK_H
