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

#include "CommonConstants/LHCConstants.h"
#include <DataFormatsMCH/Cluster.h>
#include <DataFormatsMCH/Digit.h>
#include <DataFormatsMCH/ROFRecord.h>
#include <DataFormatsMCH/TrackMCH.h>
#include <DetectorsBase/GeometryManager.h>
#include <Framework/DataRefUtils.h>
#include <Framework/InputRecord.h>
#include <MCHMappingInterface/Segmentation.h>
#include <MCHTracking/TrackExtrap.h>
#include <MCHTracking/TrackParam.h>
#include <Math/Vector4D.h>
#include <Math/Vector4Dfwd.h>
#include <ReconstructionDataFormats/TrackMCHMID.h>
#include <TH1F.h>
#include <TMath.h>
#include <gsl/span>
#include <set>

namespace
{

constexpr double muonMass = 0.1056584;
constexpr double muonMass2 = muonMass * muonMass;

static void setXAxisLabels(TProfile* h)
{
  TAxis* axis = h->GetXaxis();
  for (int i = 1; i <= 10; i++) {
    auto label = fmt::format("CH{}", i);
    axis->SetBinLabel(i, label.c_str());
  }
}

uint16_t computeDsBinX(int feeId, int linkId, int elinkId)
{
  constexpr uint64_t maxLinkId = 12;
  constexpr uint64_t maxElinkId = 40;

  int v = feeId * maxLinkId * maxElinkId +
          (linkId % maxLinkId) * maxElinkId + elinkId + 1;
  return static_cast<uint16_t>(v & 0xFFFF);
}

std::array<int, 10> getClustersPerChamber(gsl::span<const o2::mch::Cluster> clusters)
{
  std::array<int, 10> clustersPerChamber;
  clustersPerChamber.fill(0);
  for (const auto& cluster : clusters) {
    int chamberId = cluster.getChamberId();
    clustersPerChamber[chamberId]++;
  }
  return clustersPerChamber;
}

} // namespace

