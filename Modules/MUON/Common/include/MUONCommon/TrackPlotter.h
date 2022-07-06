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

#include <MCHRawElecMap/Mapper.h>
#include <TH1F.h>
#include <TProfile.h>
#include <gsl/span>
#include <memory>
#include <string>
#include <fmt/core.h>
#include "MCHGeometryTransformer/Transformations.h"

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
}
class TH1F;

namespace o2::quality_control_modules::muon
{

class TrackPlotter
{
 public:
  TrackPlotter(o2::mch::geo::TransformationCreator transformation, int maxTracksPerTF);
  ~TrackPlotter() = default;

 public:
  void fillHistograms(gsl::span<const o2::mch::ROFRecord> rofs,
                      gsl::span<const o2::mch::TrackMCH> tracks,
                      gsl::span<const o2::mch::Cluster> clusters,
                      gsl::span<const o2::mch::Digit> digits);

  void fillHistograms(gsl::span<const o2::mch::ROFRecord> rofs,
                      gsl::span<const o2::mch::TrackMCH> tracks,
                      gsl::span<const o2::mch::Cluster> clusters,
                      gsl::span<const o2::mch::Digit> digits,
                      gsl::span<const o2::dataformats::TrackMCHMID> muonTracks);

  struct HistInfo {
    TH1* histo;
    std::string drawOptions;
    std::string displayHints;
  };

  const std::vector<HistInfo>& histograms() const { return mHistograms; }

 private:
  /** create one histogram with relevant drawing options / stat box status.*/
  template <typename T>
  std::unique_ptr<T> createHisto(const char* name, const char* title,
                                 int nbins, double xmin, double xmax,
                                 bool statBox = false,
                                 const char* drawOptions = "",
                                 const char* displayHints = "");

  /** create histograms related to clusters (those attached to tracks) */
  void createClusterHistos();
  /** create histograms related to tracks */
  void createTrackHistos(int maxTracksPerTF);
  /** create histograms related to track pairs */
  void createTrackPairHistos();

  /** fill histogram related to each cluster */
  void fillClusterHistos(gsl::span<const o2::mch::Cluster> clusters,
                         gsl::span<const o2::mch::Digit> digits);

  /** fill histograms related to a single track */
  bool fillTrackHistos(const o2::mch::TrackMCH& track,
                       gsl::span<const o2::mch::Cluster> clusters,
                       gsl::span<const o2::mch::Digit> digits);

  /** fill histograms for track pairs */
  void fillTrackPairHistos(gsl::span<const o2::mch::TrackMCH> tracks);

  int dsbinx(int deid, int dsid) const;

  std::unique_ptr<TH1F> mNofTracksPerTF;   ///< number of tracks per TF
  std::unique_ptr<TH1F> mTrackBC;          ///< BC associated to the track
  std::unique_ptr<TH1F> mTrackBCWidth;     ///< BC width associated to the track
  std::unique_ptr<TH1F> mTrackChi2OverNDF; ///< chi2/ndf for the track
  std::unique_ptr<TH1F> mTrackDCA;         ///< DCA (cm) of the track
  std::unique_ptr<TH1F> mTrackEta;         ///< eta of the track
  std::unique_ptr<TH1F> mTrackPDCA;        ///< p (GeV/c) x DCA (cm) of the track
  std::unique_ptr<TH1F> mTrackPhi;         ///< phi (in degrees) of the track
  std::unique_ptr<TH1F> mTrackPt;          ///< Pt (Gev/c^2) of the track
  std::unique_ptr<TH1F> mTrackRAbs;        ///< R at absorber end of the track

  std::unique_ptr<TH1F> mNofClustersPerDualSampa;             //< aka cluster map
  std::unique_ptr<TH1F> mNofClustersPerTrack;                 ///< number of clusters per track
  std::unique_ptr<TProfile> mClusterBendingSizePerChamber;    ///< mean cluster size in bending direction (y) per chamber
  std::unique_ptr<TProfile> mClusterNonBendingSizePerChamber; ///< mean cluster size in non-bending direction (x) per chamber
  std::unique_ptr<TProfile> mClusterSizePerChamber;           ///< mean cluster size per chamber
  std::unique_ptr<TProfile> mNofClustersPerChamber;           ///< mean number of clusters per chamber

  std::unique_ptr<TH1F> mMinv; ///< invariant mass of unlike-sign track pairs

  o2::mch::raw::Det2ElecMapper mDet2ElecMapper;
  o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper;

  std::vector<HistInfo> mHistograms;
  o2::mch::geo::TransformationCreator mTransformation; ///< creator of local to global transformations
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
  mHistograms.emplace_back(HistInfo{ h.get(), drawOptions, displayHints });
  return h;
}

} // namespace o2::quality_control_modules::muon

#endif
