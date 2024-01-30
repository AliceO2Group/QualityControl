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
/// \file   QcMFTTrackMCTask.h
/// \author Diana Krupova
///         Sara Haidlova
///         inspired by ITS sim task

#ifndef QC_MFT_TRACK_MC_TASK_H
#define QC_MFT_TRACK_MC_TASK_H

#include "QualityControl/TaskInterface.h"
#include <TH1D.h>
#include <TH2D.h>
#include <DataFormatsITSMFT/TopologyDictionary.h>

#include "SimulationDataFormat/MCTrack.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include "DataFormatsMFT/TrackMFT.h"
#include <TEfficiency.h>
class TH1D;
class TH2D;

using namespace o2::quality_control::core;
using namespace std;

namespace o2::quality_control_modules::mft
{

class QcMFTTrackMCTask : public TaskInterface
{

 public:
  QcMFTTrackMCTask() = default;
  ~QcMFTTrackMCTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

  struct InfoStruct {
    bool isFilled = 0;
    int isReco = 0;
    bool isPrimary = 0;
    float pt;
    float eta;
    float phi;
  };

 private:
  std::vector<std::vector<InfoStruct>> info;

  std::unique_ptr<TH1D> hNumRecoValid_pt = nullptr;
  std::unique_ptr<TH1D> hNumRecoFake_pt = nullptr;
  std::unique_ptr<TH1D> hDenTrue_pt = nullptr;

  std::unique_ptr<TH1D> hNumRecoValid_eta = nullptr;
  std::unique_ptr<TH1D> hNumRecoFake_eta = nullptr;
  std::unique_ptr<TH1D> hDenTrue_eta = nullptr;

  std::unique_ptr<TH1D> hNumRecoValid_phi = nullptr;
  std::unique_ptr<TH1D> hNumRecoFake_phi = nullptr;
  std::unique_ptr<TH1D> hDenTrue_phi = nullptr;

  std::unique_ptr<TEfficiency> hEfficiency_pt = nullptr;
  std::unique_ptr<TEfficiency> hEfficiency_phi = nullptr;
  std::unique_ptr<TEfficiency> hEfficiency_eta = nullptr;
  std::unique_ptr<TEfficiency> hFakeTrack_pt = nullptr;
  std::unique_ptr<TEfficiency> hFakeTrack_phi = nullptr;
  std::unique_ptr<TEfficiency> hFakeTrack_eta = nullptr;

  std::unique_ptr<TH1D> hPrimaryReco_pt = nullptr;
  std::unique_ptr<TH1D> hPrimaryGen_pt = nullptr;
  std::unique_ptr<TH1D> hResolution_pt = nullptr; // (pt_reco - pt_gen)/pt_gen

  int mRunNumber = 0;
  std::string mCollisionsContextPath;

  float bz;
};
} // namespace o2::quality_control_modules::mft

#endif // QC_MFT_TRACK_MC_TASK_H
