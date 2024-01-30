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
/// \file   QcMFTTrackMCTask.cxx
/// \author Diana Krupova
///         Sara Haidlova
/// MC simulation tool inspired by ITS
//
///

#include "QualityControl/QcInfoLogger.h"
#include "MFT/QcMFTTrackMCTask.h"
#include "DataFormatsMFT/TrackMFT.h"
#include <DataFormatsITSMFT/ROFRecord.h>
#include <Framework/InputRecord.h>

#include "SimulationDataFormat/MCTrack.h"
#include "SimulationDataFormat/MCCompLabel.h"

#include "DataFormatsITSMFT/CompCluster.h"
#include "Steer/MCKinematicsReader.h" //ADDED FOR MC FILE READING

#include <typeinfo>

#include "GPUCommonDef.h"
#include "ReconstructionDataFormats/Track.h"
#include "CommonDataFormat/RangeReference.h"
#include "CommonConstants/MathConstants.h"
#include "SimulationDataFormat/MCTruthContainer.h"
#include "SimulationDataFormat/ConstMCTruthContainer.h"
#include "TFile.h"
#include "TTree.h"
#include "TMath.h"
#include <fstream>
using namespace o2::constants::math;
using namespace o2::itsmft;

namespace o2::quality_control_modules::mft
{

QcMFTTrackMCTask::~QcMFTTrackMCTask()
{
  /*
    not needed for unique pointers
  */
}

void QcMFTTrackMCTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize QcMFTTrackMCTask" << ENDM;

  mCollisionsContextPath = mCustomParameters["collisionsContextPath"];

  ILOG(Debug, Devel) << "Creating all the histograms!" << ENDM;
  const Int_t nb = 100;
  Double_t xbins[nb + 1], ptcutl = 0.01, ptcuth = 10.;
  Double_t a = TMath::Log(ptcuth / ptcutl) / nb;
  for (Int_t i = 0; i <= nb; i++)
    xbins[i] = ptcutl * TMath::Exp(i * a);

  hEfficiency_pt = std::make_unique<TEfficiency>("efficiency_pt", "#it{p}_{T} efficiency of tracking; #it{p}_{T} (GeV/#it{c}); {p}_{T}Efficiency", nb, xbins);
  getObjectsManager()->startPublishing(hEfficiency_pt.get());
  hFakeTrack_pt = std::make_unique<TEfficiency>("faketrack_pt", "#it{p}_{T} fake-track rate;#it{p}_{T} (GeV/#it{c});Fake-track rate", nb, xbins);
  getObjectsManager()->startPublishing(hFakeTrack_pt.get());
  hNumRecoValid_pt = std::make_unique<TH1D>("NumRecoValid_pt", "", nb, xbins);
  hNumRecoFake_pt = std::make_unique<TH1D>("NumRecoFake_pt", "", nb, xbins);
  hDenTrue_pt = std::make_unique<TH1D>("DenTrueMC_pt", "", nb, xbins);

  hEfficiency_phi = std::make_unique<TEfficiency>("efficiency_phi", "#phi efficiency of tracking;#phi;Efficiency", 60, 0, TMath::TwoPi());
  getObjectsManager()->startPublishing(hEfficiency_phi.get());
  hFakeTrack_phi = std::make_unique<TEfficiency>("faketrack_phi", "#phi fake-track rate;#phi;Fake-track rate", 60, 0, TMath::TwoPi());
  getObjectsManager()->startPublishing(hFakeTrack_phi.get());

  hNumRecoValid_phi = std::make_unique<TH1D>("NumRecoValid_phi", "", 60, 0, TMath::TwoPi());
  hNumRecoFake_phi = std::make_unique<TH1D>("NumRecoFake_phi", "", 60, 0, TMath::TwoPi());
  hDenTrue_phi = std::make_unique<TH1D>("DenTrueMC_phi", "", 60, 0, TMath::TwoPi());

  hEfficiency_eta = std::make_unique<TEfficiency>("efficiency_eta", "#eta efficiency of tracking;#eta;Efficiency", 30, -4, -2);
  getObjectsManager()->startPublishing(hEfficiency_eta.get());
  hFakeTrack_eta = std::make_unique<TEfficiency>("faketrack_eta", "#eta fake-track rate;#eta;Fake-track rate", 30, -4, -2);
  getObjectsManager()->startPublishing(hFakeTrack_eta.get());
  hNumRecoValid_eta = std::make_unique<TH1D>("NumRecoValid_eta", "", 30, -4, -2);
  hNumRecoFake_eta = std::make_unique<TH1D>("NumRecoFake_eta", "", 30, -4, -2);
  hDenTrue_eta = std::make_unique<TH1D>("DenTrueMC_eta", "", 30, -4, -2);

