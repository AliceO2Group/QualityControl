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

#include "MCH/ClustersTask.h"

#include "MCHGlobalMapping/DsIndex.h"
#include "MUONCommon/HistPlotter.h"
#include <DataFormatsMCH/Cluster.h>
#include <DataFormatsMCH/ROFRecord.h>
#include <DataFormatsMCH/TrackMCH.h>
#include <DetectorsBase/GeometryManager.h>
#include <Framework/DataRefUtils.h>
#include <Framework/InputRecord.h>
#include <MCHMappingInterface/Segmentation.h>
#include <TH1F.h>
#include <gsl/span>

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

ClustersTask::ClustersTask()
{
}

ClustersTask::~ClustersTask() = default;

void ClustersTask::createClusterHistos()
{
  mNofClustersPerTrack = std::make_unique<TH1F>("ClustersPerTrack", "Number of clusters per track;Mean number of clusters per track", 30, 0, 30);
  mNofClustersPerDualSampa = std::make_unique<TH1F>("ClustersPerDualSampa", "Number of clusters per dual sampa;Number of clusters per DS", o2::mch::NumberOfDualSampas, 0, o2::mch::NumberOfDualSampas - 1);

  mNofClustersPerChamber = std::make_unique<TProfile>("ClustersPerChamber", "Clusters per chamber;;Number of clusters", 10, 1, 11);
  setXAxisLabels(mNofClustersPerChamber.get());
  mClusterSizePerChamber = std::make_unique<TProfile>("ClusterSizePerChamber", "Cluster size per chamber;;Mean number of pads per cluster", 10, 1, 11);
  setXAxisLabels(mClusterSizePerChamber.get());

  std::string drawOptions;
  std::string displayHints;

  using o2::quality_control_modules::muon::HistPlotter;
  auto& histograms = mHistPlotter.histograms();

  histograms.emplace_back(HistPlotter::HistInfo{ mNofClustersPerTrack.get(), drawOptions, displayHints });
  histograms.emplace_back(HistPlotter::HistInfo{ mNofClustersPerChamber.get(), drawOptions, displayHints });
  histograms.emplace_back(HistPlotter::HistInfo{ mNofClustersPerDualSampa.get(), drawOptions, displayHints });
  histograms.emplace_back(HistPlotter::HistInfo{ mClusterSizePerChamber.get(), drawOptions, displayHints });

  mHistPlotter.publish(getObjectsManager());
}

void ClustersTask::initialize(o2::framework::InitContext& /*ic*/)
{
  ILOG(Debug, Devel) << "initialize ClustersTask" << ENDM;

  createClusterHistos();

  mDet2ElecMapper = o2::mch::raw::createDet2ElecMapper<o2::mch::raw::ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = o2::mch::raw::createSolar2FeeLinkMapper<o2::mch::raw::ElectronicMapperGenerated>();
}

void ClustersTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity : " << activity.mId << ENDM;
}

void ClustersTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void ClustersTask::fillClusterHistos(gsl::span<const o2::mch::Cluster> clusters)
{
  if (mTransformation.get() == nullptr) {
    // should not happen, but better be safe than sorry
    return;
  }

  auto clustersPerChamber = getClustersPerChamber(clusters);

  for (auto i = 0; i < clustersPerChamber.size(); i++) {
    mNofClustersPerChamber->Fill(i + 1, clustersPerChamber[i]);
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
      mNofClustersPerDualSampa->Fill(o2::mch::getDsIndex(o2::mch::raw::DsDetId(deId, dsId)));
    }
    if (nb >= 0) {
      int dsId = seg.padDualSampaId(nb);
      mNofClustersPerDualSampa->Fill(o2::mch::getDsIndex(o2::mch::raw::DsDetId(deId, dsId)));
    }
    int chamberId = cluster.getChamberId();
    mClusterSizePerChamber->Fill(chamberId + 1, cluster.nDigits);
    mNofClustersPerChamber->Fill(chamberId + 1, 1.0);
  }
}

bool ClustersTask::assertInputs(o2::framework::ProcessingContext& ctx)
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

void ClustersTask::monitorData(o2::framework::ProcessingContext& ctx)
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

  decltype(tracks.size()) nok{ 0 };

  for (const auto& track : tracks) {
    const auto trackClusters = clusters.subspan(track.getFirstClusterIdx(), track.getNClusters());
    mNofClustersPerTrack->Fill(trackClusters.size());
    fillClusterHistos(trackClusters);
  }
}

void ClustersTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void ClustersTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ClustersTask::reset()
{
  ILOG(Warning, Support) << "reset" << ENDM;
  mHistPlotter.reset();
}

} // namespace o2::quality_control_modules::muonchambers
