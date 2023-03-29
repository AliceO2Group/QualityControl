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

#include "MUONCommon/TrackPlotter.h"

#include <DataFormatsMCH/TrackMCH.h>
#include <ReconstructionDataFormats/TrackMCHMID.h>
#include <ReconstructionDataFormats/GlobalFwdTrack.h>
#include <Framework/DataRefUtils.h>
#include <Framework/InputRecord.h>
#include <TH2F.h>
#include <gsl/span>
#include <set>

namespace
{

static void setXAxisLabels(TProfile* h)
{
  TAxis* axis = h->GetXaxis();
  for (int i = 1; i <= 10; i++) {
    auto label = fmt::format("CH{}", i);
    axis->SetBinLabel(i, label.c_str());
  }
}

} // namespace

using namespace o2::dataformats;

namespace o2::quality_control_modules::muon
{

TrackPlotter::TrackPlotter(int maxTracksPerTF, GID::Source source, std::string path) : mSrc(source), mPath(path)
{
  createTrackHistos(maxTracksPerTF);
  createTrackPairHistos();
}

void TrackPlotter::createTrackHistos(int maxTracksPerTF)
{
  mNofTracksPerTF[0] = createHisto<TH1F>(TString::Format("%sPositive/TracksPerTF", mPath.c_str()), "Number of tracks per TimeFrame (+);Number of tracks per TF", maxTracksPerTF, 0, maxTracksPerTF, true, "logy");
  mNofTracksPerTF[1] = createHisto<TH1F>(TString::Format("%sNegative/TracksPerTF", mPath.c_str()), "Number of tracks per TimeFrame (-);Number of tracks per TF", maxTracksPerTF, 0, maxTracksPerTF, true, "logy");
  mNofTracksPerTF[2] = createHisto<TH1F>(TString::Format("%sTracksPerTF", mPath.c_str()), "Number of tracks per TimeFrame;Number of tracks per TF", maxTracksPerTF, 0, maxTracksPerTF, true, "logy");

  mTrackDCA[0] = createHisto<TH1F>(TString::Format("%sPositive/TrackDCA", mPath.c_str()), "Track DCA (+);DCA (cm)", 500, 0, 500);
  mTrackDCA[1] = createHisto<TH1F>(TString::Format("%sNegative/TrackDCA", mPath.c_str()), "Track DCA (-);DCA (cm)", 500, 0, 500);
  mTrackDCA[2] = createHisto<TH1F>(TString::Format("%sTrackDCA", mPath.c_str()), "Track DCA;DCA (cm)", 500, 0, 500);

  mTrackPDCA[0] = createHisto<TH1F>(TString::Format("%sPositive/TrackPDCA", mPath.c_str()), "Track p#timesDCA (+);p#timesDCA (GeVcm/c)", 5000, 0, 5000);
  mTrackPDCA[1] = createHisto<TH1F>(TString::Format("%sNegative/TrackPDCA", mPath.c_str()), "Track p#timesDCA (-);p#timesDCA (GeVcm/c)", 5000, 0, 5000);
  mTrackPDCA[2] = createHisto<TH1F>(TString::Format("%sTrackPDCA", mPath.c_str()), "Track p#timesDCA;p#timesDCA (GeVcm/c)", 5000, 0, 5000);

  mTrackPt[0] = createHisto<TH1F>(TString::Format("%sPositive/TrackPt", mPath.c_str()), "Track p_{T} (+);p_{T} (GeV/c)", 300, 0, 30, false, "logy");
  mTrackPt[1] = createHisto<TH1F>(TString::Format("%sNegative/TrackPt", mPath.c_str()), "Track p_{T} (-);p_{T} (GeV/c)", 300, 0, 30, false, "logy");
  mTrackPt[2] = createHisto<TH1F>(TString::Format("%sTrackPt", mPath.c_str()), "Track p_{T};p_{T} (GeV/c)", 300, 0, 30, false, "logy");

  mTrackEta[0] = createHisto<TH1F>(TString::Format("%sPositive/TrackEta", mPath.c_str()), "Track #eta (+);#eta", 200, -4.5, -2);
  mTrackEta[1] = createHisto<TH1F>(TString::Format("%sNegative/TrackEta", mPath.c_str()), "Track #eta (-);#eta", 200, -4.5, -2);
  mTrackEta[2] = createHisto<TH1F>(TString::Format("%sTrackEta", mPath.c_str()), "Track #eta;#eta", 200, -4.5, -2);

  mTrackPhi[0] = createHisto<TH1F>(TString::Format("%sPositive/TrackPhi", mPath.c_str()), "Track #phi (+);#phi (deg)", 360, 0, 360);
  mTrackPhi[1] = createHisto<TH1F>(TString::Format("%sNegative/TrackPhi", mPath.c_str()), "Track #phi (-);#phi (deg)", 360, 0, 360);
  mTrackPhi[2] = createHisto<TH1F>(TString::Format("%sTrackPhi", mPath.c_str()), "Track #phi;#phi (deg)", 360, 0, 360);

  mTrackRAbs[0] = createHisto<TH1F>(TString::Format("%sPositive/TrackRAbs", mPath.c_str()), "Track R_{abs} (+);R_{abs} (cm)", 1000, 0, 100);
  mTrackRAbs[1] = createHisto<TH1F>(TString::Format("%sNegative/TrackRAbs", mPath.c_str()), "Track R_{abs} (-);R_{abs} (cm)", 1000, 0, 100);
  mTrackRAbs[2] = createHisto<TH1F>(TString::Format("%sTrackRAbs", mPath.c_str()), "Track R_{abs};R_{abs} (cm)", 1000, 0, 100);

  mTrackBC = createHisto<TH1F>(TString::Format("%sTrackBC", mPath.c_str()), "Track BC;BC", o2::constants::lhc::LHCMaxBunches, 0, o2::constants::lhc::LHCMaxBunches);
  mTrackBCWidth = createHisto<TH1F>(TString::Format("%sTrackBCWidth", mPath.c_str()), "Track BCWidth;BC Width", 400, 0, 400);

  if (mSrc == GID::MFTMCHMID) {
    mTrackDT = std::make_unique<TH2F>(TString::Format("%sTrackDT", mPath.c_str()), "Track Time Correlation (MID-MCH vs MFT-MCH);BC;BC", 1000, -500, 500, 1000, -500, 500);
    histograms().emplace_back(HistInfo{ mTrackDT.get(), "col", "logz" });
  } else {
    std::string titleStr;
    if (mSrc == GID::MFTMCH) {
      titleStr = "(MFT-MCH)";
    }
    if (mSrc == GID::MCHMID) {
      titleStr = "(MID-MCH)";
    }
    mTrackDT = std::make_unique<TH1F>(TString::Format("%sTrackDT", mPath.c_str()),
                                      TString::Format("Track Time Correlation %s;BC", titleStr.c_str()),
                                      2000, -1000, 1000);
    histograms().emplace_back(HistInfo{ mTrackDT.get(), "hist", "logy" });
  }
}

void TrackPlotter::createTrackPairHistos()
{
  mMinv = createHisto<TH1F>(TString::Format("%sMinv", mPath.c_str()), "#mu^{+}#mu^{-} invariant mass;M_{#mu^{+}#mu^{-}} (GeV/c^{2})", 300, 0, 6);
}

void TrackPlotter::fillTrackPairHistos(gsl::span<const MuonTrack> tracks)
{
  if (tracks.size() > 1) {
    for (auto i = 0; i < tracks.size(); i++) {
      auto ti = tracks[i].getMuonMomentumAtVertex();
      for (auto j = i + 1; j < tracks.size(); j++) {
        if (tracks[i].getSign() == tracks[j].getSign()) {
          continue;
        }
        auto dt = tracks[j].getIR().differenceInBCNS(tracks[i].getIR());
        if (std::abs(dt) > 1000) {
          continue;
        }
        auto tj = tracks[j].getMuonMomentumAtVertex();
        auto p = ti + tj;
        mMinv->Fill(p.M());
      }
    }
  }
}

bool TrackPlotter::fillTrackHistos(const MuonTrack& track)
{
  int q = (track.getSign() < 0) ? 1 : 0;

  mTrackBC->Fill(track.getIR().bc);

  switch (mSrc) {
    case GID::MCHMID: {
      auto dt = track.getIRMID().toLong() - track.getIRMCH().toLong();
      mTrackDT->Fill(dt);
      break;
    }
    case GID::MFTMCH: {
      auto dt = track.getIRMFT().toLong() - track.getIRMCH().toLong();
      mTrackDT->Fill(dt);
      break;
    }
    case GID::MFTMCHMID: {
      auto dt1 = track.getIRMFT().toLong() - track.getIRMCH().toLong();
      auto dt2 = track.getIRMID().toLong() - track.getIRMCH().toLong();
      auto* htemp = dynamic_cast<TH2F*>(mTrackDT.get());
      if (htemp) {
        htemp->Fill(dt1, dt2);
      }
      break;
    }
    default:
      break;
  }

  double dca = track.getDCA();
  mTrackDCA[q]->Fill(dca);
  mTrackDCA[2]->Fill(dca);

  double pdca = track.getPDCAMCH();
  mTrackPDCA[q]->Fill(pdca);
  mTrackPDCA[2]->Fill(pdca);

  auto muon = track.getMuonMomentumAtVertex();

  mTrackEta[q]->Fill(muon.eta());
  mTrackEta[2]->Fill(muon.eta());
  mTrackPhi[q]->Fill(muon.phi() * TMath::RadToDeg() + 180);
  mTrackPhi[2]->Fill(muon.phi() * TMath::RadToDeg() + 180);
  mTrackPt[q]->Fill(muon.pt());
  mTrackPt[2]->Fill(muon.pt());

  auto rAbs = track.getRAbs();
  mTrackRAbs[q]->Fill(rAbs);
  mTrackRAbs[2]->Fill(rAbs);

  return true;
}

void TrackPlotter::fillHistograms(const o2::globaltracking::RecoContainer& recoCont)
{
  std::vector<MuonTrack> muonTracks;
  if (mSrc == GID::MCH) {
    auto tracksMCH = recoCont.getMCHTracks();
    for (auto& t : tracksMCH) {
      muonTracks.emplace_back(&t, recoCont);
    }
  }
  if (mSrc == GID::MFTMCH || mSrc == GID::MFTMCHMID) {
    auto tracksFwd = recoCont.getGlobalFwdTracks();
    for (auto& t : tracksFwd) {
      MuonTrack mt(&t, recoCont);
      // skip tracks without MID if full matching is requested
      if (mSrc == GID::MFTMCHMID && !mt.hasMID()) {
        continue;
      }
      muonTracks.emplace_back(&t, recoCont);
    }
  }
  if (mSrc == GID::MCHMID) {
    auto tracksMCHMID = recoCont.getMCHMIDMatches();
    for (auto& t : tracksMCHMID) {
      muonTracks.emplace_back(&t, recoCont);
    }
  }

  int nPos{ 0 };
  int nNeg{ 0 };

  for (auto& t : muonTracks) {
    if (t.getSign() < 0) {
      nNeg += 1;
    } else {
      nPos += 1;
    }
  }

  mNofTracksPerTF[0]->Fill(nPos);
  mNofTracksPerTF[1]->Fill(nNeg);
  mNofTracksPerTF[2]->Fill(muonTracks.size());

  decltype(muonTracks.size()) nok{ 0 };

  for (const auto& mt : muonTracks) {
    bool ok = fillTrackHistos(mt);
    if (ok) {
      ++nok;
    }
  }

  fillTrackPairHistos(muonTracks);
}

} // namespace o2::quality_control_modules::muon