  hPrimaryGen_pt = std::make_unique<TH1D>("PrimaryGen_pt", ";#it{p}_{T} (GeV/#it{c});counts", nb, xbins);
  hPrimaryGen_pt->SetTitle("#it{p}_{T} of primary generated particles");
  getObjectsManager()->startPublishing(hPrimaryGen_pt.get());
  hPrimaryGen_pt->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
  hPrimaryGen_pt->GetYaxis()->SetTitle("counts");
  hPrimaryGen_pt->GetXaxis()->SetTitleOffset(1);
  hPrimaryGen_pt->GetYaxis()->SetTitleOffset(1.10);

  hPrimaryReco_pt = std::make_unique<TH1D>("PrimaryReco_pt", ";#it{p}_{T} (GeV/#it{c});counts", nb, xbins);
  hPrimaryReco_pt->SetTitle("#it{p}_{T} of primary reconstructed particles");
  getObjectsManager()->startPublishing(hPrimaryReco_pt.get());
  hPrimaryReco_pt->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
  hPrimaryReco_pt->GetYaxis()->SetTitle("counts");
  hPrimaryReco_pt->GetXaxis()->SetTitleOffset(1);
  hPrimaryReco_pt->GetYaxis()->SetTitleOffset(1.10);

  hResolution_pt = std::make_unique<TH1D>("Resolution_pt", ";(#it{p}_{T}^{reco}-#it{p}_{T}^{gen})/#it{p}_{T}^{gen};counts", 100, -1, 1);
  hResolution_pt->SetTitle("Resolution of #it{p}_{T} of primary particles");
  getObjectsManager()->startPublishing(hResolution_pt.get());
  hResolution_pt->GetXaxis()->SetTitle("(#it{p}_{T}^{reco}-#it{p}_{T}^{gen})/#it{p}_{T}^{gen}");
  hResolution_pt->GetYaxis()->SetTitle("counts");
  hResolution_pt->GetXaxis()->SetTitleOffset(1);
  hResolution_pt->GetYaxis()->SetTitleOffset(1.10);
}

void QcMFTTrackMCTask::startOfActivity(const Activity& activity)
{
  mRunNumber = activity.mId;
  ILOG(Debug, Devel) << "startOfActivity" << activity.mId << ENDM;
  // reset histograms
  reset();
}

void QcMFTTrackMCTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void QcMFTTrackMCTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  ILOG(Debug, Devel) << "START DOING QC General" << ENDM;
  o2::steer::MCKinematicsReader reader(mCollisionsContextPath.c_str());
  info.resize(reader.getNEvents(0));
  for (int i = 0; i < reader.getNEvents(0); ++i) {
    std::vector<MCTrack> const& mcArr = reader.getTracks(i);
    info[i].resize(mcArr.size());
  }

  for (int i = 0; i < reader.getNEvents(0); ++i) {
    std::vector<MCTrack> const& mcArr = reader.getTracks(i);
    auto mcHeader = reader.getMCEventHeader(0, i);
    for (int mc = 0; mc < mcArr.size(); mc++) {
      const auto& mcTrack = (mcArr)[mc];
      info[i][mc].isFilled = false;
      info[i][mc].isFilled = true;
      info[i][mc].pt = mcTrack.GetPt();
      info[i][mc].eta = mcTrack.GetEta();
      info[i][mc].phi = mcTrack.GetPhi();
      info[i][mc].isPrimary = mcTrack.isPrimary();
      if (mcTrack.isPrimary()) {
        hPrimaryGen_pt->Fill(mcTrack.GetPt());
      }
      hDenTrue_pt->Fill(mcTrack.GetPt());
      hDenTrue_eta->Fill(mcTrack.GetEta());
      hDenTrue_phi->Fill(mcTrack.GetPhi());
    }
  }

  auto trackArr = ctx.inputs().get<gsl::span<o2::mft::TrackMFT>>("tracks"); // MFT Tracks
  auto MCTruth = ctx.inputs().get<gsl::span<o2::MCCompLabel>>("mctruth");   // MC track label, contains info about EventID, TrackID, SourceID etc

  for (int itrack = 0; itrack < trackArr.size(); itrack++) {
    const auto& track = trackArr[itrack];
    const auto& MCinfo = MCTruth[itrack];
    if (MCinfo.isNoise())
      continue;

    if (info[MCinfo.getEventID()][MCinfo.getTrackID()].isFilled) {
      info[MCinfo.getEventID()][MCinfo.getTrackID()].isReco++;
      if (MCinfo.isFake()) {
        hNumRecoFake_pt->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].pt);
        hNumRecoFake_phi->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].phi);
        hNumRecoFake_eta->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].eta);
        if (info[MCinfo.getEventID()][MCinfo.getTrackID()].isPrimary) {
          hPrimaryReco_pt->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].pt);
        }
      } else {
        hNumRecoValid_pt->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].pt);
        hNumRecoValid_phi->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].phi);
        hNumRecoValid_eta->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].eta);
        if (info[MCinfo.getEventID()][MCinfo.getTrackID()].isPrimary) {
          hPrimaryReco_pt->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].pt);
          hResolution_pt->Fill((track.getPt() - info[MCinfo.getEventID()][MCinfo.getTrackID()].pt) / info[MCinfo.getEventID()][MCinfo.getTrackID()].pt);
        }
      }
    }
  }

  hEfficiency_pt->SetPassedHistogram(*hNumRecoValid_pt, "f");
  hEfficiency_pt->SetTotalHistogram(*hDenTrue_pt, "f");
  hFakeTrack_pt->SetPassedHistogram(*hNumRecoFake_pt, "f");
  hFakeTrack_pt->SetTotalHistogram(*hDenTrue_pt, "f");

  hFakeTrack_phi->SetPassedHistogram(*hNumRecoFake_phi, "f");
  hFakeTrack_phi->SetTotalHistogram(*hDenTrue_phi, "f");
  hEfficiency_phi->SetPassedHistogram(*hNumRecoValid_phi, "f");
  hEfficiency_phi->SetTotalHistogram(*hDenTrue_phi, "f");

  hFakeTrack_eta->SetPassedHistogram(*hNumRecoFake_eta, "f");
  hFakeTrack_eta->SetTotalHistogram(*hDenTrue_eta, "f");
  hEfficiency_eta->SetPassedHistogram(*hNumRecoValid_eta, "f");
  hEfficiency_eta->SetTotalHistogram(*hDenTrue_eta, "f");

  ILOG(Debug, Devel) << "END DOING QC General" << ENDM;
}

void QcMFTTrackMCTask::endOfCycle()
{
  getObjectsManager()->addMetadata(hEfficiency_pt->GetName(), "Run", std::to_string(mRunNumber));
  getObjectsManager()->addMetadata(hFakeTrack_pt->GetName(), "Run", std::to_string(mRunNumber));
  getObjectsManager()->addMetadata(hEfficiency_phi->GetName(), "Run", std::to_string(mRunNumber));
  getObjectsManager()->addMetadata(hFakeTrack_phi->GetName(), "Run", std::to_string(mRunNumber));
  getObjectsManager()->addMetadata(hEfficiency_eta->GetName(), "Run", std::to_string(mRunNumber));
  getObjectsManager()->addMetadata(hFakeTrack_eta->GetName(), "Run", std::to_string(mRunNumber));
  getObjectsManager()->addMetadata(hPrimaryGen_pt->GetName(), "Run", std::to_string(mRunNumber));
  getObjectsManager()->addMetadata(hPrimaryReco_pt->GetName(), "Run", std::to_string(mRunNumber));
  getObjectsManager()->addMetadata(hResolution_pt->GetName(), "Run", std::to_string(mRunNumber));
  ILOG(Debug, Devel) << "All objects were updated" << ENDM;
}

void QcMFTTrackMCTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void QcMFTTrackMCTask::reset()
{
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  hNumRecoValid_pt->Reset();
  hNumRecoFake_pt->Reset();
  hDenTrue_pt->Reset();

  hNumRecoValid_phi->Reset();
  hNumRecoFake_phi->Reset();
  hDenTrue_phi->Reset();

  hNumRecoValid_eta->Reset();
  hNumRecoFake_eta->Reset();
  hDenTrue_eta->Reset();

  hPrimaryGen_pt->Reset();
  hPrimaryReco_pt->Reset();
  hResolution_pt->Reset();
}

} // namespace o2::quality_control_modules::mft
