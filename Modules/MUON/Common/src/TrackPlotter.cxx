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
#include "MUONCommon/Helpers.h"

#include "DetectorsBase/GRPGeomHelper.h"
#include <DataFormatsITSMFT/ROFRecord.h>
#include <DataFormatsMFT/TrackMFT.h>
#include <DataFormatsMCH/TrackMCH.h>
#include <ReconstructionDataFormats/TrackMCHMID.h>
#include <ReconstructionDataFormats/GlobalFwdTrack.h>
#include <MCHTracking/TrackExtrap.h>
#include <DetectorsBase/GeometryManager.h>
#include <Framework/DataRefUtils.h>
#include <Framework/InputRecord.h>
#include <CommonConstants/LHCConstants.h>
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

template <class HIST>
static void Fill(std::unique_ptr<HIST>& hist, double x)
{
  if (!hist) {
    return;
  }
  hist->Fill(x);
}

template <class HIST>
static void Fill(std::unique_ptr<HIST>& hist, double x, double y)
{
  if (!hist) {
    return;
  }
  hist->Fill(x, y);
}

template <>
void Fill<TH1DRatio>(std::unique_ptr<TH1DRatio>& hist, double x)
{
  if (!hist) {
    return;
  }
  hist->getNum()->Fill(x);
}

template <>
void Fill<TH2DRatio>(std::unique_ptr<TH2DRatio>& hist, double x, double y)
{
  if (!hist) {
    return;
  }
  hist->getNum()->Fill(x, y);
}

} // namespace

using namespace o2::dataformats;