namespace o2::quality_control_modules::muon
{

TrackPlotter::TrackPlotter(o2::mch::geo::TransformationCreator transformation, int maxTracksPerTF) : mTransformation(transformation)
{
  createClusterHistos();
  createTrackHistos(maxTracksPerTF);
  createTrackPairHistos();
  mDet2ElecMapper = o2::mch::raw::createDet2ElecMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = o2::mch::raw::createSolar2FeeLinkMapper<o2::mch::raw::ElectronicMapperGenerated>();
}

void TrackPlotter::createClusterHistos()
{
  mNofClustersPerTrack = createHisto<TH1F>("ClustersPerTrack", "Number of clusters per track;Mean number of clusters per track", 30, 0, 30);
  mNofClustersPerDualSampa = createHisto<TH1F>("ClustersPerDualSampa", "Number of clusters per dual sampa;Number of clusters per DS", 30720, 0, 30719);

  mNofClustersPerChamber = createHisto<TProfile>("ClustersPerChamber", "Clusters per chamber;;Number of clusters", 10, 1, 11);
  setXAxisLabels(mNofClustersPerChamber.get());
  mClusterSizePerChamber = createHisto<TProfile>("ClusterSizePerChamber", "Cluster size per chamber;;Mean number of pads per cluster", 10, 1, 11);
  mClusterBendingSizePerChamber = createHisto<TProfile>("ClusterBendingSizePerChamber", "Cluster size in bending direction (y) per chamber;;Mean number of pads per cluster", 10, 1, 11);
  mClusterNonBendingSizePerChamber = createHisto<TProfile>("ClusterNonBendingSizePerChamber", "Cluster size in non-bending direction (x) per chamber;;Mean number of pads per cluster", 10, 1, 11);
  setXAxisLabels(mClusterSizePerChamber.get());
  setXAxisLabels(mClusterBendingSizePerChamber.get());
  setXAxisLabels(mClusterNonBendingSizePerChamber.get());
}

void TrackPlotter::createTrackHistos(int maxTracksPerTF)
{

  mNofTracksPerTF = createHisto<TH1F>("TracksPerTF", "Number of tracks per TimeFrame;Number of tracks per TF", maxTracksPerTF, 0, maxTracksPerTF, true, "logy");

  mTrackDCA = createHisto<TH1F>("TrackDCA", "Track DCA;DCA (cm)", 500, 0, 500);
  mTrackPDCA = createHisto<TH1F>("TrackPDCA", "Track p#timesDCA;p#timesDCA (GeVcm/c)", 5000, 0, 5000);
  mTrackPt = createHisto<TH1F>("TrackPt", "Track p_{T};p_{T} (GeV/c)", 300, 0, 30, false, "logy");
  mTrackEta = createHisto<TH1F>("TrackEta", "Track #eta;#eta", 200, -4.5, -2);
  mTrackPhi = createHisto<TH1F>("TrackPhi", "Track #phi;#phi (deg)", 360, 0, 360);
  mTrackRAbs = createHisto<TH1F>("TrackRAbs", "Track R_{abs};R_{abs} (cm)", 1000, 0, 100);
  mTrackBC = createHisto<TH1F>("TrackBC", "Track BC;BC", o2::constants::lhc::LHCMaxBunches, 0, o2::constants::lhc::LHCMaxBunches);
  mTrackBCWidth = createHisto<TH1F>("TrackBCWidth", "Track BCWidth;BC Width", 400, 0, 400);
  mTrackChi2OverNDF = createHisto<TH1F>("TrackChi2OverNDF", "Track #chi^{2}/ndf;#chi^{2}/ndf", 500, 0, 50);
}

void TrackPlotter::createTrackPairHistos()
{
  mMinv = createHisto<TH1F>("Minv", "#mu^{+}#mu^{-} invariant mass;M_{#mu^{+}#mu^{-}} (GeV/c^{2})", 300, 0, 6);
}

int TrackPlotter::dsbinx(int deid, int dsid) const
{
  o2::mch::raw::DsDetId det{ deid, dsid };
  auto elec = mDet2ElecMapper(det);
  if (!elec.has_value()) {
    std::cerr << "mapping is wrong somewhere...";
    return -1;
  }
  auto eLinkId = elec->elinkId();
  auto solarId = elec->solarId();
  auto s2f = mSolar2FeeLinkMapper(solarId);
  if (!s2f.has_value()) {
    std::cerr << "mapping is wrong somewhere...";
    return -1;
  }
  auto feeId = s2f->feeId();
  auto linkId = s2f->linkId();

  return computeDsBinX(feeId, linkId, eLinkId);
}

ROOT::Math::PxPyPzMVector getMomentum4D(const o2::mch::TrackParam& trackParam)
{
  double px = trackParam.px();
  double py = trackParam.py();
  double pz = trackParam.pz();
  return { px, py, pz, muonMass };
}

ROOT::Math::PxPyPzMVector getMomentum4D(const o2::mch::TrackMCH& track)
{
  o2::mch::TrackParam trackParamAtVertex(track.getZ(), track.getParameters());
  double vz = 0.0;
  if (!o2::mch::TrackExtrap::extrapToVertex(trackParamAtVertex, 0.0, 0.0, 0.0, 0.0, 0.0)) {
    std::cerr << "Track extrap failed\n";
    return { 0, 0, 0, 0 };
  }
  return getMomentum4D(trackParamAtVertex);
}

namespace
{
struct PadSize {
  double dx, dy;
};
const bool operator<(const PadSize& s1, const PadSize& s2)
{
  if (s1.dx == s2.dx) {
    return s1.dy < s2.dy;
  }
  return s1.dx < s2.dx;
};
} // namespace

void TrackPlotter::fillClusterHistos(gsl::span<const o2::mch::Cluster> clusters,
                                     gsl::span<const o2::mch::Digit> digits)
{

  auto padPosCompare = [](double a, double b) {
    constexpr double smallestPadSize = 0.21;
    if (std::fabs(a - b) < smallestPadSize) {
      return false;
    }
    return a < b;
  };

  for (const auto& cluster : clusters) {
    int deId = cluster.getDEId();
    const auto& seg = o2::mch::mapping::segmentation(deId);
    int b, nb;
    o2::math_utils::Point3D<double> global{ cluster.getX(),
                                            cluster.getY(), cluster.getZ() };
    auto t = mTransformation(deId).Inverse();
    auto local = t(global);

    seg.findPadPairByPosition(local.X(), local.Y(), b, nb);
    if (b >= 0) {
      int dsId = seg.padDualSampaId(b);
      mNofClustersPerDualSampa->Fill(dsbinx(deId, dsId));
    }
    if (nb >= 0) {
      int dsId = seg.padDualSampaId(nb);
      mNofClustersPerDualSampa->Fill(dsbinx(deId, dsId));
    }
    int chamberId = cluster.getChamberId();
    mClusterSizePerChamber->Fill(chamberId + 1, cluster.nDigits);
    std::set<PadSize> padSizes;
    std::set<double, decltype(padPosCompare)> xpos(padPosCompare);
    std::set<double, decltype(padPosCompare)> ypos(padPosCompare);
    for (int dix = cluster.firstDigit; dix < cluster.firstDigit + cluster.nDigits; ++dix) {
      const auto& digit = digits[dix];
      auto padid = digit.getPadID();
      PadSize padSize{ seg.padSizeX(padid),
                       seg.padSizeY(padid) };
      padSizes.emplace(padSize);
      if (seg.isBendingPad(padid)) {
        ypos.emplace(seg.padPositionY(padid));
      } else {
        xpos.emplace(seg.padPositionX(padid));
      }
    }
    mClusterBendingSizePerChamber->Fill(chamberId + 1, ypos.size());
    mClusterNonBendingSizePerChamber->Fill(chamberId + 1, xpos.size());
  }
}

void TrackPlotter::fillTrackPairHistos(gsl::span<const o2::mch::TrackMCH> tracks)
{
  if (tracks.size() > 1) {
    for (auto i = 0; i < tracks.size(); i++) {
      auto ti = getMomentum4D(tracks[i]);
      for (auto j = i + 1; j < tracks.size(); j++) {
        if (tracks[i].getSign() == tracks[j].getSign()) {
          continue;
        }
        auto tj = getMomentum4D(tracks[j]);
        auto p = ti + tj;
        mMinv->Fill(p.M());
      }
    }
  }
}

bool TrackPlotter::fillTrackHistos(const o2::mch::TrackMCH& track,
                                   gsl::span<const o2::mch::Cluster> clusters,
                                   gsl::span<const o2::mch::Digit> digits)
{
  mNofClustersPerTrack->Fill(track.getNClusters());
  mTrackChi2OverNDF->Fill(track.getChi2OverNDF());

  const auto trackClusters = clusters.subspan(track.getFirstClusterIdx(), track.getNClusters());
  fillClusterHistos(trackClusters, digits);

  auto clustersPerChamber = getClustersPerChamber(trackClusters);

  for (auto i = 0; i < clustersPerChamber.size(); i++) {
    mNofClustersPerChamber->Fill(i + 1, clustersPerChamber[i]);
  }

  auto p = track.getP(); // uncorrected p

  o2::mch::TrackParam trackParamAtDCA(track.getZ(), track.getParameters());
  if (!o2::mch::TrackExtrap::extrapToVertexWithoutBranson(trackParamAtDCA, 0.0)) {
    return false;
  }

  o2::mch::TrackParam trackParamAtOrigin(track.getZ(), track.getParameters());
  if (!o2::mch::TrackExtrap::extrapToVertex(trackParamAtOrigin, 0.0, 0.0, 0.0, 0.0, 0.0)) {
    return false;
  }
  double dcaX = trackParamAtDCA.getBendingCoor();
  double dcaY = trackParamAtDCA.getNonBendingCoor();
  double dca = std::sqrt(dcaX * dcaX + dcaY * dcaY);
  mTrackDCA->Fill(dca);
  mTrackPDCA->Fill(p * dca);

  auto muon = getMomentum4D(trackParamAtOrigin);

  mTrackEta->Fill(muon.eta());
  mTrackPhi->Fill(muon.phi() * TMath::RadToDeg() + 180);
  mTrackPt->Fill(muon.pt());

  o2::mch::TrackExtrap::extrapToZ(trackParamAtOrigin, -505.);
  double xAbs = trackParamAtOrigin.getNonBendingCoor();
  double yAbs = trackParamAtOrigin.getBendingCoor();
  auto rAbs = std::sqrt(xAbs * xAbs + yAbs * yAbs);
  mTrackRAbs->Fill(rAbs);

  return true;
}
void TrackPlotter::fillHistograms(gsl::span<const o2::mch::ROFRecord> rofs,
                                  gsl::span<const o2::mch::TrackMCH> tracks,
                                  gsl::span<const o2::mch::Cluster> clusters,
                                  gsl::span<const o2::mch::Digit> digits,
                                  gsl::span<const o2::dataformats::TrackMCHMID> muonTracks)
{

  if (muonTracks.size() == 0) {
    return;
  }

  std::vector<bool> matchedTracks;
  matchedTracks.resize(tracks.size(), false);

  for (const auto& mt : muonTracks) {
    auto ix = mt.getMCHRef().getIndex();
    matchedTracks[ix] = true;
  }
  fill(rofs, tracks, clusters, digits,
       matchedTracks);
}

void TrackPlotter::fillHistograms(gsl::span<const o2::mch::ROFRecord> rofs,
                                  gsl::span<const o2::mch::TrackMCH> tracks,
                                  gsl::span<const o2::mch::Cluster> clusters,
                                  gsl::span<const o2::mch::Digit> digits)
{
  std::vector<bool> selectedTracks;
  selectedTracks.resize(tracks.size(), true);
  fill(rofs, tracks, clusters, digits, selectedTracks);
}

void TrackPlotter::fill(gsl::span<const o2::mch::ROFRecord> rofs,
                        gsl::span<const o2::mch::TrackMCH> tracks,
                        gsl::span<const o2::mch::Cluster> clusters,
                        gsl::span<const o2::mch::Digit> digits,
                        const std::vector<bool>& selectedTracks)
{

  auto ntracks = std::count_if(selectedTracks.begin(), selectedTracks.end(),
                               [](bool v) { return v; });

  mNofTracksPerTF->Fill(ntracks);

  for (const auto& rof : rofs) {
    for (auto ix = rof.getFirstIdx(); ix <= rof.getLastIdx(); ix++) {
      if (selectedTracks[ix]) {
        mTrackBC->Fill(rof.getBCData().bc);
        mTrackBCWidth->Fill(rof.getBCWidth());
      }
    }
  }

  decltype(tracks.size()) nok{ 0 };

  for (auto ix = 0; ix < tracks.size(); ix++) {
    if (selectedTracks[ix]) {
      bool ok = fillTrackHistos(tracks[ix], clusters, digits);
      if (ok) {
        ++nok;
      }
    }
  }

  if (nok != ntracks) {
    std::cerr << "Could only extrapolate " << nok << " tracks over " << ntracks << "\n";
  }

  for (const auto& rof : rofs) {
    if (rof.getNEntries() > 0) {
      std::vector<o2::mch::TrackMCH> rtracks;
      for (auto ix = rof.getFirstIdx(); ix <= rof.getLastIdx(); ix++) {
        if (selectedTracks[ix]) {
          rtracks.emplace_back(tracks[ix]);
        }
      }
      fillTrackPairHistos(rtracks);
    }
  }
}

void TrackPlotter::reset()
{
  for (auto hinfo : mHistograms) {
    hinfo.histo->Reset();
  }
}

} // namespace o2::quality_control_modules::muon
