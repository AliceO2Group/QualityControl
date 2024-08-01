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

#ifndef QC_MODULE_MUON_COMMON_TRACK_PLOTTER_H
#define QC_MODULE_MUON_COMMON_TRACK_PLOTTER_H

#include "MUONCommon/HistPlotter.h"
#include "MUONCommon/MuonTrack.h"
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"

#include "ReconstructionDataFormats/GlobalTrackID.h"
#include <DataFormatsGlobalTracking/RecoContainer.h>
#include "MFTTracking/Constants.h"

#include <TH1F.h>
#include <TH2F.h>
#include <TProfile.h>

#include <fmt/core.h>
#include <gsl/span>
#include <memory>
#include <string>
#include <vector>
#include <functional>

using GID = o2::dataformats::GlobalTrackID;

using namespace o2::quality_control_modules::common;

using MuonCutFunc = std::function<bool(const o2::quality_control_modules::muon::MuonTrack&)>;
using DiMuonCutFunc = std::function<bool(const o2::quality_control_modules::muon::MuonTrack&, const o2::quality_control_modules::muon::MuonTrack&)>;

namespace o2::quality_control_modules::muon
{

class TrackPlotter : public HistPlotter
{
 public:
  static constexpr Double_t sLastMFTPlaneZ = o2::mft::constants::mft::LayerZCoordinate()[9];

  TrackPlotter(int maxTracksPerTF, int etaBins, int phiBins, int ptBins, GID::Source source, std::string path, bool fullHistos = false);
  ~TrackPlotter() = default;

 public:
  void setMuonCuts(const std::vector<MuonCutFunc>& cuts) { mMuonCuts = cuts; }
  void setDiMuonCuts(const std::vector<DiMuonCutFunc>& cuts) { mDiMuonCuts = cuts; }

  void addMuonCut(const MuonCutFunc f) { mMuonCuts.push_back(f); }
  void addDiMuonCut(const DiMuonCutFunc f) { mDiMuonCuts.push_back(f); }

  void fillHistograms(const o2::globaltracking::RecoContainer& recoCont);
  void setFirstTForbit(int orbit) { mFirstTForbit = orbit; }

  void endOfCycle();

  const std::vector<std::pair<MuonTrack, bool>>& getMuonTracks() const { return mMuonTracks; }

 private:
  /** create histograms related to tracks */
  void createTrackHistos(int maxTracksPerTF, int etaBins, int phiBins, int ptBins);
  /** create histograms related to track pairs */
  void createTrackPairHistos();

  /** create one histogram with relevant drawing options / stat box status.*/
  template <typename T>
  std::unique_ptr<T> createHisto(const char* name, const char* title,
                                 int nbins, double xmin, double xmax,
                                 bool optional,
                                 bool statBox = false,
                                 const char* drawOptions = "",
                                 const char* displayHints = "");
  template <typename T>
  std::unique_ptr<T> createHisto(const char* name, const char* title,
                                 int nbins, double* xbins,
                                 bool optional,
                                 bool statBox = false,
                                 const char* drawOptions = "",
                                 const char* displayHints = "");
  template <typename T>
  std::unique_ptr<T> createHisto(const char* name, const char* title,
                                 int nbins, double xmin, double xmax,
                                 int nbinsy, double ymin, double ymax,
                                 bool optional,
                                 bool statBox = false,
                                 const char* drawOptions = "",
                                 const char* displayHints = "");

  /** fill histograms related to a single track */
  void fillTrackHistos(const MuonTrack& track);

  /** fill histograms for track pairs */
  void fillTrackPairHistos(gsl::span<const std::pair<MuonTrack, bool>> tracks);

  void normalizePlot(TH1* h);

  GID::Source mSrc;
  std::string mPath;

  bool mFullHistos;

  uint32_t mFirstTForbit{ 0 };
  int mNOrbitsPerTF{ -1 };

  std::vector<std::pair<MuonTrack, bool>> mMuonTracks;

  std::vector<MuonCutFunc> mMuonCuts;
  std::vector<DiMuonCutFunc> mDiMuonCuts;

