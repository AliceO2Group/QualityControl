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

#include "ReconstructionDataFormats/GlobalTrackID.h"
#include <DataFormatsGlobalTracking/RecoContainer.h>

#include <TH1F.h>
#include <TProfile.h>

#include <fmt/core.h>
#include <gsl/span>
#include <memory>
#include <string>
#include <vector>
/*
namespace o2::mch
{
class Cluster;
class ROFRecord;
class TrackMCH;
class Digit;
} // namespace o2::mch

namespace o2::dataformats
{
class TrackMCHMID;
class GlobalFwdTrack;
}
*/
using GID = o2::dataformats::GlobalTrackID;

namespace o2::quality_control_modules::muon
{

class TrackPlotter : public HistPlotter
{
 public:
  TrackPlotter(int maxTracksPerTF, GID::Source source, std::string path);
  ~TrackPlotter() = default;

 public:
  void fillHistograms(const o2::globaltracking::RecoContainer& recoCont);

 private:
  /** create histograms related to tracks */
  void createTrackHistos(int maxTracksPerTF);
  /** create histograms related to track pairs */
  void createTrackPairHistos();

  /** create one histogram with relevant drawing options / stat box status.*/
  template <typename T>
  std::unique_ptr<T> createHisto(const char* name, const char* title,
                                 int nbins, double xmin, double xmax,
                                 bool statBox = false,
                                 const char* drawOptions = "",
                                 const char* displayHints = "");

  /** fill histograms related to a single track */
  bool fillTrackHistos(const MuonTrack& track);

  /** fill histograms for track pairs */
  void fillTrackPairHistos(gsl::span<const MuonTrack> tracks);

  GID::Source mSrc;
  std::string mPath;

  std::unique_ptr<TH1F> mTrackBC;                         ///< BC associated to the track
  std::unique_ptr<TH1F> mTrackBCWidth;                    ///< BC width associated to the track
  std::unique_ptr<TH1> mTrackDT;                          ///< time difference between MFT/MCH/MID tracks segments
  std::array<std::unique_ptr<TH1F>, 3> mNofTracksPerTF;   ///< number of tracks per TF
  std::array<std::unique_ptr<TH1F>, 3> mTrackDCA;         ///< DCA (cm) of the track
  std::array<std::unique_ptr<TH1F>, 3> mTrackEta;         ///< eta of the track
  std::array<std::unique_ptr<TH1F>, 3> mTrackPDCA;        ///< p (GeV/c) x DCA (cm) of the track
  std::array<std::unique_ptr<TH1F>, 3> mTrackPhi;         ///< phi (in degrees) of the track
  std::array<std::unique_ptr<TH1F>, 3> mTrackPt;          ///< Pt (Gev/c^2) of the track
  std::array<std::unique_ptr<TH1F>, 3> mTrackRAbs;        ///< R at absorber end of the track

  std::unique_ptr<TH1F> mMinv; ///< invariant mass of unlike-sign track pairs
};

template <typename T>
std::unique_ptr<T> TrackPlotter::createHisto(const char* name, const char* title,
                                             int nbins, double xmin, double xmax,
                                             bool statBox,
                                             const char* drawOptions,
                                             const char* displayHints)
{
  auto h = std::make_unique<T>(name, title, nbins, xmin, xmax);
  if (!statBox) {
    h->SetStats(0);
  }
  histograms().emplace_back(HistInfo{ h.get(), drawOptions, displayHints });
  return h;
}

} // namespace o2::quality_control_modules::muon

#endif
