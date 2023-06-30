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

#include "MCH/TracksTask.h"

#include "CommonConstants/LHCConstants.h"
#include <DataFormatsMCH/Cluster.h>
#include <DataFormatsMCH/ROFRecord.h>
#include <DataFormatsMCH/TrackMCH.h>
#include <DetectorsBase/GeometryManager.h>
#include <Framework/DataRefUtils.h>
#include <Framework/InputRecord.h>
#include <MCHMappingInterface/Segmentation.h>
#include <MCHTracking/TrackExtrap.h>
#include <MCHTracking/TrackParam.h>
#include <Math/Vector4Dfwd.h>
#include <TH1F.h>
#include <Math/Vector4D.h>
#include <TMath.h>
#include <gsl/span>

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

namespace o2::quality_control_modules::muonchambers
{

TracksTask::TracksTask()
{
}

TracksTask::~TracksTask() = default;

void TracksTask::createClusterHistos()
{
  mNofClustersPerTrack = createHisto<TH1F>("ClustersPerTrack", "Number of clusters per track;Mean number of clusters per track", 30, 0, 30);
  mNofClustersPerDualSampa = createHisto<TH1F>("ClustersPerDualSampa", "Number of clusters per dual sampa;Number of clusters per DS", 30720, 0, 30719);

  mNofClustersPerChamber = createHisto<TProfile>("ClustersPerChamber", "Clusters per chamber;;Number of clusters", 10, 1, 11);
  setXAxisLabels(mNofClustersPerChamber.get());
  mClusterSizePerChamber = createHisto<TProfile>("ClusterSizePerChamber", "Cluster size per chamber;;Mean number of pads per cluster", 10, 1, 11);
  setXAxisLabels(mClusterSizePerChamber.get());
}

void TracksTask::createTrackHistos()
{
  double maxTracksPerTF = 400;

  if (auto param = mCustomParameters.find("maxTracksPerTF"); param != mCustomParameters.end()) {
    maxTracksPerTF = std::stof(param->second);
  }

  mNofTracksPerTF = createHisto<TH1F>("TracksPerTF", "Number of tracks per TimeFrame;Number of tracks per TF", maxTracksPerTF, 0, maxTracksPerTF, true, "logy");

  mTrackDCA = createHisto<TH1F>("TrackDCA", "Track DCA;DCA (cm)", 500, 0, 500);
  mTrackPDCA = createHisto<TH1F>("TrackPDCA", "Track p#timesDCA;p#timesDCA (GeVcm/c)", 5000, 0, 5000);
  mTrackPt = createHisto<TH1F>("TrackPt", "Track p_{T};p_{T} (GeV/c)", 300, 0, 30, false, "logy");
  mTrackEta = createHisto<TH1F>("TrackEta", "Track #eta;#eta", 200, -4.5, -2);
  mTrackPhi = createHisto<TH1F>("TrackPhi", "Track #phi;#phi (deg)", 360, 0, 360);
  mTrackRAbs = createHisto<TH1F>("TrackRAbs", "Track R_{abs};R_{abs} (cm)", 1000, 0, 100);
  mTrackBC = createHisto<TH1F>("TrackBC", "Track BC;BC", o2::constants::lhc::LHCMaxBunches, 0, o2::constants::lhc::LHCMaxBunches - 1);
  mTrackChi2OverNDF = createHisto<TH1F>("TrackChi2OverNDF", "Track #chi^{2}/ndf;#chi^{2}/ndf", 500, 0, 50);
}

void TracksTask::createTrackPairHistos()
{
  mMinv = createHisto<TH1F>("Minv", "#mu^{+}#mu^{-} invariant mass;M_{#mu^{+}#mu^{-}} (GeV/c^{2})", 300, 0, 6);
}

void TracksTask::initialize(o2::framework::InitContext& /*ic*/)
{
  ILOG(Debug, Devel) << "initialize TracksTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  createTrackHistos();
  createClusterHistos();
  createTrackPairHistos();

  mDet2ElecMapper = o2::mch::raw::createDet2ElecMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = o2::mch::raw::createSolar2FeeLinkMapper<o2::mch::raw::ElectronicMapperGenerated>();
}

int TracksTask::dsbinx(int deid, int dsid) const
{
  o2::mch::raw::DsDetId det{ deid, dsid };
  auto elec = mDet2ElecMapper(det);
  if (!elec.has_value()) {
    ILOGE << "mapping is wrong somewhere...";
    return -1;
  }
  auto eLinkId = elec->elinkId();
  auto solarId = elec->solarId();
  auto s2f = mSolar2FeeLinkMapper(solarId);
  if (!s2f.has_value()) {
    ILOGE << "mapping is wrong somewhere...";
    return -1;
  }
  auto feeId = s2f->feeId();
  auto linkId = s2f->linkId();

  return computeDsBinX(feeId, linkId, eLinkId);
}

void TracksTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity : " << activity.mId << ENDM;
}

void TracksTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
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
    ILOG(Error, Support) << "Track extrap failed" << ENDM;
    return { 0, 0, 0, 0 };
  }
  return getMomentum4D(trackParamAtVertex);
}

void TracksTask::fillClusterHistos(gsl::span<const o2::mch::Cluster> clusters)
{
  if (mTransformation.get() == nullptr) {
    // should not happen, but better be safe than sorry
    return;
  }
  for (const auto& cluster : clusters) {
    int deId = cluster.getDEId();
    const o2::mch::mapping::Segmentation& seg = o2::mch::mapping::segmentation(deId);
    int b, nb;

    o2::math_utils::Point3D<double> global{ cluster.getX(),
                                            cluster.getY(), cluster.getZ() };
    auto t = (*mTransformation)(deId).Inverse();
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
  }
}

bool TracksTask::assertInputs(o2::framework::ProcessingContext& ctx)
{
  if (!ctx.inputs().isValid("tracks")) {
    ILOG(Info, Support) << "no mch tracks available on input" << ENDM;
    return false;
  }
  if (!ctx.inputs().isValid("trackrofs")) {
    ILOG(Info, Support) << "no mch track rofs available on input" << ENDM;
    return false;
  }
  if (!ctx.inputs().isValid("trackclusters")) {
    ILOG(Info, Support) << "no mch track clusters available on input" << ENDM;
    return false;
  }
  return true;
}

void TracksTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  if (!assertInputs(ctx)) {
    return;
  }

  if (mTransformation.get() == nullptr && gGeoManager != nullptr) {
    ILOG(Info, Support) << "get transformation from gGeoManager" << ENDM;
    mTransformation = std::make_unique<o2::mch::geo::TransformationCreator>(o2::mch::geo::transformationFromTGeoManager(*gGeoManager));
  }

  auto tracks = ctx.inputs().get<gsl::span<o2::mch::TrackMCH>>("tracks");
  auto rofs = ctx.inputs().get<gsl::span<o2::mch::ROFRecord>>("trackrofs");
  auto clusters = ctx.inputs().get<gsl::span<o2::mch::Cluster>>("trackclusters");

  mNofTracksPerTF->Fill(tracks.size());

  for (const auto& rof : rofs) {
    if (rof.getNEntries() > 0) {
      mTrackBC->Fill(rof.getBCData().bc);
    }
  }

  decltype(tracks.size()) nok{ 0 };

  for (const auto& track : tracks) {
    bool ok = fillTrackHistos(track, clusters);
    if (ok) {
      ++nok;
    }
  }

  if (nok != tracks.size()) {
    ILOG(Warning, Support) << "Could only extrapolate " << nok << " tracks over " << tracks.size() << ENDM;
  }

  for (const auto& rof : rofs) {
    if (rof.getNEntries() > 0) {
      auto rtracks = tracks.subspan(rof.getFirstIdx(), rof.getNEntries());
      fillTrackPairHistos(rtracks);
    }
  }
}

void TracksTask::fillTrackPairHistos(gsl::span<const o2::mch::TrackMCH> tracks)
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

bool TracksTask::fillTrackHistos(const o2::mch::TrackMCH& track,
                                 gsl::span<const o2::mch::Cluster> clusters)
{

  mNofClustersPerTrack->Fill(track.getNClusters());
  mTrackChi2OverNDF->Fill(track.getChi2OverNDF());

  const auto trackClusters = clusters.subspan(track.getFirstClusterIdx(), track.getNClusters());
  fillClusterHistos(trackClusters);

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

  o2::mch::TrackParam trackParamAtAbsEnd(track.getZ(), track.getParameters());
  o2::mch::TrackExtrap::extrapToZ(trackParamAtAbsEnd, -505.);
  double xAbs = trackParamAtAbsEnd.getNonBendingCoor();
  double yAbs = trackParamAtAbsEnd.getBendingCoor();
  auto rAbs = std::sqrt(xAbs * xAbs + yAbs * yAbs);
  mTrackRAbs->Fill(rAbs);

  return true;
}

void TracksTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void TracksTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void TracksTask::reset()
{
  ILOG(Warning, Support) << "resetting the histograms is not implemented" << ENDM;
}

} // namespace o2::quality_control_modules::muonchambers