  std::unique_ptr<TH1DRatio> mTrackBC;                         ///< BC associated to the track
  std::unique_ptr<TH1D> mTrackDT;                              ///< time difference between MCH/MID tracks segments
  std::array<std::unique_ptr<TH1D>, 3> mNofTracksPerTF;        ///< number of tracks per TF
  std::array<std::unique_ptr<TH1DRatio>, 3> mTrackChi2OverNDF; ///< chi2/ndf for the track
  std::array<std::unique_ptr<TH1DRatio>, 3> mTrackDCA;         ///< DCA (cm) of the track
  std::array<std::unique_ptr<TH1DRatio>, 3> mTrackPDCA;        ///< p (GeV/c) x DCA (cm) of the track
  std::array<std::unique_ptr<TH1DRatio>, 3> mTrackRAbs;        ///< R at absorber end of the track
  // kinematic variables, using MCH tracks parameters
  std::array<std::unique_ptr<TH1DRatio>, 3> mTrackEta;    ///< eta of the track
  std::array<std::unique_ptr<TH1DRatio>, 3> mTrackPhi;    ///< phi (in degrees) of the track
  std::array<std::unique_ptr<TH1DRatio>, 3> mTrackPt;     ///< Pt (Gev/c) of the track
  std::unique_ptr<TH1DRatio> mTrackQOverPt;               ///< Q / Pt of the track
  std::array<std::unique_ptr<TH2DRatio>, 3> mTrackEtaPhi; ///< phi (in degrees) vs. eta of the track
  std::array<std::unique_ptr<TH2DRatio>, 3> mTrackEtaPt;  ///< Pt (Gev/c) vs. eta of the track
  std::array<std::unique_ptr<TH2DRatio>, 3> mTrackPhiPt;  ///< Pt (Gev/c) vs. phi (in degrees) of the track
  // kinematic variables, using global tracks parameters (only instantiated when MFT is included)
  std::array<std::unique_ptr<TH1DRatio>, 3> mTrackEtaGlobal;    ///< eta of the track
  std::array<std::unique_ptr<TH1DRatio>, 3> mTrackPhiGlobal;    ///< phi (in degrees) of the track
  std::array<std::unique_ptr<TH1DRatio>, 3> mTrackPtGlobal;     ///< Pt (Gev/c) of the track
  std::unique_ptr<TH1DRatio> mTrackQOverPtGlobal;               ///< Q / Pt of the track
  std::array<std::unique_ptr<TH2DRatio>, 3> mTrackEtaPhiGlobal; ///< phi (in degrees) vs. eta of the track
  std::array<std::unique_ptr<TH2DRatio>, 3> mTrackEtaPtGlobal;  ///< Pt (Gev/c) vs. eta of the track
  std::array<std::unique_ptr<TH2DRatio>, 3> mTrackPhiPtGlobal;  ///< Pt (Gev/c) vs. phi (in degrees) of the track

  std::unique_ptr<TH2DRatio> mTrackPosAtMFT; ///< MCH track poisiton at MFT exit
  std::unique_ptr<TH2DRatio> mTrackPosAtMID; ///< MCH track poisiton at MID entrance

  std::unique_ptr<TH1D> mMatchChi2MCHMID;

  // plots specific to MFT-MCH(-MID) matched tracks
  std::unique_ptr<TH1D> mMatchNMFTCandidates;
  std::unique_ptr<TH1D> mMatchScoreMFTMCH;
  std::unique_ptr<TH1D> mMatchChi2MFTMCH;
  std::array<std::unique_ptr<TH2DRatio>, 3> mTrackEtaCorr;   ///< correlation between MCH and global track parameters - eta
  std::array<std::unique_ptr<TH2DRatio>, 3> mTrackDEtaVsEta; ///< deviation between MCH and global track parameters - eta
  std::array<std::unique_ptr<TH2DRatio>, 3> mTrackPhiCorr;   ///< correlation between MCH and global track parameters - phi
  std::array<std::unique_ptr<TH2DRatio>, 3> mTrackDPhiVsPhi; ///< deviation between MCH and global track parameters - phi
  std::array<std::unique_ptr<TH2DRatio>, 3> mTrackPtCorr;    ///< correlation between MCH and global track parameters - pT
  std::array<std::unique_ptr<TH2DRatio>, 3> mTrackDPtVsPt;   ///< deviation between MCH and global track parameters - pT

  std::unique_ptr<TH1DRatio> mMinvFull; ///< invariant mass of unlike-sign track pairs
  std::unique_ptr<TH1DRatio> mMinv;     ///< invariant mass background of unlike-sign track pairs
  std::unique_ptr<TH1DRatio> mMinvBgd;  ///< invariant mass background of unlike-sign, out-of-time track pairs
  std::unique_ptr<TH1DRatio> mDimuonDT; ///< time difference between di-muon tracks
};

template <typename T>
std::unique_ptr<T> TrackPlotter::createHisto(const char* name, const char* title,
                                             int nbins, double xmin, double xmax,
                                             bool optional,
                                             bool statBox,
                                             const char* drawOptions,
                                             const char* displayHints)
{
  if (optional && !mFullHistos) {
    return nullptr;
  }
  std::string fullTitle = std::string("[") + GID::getSourceName(mSrc) + "] " + title;
  auto h = std::make_unique<T>(name, fullTitle.c_str(), nbins, xmin, xmax);
  if (!statBox) {
    h->SetStats(0);
  }
  histograms().emplace_back(HistInfo{ h.get(), drawOptions, displayHints });
  return h;
}

template <typename T>
std::unique_ptr<T> TrackPlotter::createHisto(const char* name, const char* title,
                                             int nbins, double* xbins,
                                             bool optional,
                                             bool statBox,
                                             const char* drawOptions,
                                             const char* displayHints)
{
  if (optional && !mFullHistos) {
    return nullptr;
  }
  std::string fullTitle = std::string("[") + GID::getSourceName(mSrc) + "] " + title;
  auto h = std::make_unique<T>(name, fullTitle.c_str(), nbins, xbins);
  if (!statBox) {
    h->SetStats(0);
  }
  histograms().emplace_back(HistInfo{ h.get(), drawOptions, displayHints });
  return h;
}

template <typename T>
std::unique_ptr<T> TrackPlotter::createHisto(const char* name, const char* title,
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
  std::string fullTitle = std::string("[") + GID::getSourceName(mSrc) + "] " + title;
  auto h = std::make_unique<T>(name, fullTitle.c_str(), nbins, xmin, xmax, nbinsy, ymin, ymax);
  if (!statBox) {
    h->SetStats(0);
  }
  histograms().emplace_back(HistInfo{ h.get(), drawOptions, displayHints });
  return h;
}

} // namespace o2::quality_control_modules::muon

#endif