namespace o2::quality_control_modules::muon
{

TrackPlotter::TrackPlotter(int maxTracksPerTF,
                           int etaBins, int phiBins, int ptBins,
                           GID::Source source,
                           std::string path,
                           bool fullHistos)
  : mSrc(source),
    mPath(path),
    mFullHistos(fullHistos)
{
  createTrackHistos(maxTracksPerTF, etaBins, phiBins, ptBins);
  createTrackPairHistos();
}

template <>
std::unique_ptr<TH1DRatio> TrackPlotter::createHisto(const char* name, const char* title,
                                                     int nbins, double xmin, double xmax,
                                                     bool optional,
                                                     bool statBox,
                                                     const char* drawOptions,
                                                     const char* displayHints)
{
  if (optional && !mFullHistos) {
    return nullptr;
  }
  std::string fullTitle = GID::getSourceName(mSrc) + " " + title;
  auto h = std::make_unique<TH1DRatio>(name, fullTitle.c_str(), nbins, xmin, xmax, true);
  if (!statBox) {
    h->SetStats(0);
  }
  histograms().emplace_back(HistInfo{ h.get(), drawOptions, displayHints });
  return h;
}

template <>
std::unique_ptr<TH2DRatio> TrackPlotter::createHisto(const char* name, const char* title,
                                                     int nbins, double xmin, double xmax,
                                                     int nbinsy, double ymin, double ymax,
                                                     bool optional,
                                                     bool statBox,
                                                     const char* drawOptions,
                                                     const char* displayHints)
{
  if (optional && !mFullHistos) {
    return nullptr;
  }
  std::string fullTitle = GID::getSourceName(mSrc) + " " + title;
  auto h = std::make_unique<TH2DRatio>(name, fullTitle.c_str(), nbins, xmin, xmax, nbinsy, ymin, ymax, true);
  if (!statBox) {
    h->SetStats(0);
  }
  histograms().emplace_back(HistInfo{ h.get(), drawOptions, displayHints });
  return h;
}

void TrackPlotter::createTrackHistos(int maxTracksPerTF, int etaBins, int phiBins, int ptBins)
{
  int nbins = 100;
  auto xLogBins = makeLogBinning(1, maxTracksPerTF, nbins);
  mNofTracksPerTF[0] = createHisto<TH1D>(TString::Format("%sPositive/TracksPerTF", mPath.c_str()), "Number of tracks per TimeFrame (+);Number of tracks per TF", nbins, xLogBins.data(), true, false, "logx");
  mNofTracksPerTF[1] = createHisto<TH1D>(TString::Format("%sNegative/TracksPerTF", mPath.c_str()), "Number of tracks per TimeFrame (-);Number of tracks per TF", nbins, xLogBins.data(), true, false, "logx");
  mNofTracksPerTF[2] = createHisto<TH1D>(TString::Format("%sTracksPerTF", mPath.c_str()), "Number of tracks per TimeFrame;Number of tracks per TF", nbins, xLogBins.data(), false, false, "logx");

  mTrackChi2OverNDF[0] = createHisto<TH1DRatio>(TString::Format("%sPositive/TrackMCHChi2OverNDF", mPath.c_str()), "Track #chi^{2}/ndf (MCH +);#chi^{2}/ndf;entries/s", 500, 0, 50, true, false, "hist");
  mTrackChi2OverNDF[1] = createHisto<TH1DRatio>(TString::Format("%sNegative/TrackMCHChi2OverNDF", mPath.c_str()), "Track #chi^{2}/ndf (MCH -);#chi^{2}/ndf;entries/s", 500, 0, 50, true, false, "hist");
  mTrackChi2OverNDF[2] = createHisto<TH1DRatio>(TString::Format("%sTrackMCHChi2OverNDF", mPath.c_str()), "Track #chi^{2}/ndf (MCH);#chi^{2}/ndf;entries/s", 500, 0, 50, false, false, "hist");

  mTrackDCA[0] = createHisto<TH1DRatio>(TString::Format("%sPositive/TrackDCA", mPath.c_str()), "Track DCA (+);DCA (cm);entries/s", 500, 0, 500, true, false, "hist");
  mTrackDCA[1] = createHisto<TH1DRatio>(TString::Format("%sNegative/TrackDCA", mPath.c_str()), "Track DCA (-);DCA (cm);entries/s", 500, 0, 500, true, false, "hist");
  mTrackDCA[2] = createHisto<TH1DRatio>(TString::Format("%sTrackDCA", mPath.c_str()), "Track DCA;DCA (cm);entries/s", 500, 0, 500, false, false, "hist");

  mTrackPDCA[0] = createHisto<TH1DRatio>(TString::Format("%sPositive/TrackPDCA", mPath.c_str()), "Track p#timesDCA (+);p#timesDCA (GeVcm/c);entries/s", 5000, 0, 5000, true, false, "hist");
  mTrackPDCA[1] = createHisto<TH1DRatio>(TString::Format("%sNegative/TrackPDCA", mPath.c_str()), "Track p#timesDCA (-);p#timesDCA (GeVcm/c);entries/s", 5000, 0, 5000, true, false, "hist");
  mTrackPDCA[2] = createHisto<TH1DRatio>(TString::Format("%sTrackPDCA", mPath.c_str()), "Track p#timesDCA;p#timesDCA (GeVcm/c);entries/s", 5000, 0, 5000, false, false, "hist");

  mTrackPt[0] = createHisto<TH1DRatio>(TString::Format("%sPositive/TrackPt", mPath.c_str()), "Track p_{T} (+);p_{T} (GeV/c);entries/s", ptBins, 0, 30, true, false, "hist logy");
  mTrackPt[1] = createHisto<TH1DRatio>(TString::Format("%sNegative/TrackPt", mPath.c_str()), "Track p_{T} (-);p_{T} (GeV/c);entries/s", ptBins, 0, 30, true, false, "hist logy");
  mTrackPt[2] = createHisto<TH1DRatio>(TString::Format("%sTrackPt", mPath.c_str()), "Track p_{T};p_{T} (GeV/c);entries/s", ptBins, 0, 30, false, false, "hist logy");

  mTrackQOverPt = createHisto<TH1DRatio>(TString::Format("%sTrackQOverPt", mPath.c_str()), "Track q/p_{T};q/p_{T} (GeV/c)^{-1};entries/s", 200, -10, 10, false, false, "hist logy");

  mTrackEta[0] = createHisto<TH1DRatio>(TString::Format("%sPositive/TrackEta", mPath.c_str()), "Track #eta (+);#eta;entries/s", etaBins, -4.5, -2, true, false, "hist");
  mTrackEta[1] = createHisto<TH1DRatio>(TString::Format("%sNegative/TrackEta", mPath.c_str()), "Track #eta (-);#eta;entries/s", etaBins, -4.5, -2, true, false, "hist");
  mTrackEta[2] = createHisto<TH1DRatio>(TString::Format("%sTrackEta", mPath.c_str()), "Track #eta;#eta;entries/s", etaBins, -4.5, -2, false, false, "hist");

  mTrackPhi[0] = createHisto<TH1DRatio>(TString::Format("%sPositive/TrackPhi", mPath.c_str()), "Track #phi (+);#phi (deg);entries/s", phiBins, -180, 180, true, false, "hist");
  mTrackPhi[1] = createHisto<TH1DRatio>(TString::Format("%sNegative/TrackPhi", mPath.c_str()), "Track #phi (-);#phi (deg);entries/s", phiBins, -180, 180, true, false, "hist");
  mTrackPhi[2] = createHisto<TH1DRatio>(TString::Format("%sTrackPhi", mPath.c_str()), "Track #phi;#phi (deg);entries/s", phiBins, -180, 180, false, false, "hist");

  mTrackRAbs[0] = createHisto<TH1DRatio>(TString::Format("%sPositive/TrackRAbs", mPath.c_str()), "Track R_{abs} (+);R_{abs} (cm);entries/s", 1000, 0, 100, true, false, "hist");
  mTrackRAbs[1] = createHisto<TH1DRatio>(TString::Format("%sNegative/TrackRAbs", mPath.c_str()), "Track R_{abs} (-);R_{abs} (cm);entries/s", 1000, 0, 100, true, false, "hist");
  mTrackRAbs[2] = createHisto<TH1DRatio>(TString::Format("%sTrackRAbs", mPath.c_str()), "Track R_{abs};R_{abs} (cm);entries/s", 1000, 0, 100, false, false, "hist");

  mTrackEtaPhi[0] = createHisto<TH2DRatio>(TString::Format("%sPositive/TrackEtaPhi", mPath.c_str()), "Track #phi vs #eta (+);#eta;#phi", etaBins / 5, -4.5, -2, phiBins / 5, -180, 180, true, false, "colz");
  mTrackEtaPhi[1] = createHisto<TH2DRatio>(TString::Format("%sNegative/TrackEtaPhi", mPath.c_str()), "Track #phi vs #eta (-);#eta;#phi", etaBins / 5, -4.5, -2, phiBins / 5, -180, 180, true, false, "colz");
  mTrackEtaPhi[2] = createHisto<TH2DRatio>(TString::Format("%sTrackEtaPhi", mPath.c_str()), "Track #phi vs #eta;#eta;#phi", etaBins / 5, -4.5, -2, phiBins / 5, -180, 180, false, false, "colz");

  mTrackEtaPt[0] = createHisto<TH2DRatio>(TString::Format("%sPositive/TrackEtaPt", mPath.c_str()), "Track p_{T} vs #eta (+);#eta;p_{T} (GeV/c)", etaBins / 5, -4.5, -2, ptBins / 5, 0, 30, true, false, "colz");
  mTrackEtaPt[1] = createHisto<TH2DRatio>(TString::Format("%sNegative/TrackEtaPt", mPath.c_str()), "Track p_{T} vs #eta (-);#eta;p_{T} (GeV/c)", etaBins / 5, -4.5, -2, ptBins / 5, 0, 30, true, false, "colz");
  mTrackEtaPt[2] = createHisto<TH2DRatio>(TString::Format("%sTrackEtaPt", mPath.c_str()), "Track p_{T} vs #eta;#eta;p_{T} (GeV/c)", etaBins / 5, -4.5, -2, ptBins / 5, 0, 30, false, false, "colz");

  mTrackPhiPt[0] = createHisto<TH2DRatio>(TString::Format("%sPositive/TrackPhiPt", mPath.c_str()), "Track p_{T} vs #phi (+);#phi;p_{T} (GeV/c)", phiBins / 5, -180, 180, ptBins / 5, 0, 30, true, false, "colz");
  mTrackPhiPt[1] = createHisto<TH2DRatio>(TString::Format("%sNegative/TrackPhiPt", mPath.c_str()), "Track p_{T} vs #phi (-);#phi;p_{T} (GeV/c)", phiBins / 5, -180, 180, ptBins / 5, 0, 30, true, false, "colz");
  mTrackPhiPt[2] = createHisto<TH2DRatio>(TString::Format("%sTrackPhiPt", mPath.c_str()), "Track p_{T} vs #phi;#phi;p_{T} (GeV/c)", phiBins / 5, -180, 180, ptBins / 5, 0, 30, false, false, "colz");

  mTrackBC = createHisto<TH1DRatio>(TString::Format("%sTrackBC", mPath.c_str()), "Track BC;BC;entries/s", o2::constants::lhc::LHCMaxBunches, 0, o2::constants::lhc::LHCMaxBunches, false, false, "hist");

  if (mSrc == GID::MFTMCH || mSrc == GID::MFTMCHMID) {
    mMatchScoreMFTMCH = createHisto<TH1D>(TString::Format("%sMatchScoreMFTMCH", mPath.c_str()), "Match Score MFT-MCH;score", 1000, 0, 100, false, false, "hist");
    mMatchChi2MFTMCH = createHisto<TH1D>(TString::Format("%sMatchChi2MFTMCH", mPath.c_str()), "Match #chi^{2} MFT-MCH;#chi^{2}", 1000, 0, 100, false, false, "hist");
    mMatchNMFTCandidates = createHisto<TH1D>(TString::Format("%sMatchNMFTCandidates", mPath.c_str()), "MFT Candidates;candidates", 1000, 0, 1000, false, false, "hist");

    mTrackEtaCorr[0] = createHisto<TH2DRatio>(TString::Format("%sPositive/TrackEtaCorr", mPath.c_str()), "Track #eta - GLO vs MCH (+);#eta^{MCH};#eta^{GLO}", etaBins / 5, -4.5, -2, etaBins / 5, -4.5, -2, true, false, "colz");
    mTrackEtaCorr[1] = createHisto<TH2DRatio>(TString::Format("%sNegative/TrackEtaCorr", mPath.c_str()), "Track #eta - GLO vs MCH (-);#eta^{MCH};#eta^{GLO}", etaBins / 5, -4.5, -2, etaBins / 5, -4.5, -2, true, false, "colz");
    mTrackEtaCorr[2] = createHisto<TH2DRatio>(TString::Format("%sTrackEtaCorr", mPath.c_str()), "Track #eta - GLO vs MCH;#eta^{MCH};#eta^{GLO}", etaBins / 5, -4.5, -2, etaBins / 5, -4.5, -2, false, false, "colz");

    mTrackDEtaVsEta[0] = createHisto<TH2DRatio>(TString::Format("%sPositive/TrackDEtaVsEta", mPath.c_str()), "Track #eta^{GLO}-#eta^{MCH} vs #eta^{MCH} (+);#eta^{MCH};#eta^{GLO}-#eta^{MCH}", etaBins / 5, -4.5, -2, 200, -1, 1, true, false, "colz");
    mTrackDEtaVsEta[1] = createHisto<TH2DRatio>(TString::Format("%sNegative/TrackDEtaVsEta", mPath.c_str()), "Track #eta^{GLO}-#eta^{MCH} vs #eta^{MCH} (-);#eta^{MCH};#eta^{GLO}-#eta^{MCH}", etaBins / 5, -4.5, -2, 200, -1, 1, true, false, "colz");
    mTrackDEtaVsEta[2] = createHisto<TH2DRatio>(TString::Format("%sTrackDEtaVsEta", mPath.c_str()), "Track #eta^{GLO}-#eta^{MCH} vs #eta^{MCH};#eta^{MCH};#eta^{GLO}-#eta^{MCH}", etaBins / 5, -4.5, -2, 200, -1, 1, false, false, "colz");

    mTrackPhiCorr[0] = createHisto<TH2DRatio>(TString::Format("%sPositive/TrackPhiCorr", mPath.c_str()), "Track #phi - GLO vs MCH (+);#phi^{MCH};#phi^{GLO}", phiBins / 5, -180, 180, phiBins / 5, -180, 180, true, false, "colz");
    mTrackPhiCorr[1] = createHisto<TH2DRatio>(TString::Format("%sNegative/TrackPhiCorr", mPath.c_str()), "Track #phi - GLO vs MCH (-);#phi^{MCH};#phi^{GLO}", phiBins / 5, -180, 180, phiBins / 5, -180, 180, true, false, "colz");
    mTrackPhiCorr[2] = createHisto<TH2DRatio>(TString::Format("%sTrackPhiCorr", mPath.c_str()), "Track #phi - GLO vs MCH;#phi^{MCH};#phi^{GLO}", phiBins / 5, -180, 180, phiBins / 5, -180, 180, false, false, "colz");

    mTrackDPhiVsPhi[0] = createHisto<TH2DRatio>(TString::Format("%sPositive/TrackDPhiVsPhi", mPath.c_str()), "Track #phi^{GLO}-#phi^{MCH} vs #phi^{MCH} (+);#phi^{MCH};#phi^{GLO}-#phi^{MCH}", phiBins / 5, -180, 180, 200, -100, 100, true, false, "colz");
    mTrackDPhiVsPhi[1] = createHisto<TH2DRatio>(TString::Format("%sNegative/TrackDPhiVsPhi", mPath.c_str()), "Track #phi^{GLO}-#phi^{MCH} vs #phi^{MCH} (-);#phi^{MCH};#phi^{GLO}-#phi^{MCH}", phiBins / 5, -180, 180, 200, -100, 100, true, false, "colz");
    mTrackDPhiVsPhi[2] = createHisto<TH2DRatio>(TString::Format("%sTrackDPhiVsPhi", mPath.c_str()), "Track #phi^{GLO}-#phi^{MCH} vs #phi^{MCH};#phi^{MCH};#phi^{GLO}-#phi^{MCH}", phiBins / 5, -180, 180, 200, -100, 100, false, false, "colz");

    mTrackPtCorr[0] = createHisto<TH2DRatio>(TString::Format("%sPositive/TrackPtCorr", mPath.c_str()), "Track p_{T} - GLO vs MCH (+);p_{T}^{MCH};p_{T}^{GLO}", ptBins / 5, 0, 30, ptBins / 5, 0, 30, true, false, "colz");
    mTrackPtCorr[1] = createHisto<TH2DRatio>(TString::Format("%sNegative/TrackPtCorr", mPath.c_str()), "Track p_{T} - GLO vs MCH (-);p_{T}^{MCH};p_{T}^{GLO}", ptBins / 5, 0, 30, ptBins / 5, 0, 30, true, false, "colz");
    mTrackPtCorr[2] = createHisto<TH2DRatio>(TString::Format("%sTrackPtCorr", mPath.c_str()), "Track p_{T} - GLO vs MCH;p_{T}^{MCH};p_{T}^{GLO}", ptBins / 5, 0, 30, ptBins / 5, 0, 30, false, false, "colz");

    mTrackDPtVsPt[0] = createHisto<TH2DRatio>(TString::Format("%sPositive/TrackDPtVsPt", mPath.c_str()), "Track p_{T}^{GLO}-p_{T}^{MCH} vs p_{T}^{MCH} (+);p_{T}^{MCH};p_{T}^{GLO}-p_{T}^{MCH}", ptBins / 5, 0, 30, 200, -10, 10, true, false, "colz");
    mTrackDPtVsPt[1] = createHisto<TH2DRatio>(TString::Format("%sNegative/TrackDPtVsPt", mPath.c_str()), "Track p_{T}^{GLO}-p_{T}^{MCH} vs p_{T}^{MCH} (-);p_{T}^{MCH};p_{T}^{GLO}-p_{T}^{MCH}", ptBins / 5, 0, 30, 200, -10, 10, true, false, "colz");
    mTrackDPtVsPt[2] = createHisto<TH2DRatio>(TString::Format("%sTrackDPtVsPt", mPath.c_str()), "Track p_{T}^{GLO}-p_{T}^{MCH} vs p_{T}^{MCH};p_{T}^{MCH};p_{T}^{GLO}-p_{T}^{MCH}", ptBins / 5, 0, 30, 200, -10, 10, false, false, "colz");
  }

  if (mSrc == GID::MCHMID || mSrc == GID::MFTMCHMID) {
    mMatchChi2MCHMID = createHisto<TH1D>(TString::Format("%sMatchChi2MCHMID", mPath.c_str()), "Match #chi^{2} MCH-MID;#chi^{2}", 1000, 0, 100, false, true, "hist");
    mTrackDT = createHisto<TH1D>(TString::Format("%sTrackDT", mPath.c_str()), "MCH-MID time correlation;ns", 4000, -500, 500, false, true, "hist");
  }

  mTrackPosAtMFT = createHisto<TH2DRatio>(TString::Format("%sTrackPosAtMFT", mPath.c_str()), "MCH Track position at MFT exit;X (cm);Y (cm)", 100, -50, 50, 100, -50, 50, false, false, "colz");
  mTrackPosAtMID = createHisto<TH2DRatio>(TString::Format("%sTrackPosAtMID", mPath.c_str()), "MCH Track position at MID entrance;X (cm);Y (cm)", 80, -400, 400, 80, -400, 400, false, false, "colz");
}

void TrackPlotter::createTrackPairHistos()
{
  mMinvFull = createHisto<TH1DRatio>(TString::Format("%sMinvFull", mPath.c_str()), "#mu^{+}#mu^{-} invariant mass;M_{#mu^{+}#mu^{-}} (GeV/c^{2})", 5000, 0, 100, false, true, "hist");
  mMinv = createHisto<TH1DRatio>(TString::Format("%sMinv", mPath.c_str()), "#mu^{+}#mu^{-} invariant mass;M_{#mu^{+}#mu^{-}} (GeV/c^{2})", 200, 1, 5, false, true, "hist");
  mMinvBgd = createHisto<TH1DRatio>(TString::Format("%sMinvBgd", mPath.c_str()), "#mu^{+}#mu^{-} inv. mass background;M_{#mu^{+}#mu^{-}} (GeV/c^{2})", 200, 1, 5, true, true, "hist");
  mDimuonDT = createHisto<TH1DRatio>(TString::Format("%sDimuonTimeDiff", mPath.c_str()), "#mu^{+}#mu^{-} time difference;ns", 4000, -2000, 2000, false, true, "hist");
}

void TrackPlotter::normalizePlot(TH1* hist)
{
  static constexpr double sOrbitLengthInSeconds = o2::constants::lhc::LHCOrbitMUS / 1000000;

  TH1DRatio* h1 = dynamic_cast<TH1DRatio*>(hist);
  if (h1) {
    h1->getDen()->Fill((Double_t)0, sOrbitLengthInSeconds * mNOrbitsPerTF);
  } else {
    TH2DRatio* h2 = dynamic_cast<TH2DRatio*>(hist);
    if (h2) {
      h2->getDen()->Fill((Double_t)0, (Double_t)0, sOrbitLengthInSeconds * mNOrbitsPerTF);
    }
  }
}

void TrackPlotter::fillTrackPairHistos(gsl::span<const std::pair<MuonTrack, bool>> tracks)
{
  if (tracks.size() > 1) {
    for (auto i = 0; i < tracks.size(); i++) {
      if (!(tracks[i].second)) {
        continue;
      }
      const MuonTrack& ti = tracks[i].first;
      auto pi = ti.getMuonMomentumAtVertex();

      for (auto j = i + 1; j < tracks.size(); j++) {
        if (!(tracks[j].second)) {
          continue;
        }
        const MuonTrack& tj = tracks[j].first;

        if (ti.getSign() == tj.getSign()) {
          continue;
        }

        auto dt = (ti.getIR() - tj.getIR()).bc2ns();
        auto dtMUS = ti.getTime().getTimeStamp() - tj.getTime().getTimeStamp();
        Fill(mDimuonDT, dtMUS * 1000);

        bool diMuonOK = true;
        for (auto& cut : mDiMuonCuts) {
          if (!cut(ti, tj)) {
            diMuonOK = false;
            break;
          }
        }
        // tracks are considered to be correlated if they are closer than 1 us in time
        auto pj = tj.getMuonMomentumAtVertex();
        auto p = pi + pj;
        if (diMuonOK) {
          Fill(mMinv, p.M());
          Fill(mMinvFull, p.M());
        }
        // the shape of the combinatorial background is derived by combining tracks
        // than belong to different orbits (more than 90 us apart)
        if (std::abs(dtMUS) > 1) {
          Fill(mMinvBgd, p.M());
        }
      }
    }
  }
}

void TrackPlotter::fillTrackHistos(const MuonTrack& track)
{
  int q = (track.getSign() < 0) ? 1 : 0;

  Fill(mTrackBC, track.getIR().bc);

  auto dtMUS = track.getTimeMID().getTimeStamp() - track.getTimeMCH().getTimeStamp();
  Fill(mTrackDT, dtMUS * 1000);

  switch (mSrc) {
    case GID::MCHMID: {
      Fill(mMatchChi2MCHMID, track.getMatchInfoFwd().getMIDMatchingChi2());
      break;
    }
    case GID::MFTMCH: {
      Fill(mMatchScoreMFTMCH, track.getMatchInfoFwd().getMFTMCHMatchingScore());
      Fill(mMatchChi2MFTMCH, track.getMatchInfoFwd().getMFTMCHMatchingChi2());
      Fill(mMatchNMFTCandidates, track.getMatchInfoFwd().getNMFTCandidates());
      break;
    }
    case GID::MFTMCHMID: {
      Fill(mMatchScoreMFTMCH, track.getMatchInfoFwd().getMFTMCHMatchingScore());
      Fill(mMatchChi2MFTMCH, track.getMatchInfoFwd().getMFTMCHMatchingChi2());
      Fill(mMatchChi2MCHMID, track.getMatchInfoFwd().getMIDMatchingChi2());
      Fill(mMatchNMFTCandidates, track.getMatchInfoFwd().getNMFTCandidates());
      break;
    }
    default:
      break;
  }

  double chi2 = track.getChi2OverNDF();
  Fill(mTrackChi2OverNDF[q], chi2);
  Fill(mTrackChi2OverNDF[2], chi2);

  double dca = track.getDCA();
  Fill(mTrackDCA[q], dca);
  Fill(mTrackDCA[2], dca);

  double pdca = track.getPDCAMCH();
  Fill(mTrackPDCA[q], pdca);
  Fill(mTrackPDCA[2], pdca);

  double eta = track.getEta();
  double etaMCH = track.hasMCH() ? track.getEtaMCH() : 0;
  Fill(mTrackEta[q], eta);
  Fill(mTrackEta[2], eta);
  Fill(mTrackEtaCorr[q], etaMCH, eta);
  Fill(mTrackEtaCorr[2], etaMCH, eta);
  Fill(mTrackDEtaVsEta[q], etaMCH, eta - etaMCH);
  Fill(mTrackDEtaVsEta[2], etaMCH, eta - etaMCH);

  double phi = track.getPhi();
  double phiMCH = track.hasMCH() ? track.getPhiMCH() : 0;
  Fill(mTrackPhi[q], phi);
  Fill(mTrackPhi[2], phi);
  Fill(mTrackPhiCorr[q], phiMCH, phi);
  Fill(mTrackPhiCorr[2], phiMCH, phi);
  Fill(mTrackDPhiVsPhi[q], phiMCH, phi - phiMCH);
  Fill(mTrackDPhiVsPhi[2], phiMCH, phi - phiMCH);

  Fill(mTrackEtaPhi[q], eta, phi);
  Fill(mTrackEtaPhi[2], eta, phi);

  double pt = track.getPt();
  double ptMCH = track.hasMCH() ? track.getPtMCH() : 0;
  Fill(mTrackPt[q], pt);
  Fill(mTrackPt[2], pt);
  Fill(mTrackPtCorr[q], ptMCH, pt);
  Fill(mTrackPtCorr[2], ptMCH, pt);
  Fill(mTrackDPtVsPt[q], ptMCH, pt - ptMCH);
  Fill(mTrackDPtVsPt[2], ptMCH, pt - ptMCH);

  Fill(mTrackEtaPt[q], eta, pt);
  Fill(mTrackEtaPt[2], eta, pt);

  Fill(mTrackPhiPt[q], phi, pt);
  Fill(mTrackPhiPt[2], phi, pt);

  if (track.getSign() != 0 && pt != 0) {
    double qOverPt = track.getSign() / pt;
    Fill(mTrackQOverPt, qOverPt);
  }

  auto rAbs = track.getRAbs();
  Fill(mTrackRAbs[q], rAbs);
  Fill(mTrackRAbs[2], rAbs);

  o2::mch::TrackParam trackParamAtMFT;
  float zMFT = sLastMFTPlaneZ;
  track.extrapToZMCH(trackParamAtMFT, zMFT);
  double xMCH = trackParamAtMFT.getNonBendingCoor();
  double yMCH = trackParamAtMFT.getBendingCoor();
  Fill(mTrackPosAtMFT, trackParamAtMFT.getNonBendingCoor(), trackParamAtMFT.getBendingCoor());

  Fill(mTrackPosAtMID, track.getXMid(), track.getYMid());
}

void TrackPlotter::fillHistograms(const o2::globaltracking::RecoContainer& recoCont)
{
  static bool sFirst = true;

  if (sFirst) {
    o2::mch::TrackExtrap::setField();
    sFirst = false;
  }

  if (mNOrbitsPerTF < 0) {
    mNOrbitsPerTF = o2::base::GRPGeomHelper::instance().getNHBFPerTF();
  }

  mMuonTracks.clear();

  if (mSrc == GID::MCH) {
    auto tracksMCH = recoCont.getMCHTracks();
    int trackID = 0;
    for (auto& t : tracksMCH) {
      mMuonTracks.emplace_back(std::make_pair<MuonTrack, bool>({ &t, trackID, recoCont, mFirstTForbit }, true));
      trackID += 1;
    }
  }
  if (mSrc == GID::MFTMCH || mSrc == GID::MFTMCHMID) {
    auto tracksFwd = recoCont.getGlobalFwdTracks();
    for (auto& t : tracksFwd) {
      MuonTrack mt(&t, recoCont, mFirstTForbit);
      // skip tracks without MID if full matching is requested
      if (mSrc == GID::MFTMCHMID && !mt.hasMID()) {
        continue;
      }
      mMuonTracks.emplace_back(std::make_pair<MuonTrack, bool>({ &t, recoCont, mFirstTForbit }, true));
    }
  }
  if (mSrc == GID::MCHMID) {
    auto tracksMCHMID = recoCont.getMCHMIDMatches();
    for (auto& t : tracksMCHMID) {
      mMuonTracks.emplace_back(std::make_pair<MuonTrack, bool>({ &t, recoCont, mFirstTForbit }, true));
    }
  }

  int nPos{ 0 };
  int nNeg{ 0 };
  int nTot{ 0 };

  for (auto& t : mMuonTracks) {
    bool ok = true;
    for (auto& cut : mMuonCuts) {
      if (!cut(t.first)) {
        ok = false;
        break;
      }
    }
    t.second = ok;
    if (!t.second) {
      continue;
    }

    nTot += 1;
    if (t.first.getSign() < 0) {
      nNeg += 1;
    } else {
      nPos += 1;
    }
  }

  Fill(mNofTracksPerTF[0], nPos);
  Fill(mNofTracksPerTF[1], nNeg);
  Fill(mNofTracksPerTF[2], nTot);

  for (const auto& mt : mMuonTracks) {
    if (!mt.second) {
      continue;
    }
    fillTrackHistos(mt.first);
  }

  fillTrackPairHistos(mMuonTracks);

  for (auto hinfo : histograms()) {
    TH1* h1 = dynamic_cast<TH1*>(hinfo.object);
    if (h1) {
      normalizePlot(h1);
    }
  }
}

void TrackPlotter::endOfCycle()
{
  for (auto hinfo : histograms()) {
    TH1DRatio* h1 = dynamic_cast<TH1DRatio*>(hinfo.object);
    if (h1) {
      h1->update();
    } else {
      TH2DRatio* h2 = dynamic_cast<TH2DRatio*>(hinfo.object);
      if (h2) {
        h2->update();
      }
    }
  }
}

} // namespace o2::quality_control_modules::muon
