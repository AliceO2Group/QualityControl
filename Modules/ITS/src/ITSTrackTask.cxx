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
/// \file   ITSTrackTask.cxx
/// \author Artem Isakov
///

#include "QualityControl/QcInfoLogger.h"
#include "ITS/ITSTrackTask.h"
#include <DataFormatsITS/TrackITS.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <Framework/InputRecord.h>
#include "ReconstructionDataFormats/Vertex.h"
#include "ReconstructionDataFormats/PrimaryVertex.h"
#include "ITStracking/IOUtils.h"
#include <DataFormatsITSMFT/ClusterTopology.h>
#include "CCDB/BasicCCDBManager.h"
#include "CCDB/CCDBTimeStampUtils.h"
#include "Common/Utils.h"

#include <Framework/DataSpecUtils.h>
#include "ITStracking/Constants.h"

using namespace o2::itsmft;
using namespace o2::its;

namespace o2::quality_control_modules::its
{

ITSTrackTask::ITSTrackTask() : TaskInterface()
{
  // createAllHistos();
}

ITSTrackTask::~ITSTrackTask()
{
  delete hNClusters;
  delete hNClustersReset;
  delete hTrackEta;
  delete hTrackPhi;
  delete hAngularDistribution;
  delete hVertexCoordinates;
  delete hVertexRvsZ;
  delete hVertexZ;
  delete hVertexContributors;
  delete hAssociatedClusterFraction;
  delete hNtracks;
  delete hNtracksReset;
  delete hNClustersPerTrackEta;
  delete hClusterVsBunchCrossing;
  delete hNClusterVsChipITS;
}

void ITSTrackTask::initialize(o2::framework::InitContext& /*ctx*/)
{

  ILOG(Debug, Devel) << "initialize ITSTrackTask" << ENDM;

  mVertexXYsize = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "vertexXYsize", mVertexXYsize);
  mVertexZsize = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "vertexZsize", mVertexZsize);
  mVertexRsize = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "vertexRsize", mVertexRsize);
  mNtracksMAX = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "NtracksMAX", mNtracksMAX);
  mDoTTree = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "doTTree", mDoTTree);
  nBCbins = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "nBCbins", nBCbins);

  createAllHistos();
  publishHistos();

  // NClusterVsChip Full Detector distinguishable
  for (int l = 0; l < NLayer + 1; l++) {
    auto line = new TLine(ChipBoundary[l], 0, ChipBoundary[l], 10);
    hNClusterVsChipITS->GetListOfFunctions()->Add(line);
  }
}

void ITSTrackTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
}

void ITSTrackTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
  isNewCycle = true;
}

void ITSTrackTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  ILOG(Info, Support) << "START DOING QC General" << ENDM;

  if (isNewCycle) {
    hNClustersReset->Reset();
    hNtracksReset->Reset();
    isNewCycle = false;
  }

  if (mTimestamp == -1) { // get dict from ccdb
    mTimestamp = std::stol(o2::quality_control_modules::common::getFromConfig<string>(mCustomParameters, "dicttimestamp", "0"));
    long int ts = mTimestamp ? mTimestamp : ctx.services().get<o2::framework::TimingInfo>().creation;
    ILOG(Info, Support) << "Getting dictionary from ccdb - timestamp: " << ts << ENDM;
    auto& mgr = o2::ccdb::BasicCCDBManager::instance();
    mgr.setTimestamp(ts);
    mDict = mgr.get<o2::itsmft::TopologyDictionary>("ITS/Calib/ClusterDictionary");
    ILOG(Info, Support) << "Dictionary size: " << mDict->getSize() << ENDM;
  }

  auto trackArr = ctx.inputs().get<gsl::span<o2::its::TrackITS>>("tracks");
  auto trackRofArr = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("rofs");
  auto clusRofArr = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("clustersrof");
  auto clusArr = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("compclus");
  auto vertexArr = ctx.inputs().get<gsl::span<o2::dataformats::Vertex<o2::dataformats::TimeStamp<int>>>>("Vertices");
  auto vertexRofArr = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("Verticesrof");

  auto clusIdx = ctx.inputs().get<gsl::span<int>>("clusteridx");
  auto clusPatternArr = ctx.inputs().get<gsl::span<unsigned char>>("patterns");
  auto pattIt = clusPatternArr.begin();

  // multiply angular distributions before re-filling
  hTrackEta->Scale((double)nVertices);
  hTrackPhi->Scale((double)nVertices);
  hAngularDistribution->Scale((double)nVertices);
  hNClustersPerTrackEta->Scale((double)nVertices);

  // Multiply cos(lambda) plot before refilling
  for (int ix = 1; ix <= hNClusterVsChipITS->GetNbinsX(); ix++) {
    double integral = hNClusterVsChipITS->Integral(ix, ix, 0, -1);
    if (integral < 1e-15) {
      continue;
    }
    for (int iy = 1; iy <= hNClusterVsChipITS->GetNbinsY(); iy++) {
      double binc = hNClusterVsChipITS->GetBinContent(ix, iy);
      if (binc < 1e-15) {
        continue;
      }
      hNClusterVsChipITS->SetBinContent(ix, iy, binc * integral);
      hNClusterVsChipITS->SetBinError(ix, iy, std::sqrt(binc * integral));
    }
  }

  std::vector<int> clSize;
  for (const auto& clus : clusArr) {
    auto ClusterID = clus.getPatternID();
    if (ClusterID != o2::itsmft::CompCluster::InvalidPatternID && !mDict->isGroup(ClusterID)) { // Normal (frequent) cluster shapes
      clSize.push_back(mDict->getNpixels(ClusterID));
    } else {
      o2::itsmft::ClusterPattern patt(pattIt);
      clSize.push_back(patt.getNPixels());
    }
  }

  for (const auto& vertex : vertexArr) {
    hVertexCoordinates->Fill(vertex.getX(), vertex.getY());
    hVertexRvsZ->Fill(vertex.getZ(), sqrt(vertex.getX() * vertex.getX() + vertex.getY() * vertex.getY()));
    hVertexZ->Fill(vertex.getZ());
    hVertexContributors->Fill(vertex.getNContributors());
  }

  // loop on vertices per ROF
  for (int iROF = 0; iROF < vertexRofArr.size(); iROF++) {

    int start = vertexRofArr[iROF].getFirstEntry();
    int end = start + vertexRofArr[iROF].getNEntries();
    int nvtxROF = 0;
    int nvtxROF_nocut = vertexRofArr[iROF].getNEntries();
    for (int ivtx = start; ivtx < end; ivtx++) {
      auto& vertex = vertexArr[ivtx];
      if (vertex.getNContributors() > 0) { // TODO: for now no cut on contributors
        nvtxROF++;
      }
      if (vertex.getNContributors() > 2) { // Apply cut for normalization
        nVertices++;
      } else if (nvtxROF_nocut == 1 && vertex.getNContributors() == 2) {
        nVertices++;
      }
    }
    hVerticesRof->Fill(nvtxROF);
  }

  // loop on tracks per ROF
  for (int iROF = 0; iROF < trackRofArr.size(); iROF++) {

    vMap.clear();
    vEta.clear();
    vPhi.clear();

    int nClusterCntTrack = 0;
    int nTracks = trackRofArr[iROF].getNEntries();
    int start = trackRofArr[iROF].getFirstEntry();
    int end = start + trackRofArr[iROF].getNEntries();
    for (int itrack = start; itrack < end; itrack++) {

      auto& track = trackArr[itrack];
      auto out = track.getParamOut();
      Float_t Eta = -log(tan(out.getTheta() / 2.));
      hTrackEta->Fill(Eta);
      hTrackPhi->Fill(out.getPhi());
      hAngularDistribution->Fill(Eta, out.getPhi());
      hNClusters->Fill(track.getNumberOfClusters());
      hNClustersReset->Fill(track.getNumberOfClusters());

      vMap.emplace_back(track.getPattern());
      vEta.emplace_back(Eta);
      vPhi.emplace_back(out.getPhi());

      hNClustersPerTrackEta->Fill(Eta, track.getNumberOfClusters());
      nClusterCntTrack += track.getNumberOfClusters();
      for (int icluster = 0; icluster < track.getNumberOfClusters(); icluster++) {
        const int index = clusIdx[track.getFirstClusterEntry() + icluster];
        auto& cluster = clusArr[index];
        auto ChipID = cluster.getSensorID();

        int layer = 0;
        while (ChipID > ChipBoundary[layer])
          layer++;
        layer--;

        double clusterSizeWithCorrection = (double)clSize[index] * (std::cos(std::atan(out.getTgl())));
        hNClusterVsChipITS->Fill(ChipID + 1, clusterSizeWithCorrection);
      }
    }

    int nTotCls = clusRofArr[iROF].getNEntries();

    float clusterRatio = nTotCls > 0 ? (float)nClusterCntTrack / (float)nTotCls : -1;
    hAssociatedClusterFraction->Fill(clusterRatio);
    hNtracks->Fill(nTracks);
    hNtracksReset->Fill(nTracks);

    const auto bcdata = trackRofArr[iROF].getBCData();
    hClusterVsBunchCrossing->Fill(bcdata.bc, clusterRatio);

    if (mDoTTree)
      tClusterMap->Fill();
  } // end loop on ROFs

  mNTracks += trackArr.size();
  mNClusters += clusArr.size();
  mNRofs += trackRofArr.size();

  if (mNRofs >= NROFOCCUPANCY) {
    mNTracks = 0;
    mNRofs = 0;
    mNClusters = 0;
  }

  // Scale angular distributions by latest number of vertices
  if (nVertices > 0) {
    hAngularDistribution->Scale(1. / (double)nVertices);
    hTrackEta->Scale(1. / (double)nVertices);
    hTrackPhi->Scale(1. / (double)nVertices);
    hNClustersPerTrackEta->Scale(1. / (double)nVertices);
  }

  // Normalize hNClusterVsChipITS to the clusters per chip
  for (int ix = 1; ix <= hNClusterVsChipITS->GetNbinsX(); ix++) {
    double integral = hNClusterVsChipITS->Integral(ix, ix, 0, -1);
    if (integral < 1e-15) {
      continue;
    }
    for (int iy = 1; iy <= hNClusterVsChipITS->GetNbinsY(); iy++) {
      double binc = hNClusterVsChipITS->GetBinContent(ix, iy);
      if (binc < 1e-15) {
        continue;
      }
      double bine = hNClusterVsChipITS->GetBinError(ix, iy);
      hNClusterVsChipITS->SetBinContent(ix, iy, binc / integral);
      hNClusterVsChipITS->SetBinError(ix, iy, (binc / integral) * std::sqrt((bine / binc) * (bine / binc) + (std::sqrt(integral) / integral) * (std::sqrt(integral) / integral)));
    }
  }
}

void ITSTrackTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void ITSTrackTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ITSTrackTask::reset()
{

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  hAngularDistribution->Reset();
  hNClusters->Reset();
  hTrackPhi->Reset();
  hTrackEta->Reset();
  hVerticesRof->Reset();
  nVertices = 0;

  hVertexCoordinates->Reset();
  hVertexRvsZ->Reset();
  hVertexZ->Reset();
  hVertexContributors->Reset();

  hAssociatedClusterFraction->Reset();
  hNtracks->Reset();
  hNClustersPerTrackEta->Reset();
  hClusterVsBunchCrossing->Reset();
  hNClusterVsChipITS->Reset();
}

void ITSTrackTask::createAllHistos()
{
  tClusterMap = new TTree("ClusterMap", "Cluster Map");
  tClusterMap->Branch("bitmap", &vMap);
  tClusterMap->Branch("eta", &vEta);
  tClusterMap->Branch("phi", &vPhi);
  if (mDoTTree)
    addObject(tClusterMap);

  hAngularDistribution = new TH2D("AngularDistribution", "AngularDistribution", 40, -2.0, 2.0, 60, 0, TMath::TwoPi());
  hAngularDistribution->SetBit(TH1::kIsAverage);
  hAngularDistribution->SetTitle("AngularDistribution");
  addObject(hAngularDistribution);
  formatAxes(hAngularDistribution, "#eta", "#phi", 1, 1.10);
  hAngularDistribution->SetStats(0);

  hNClusters = new TH1D("NClusters", "NClusters", 15, -0.5, 14.5);
  hNClusters->SetTitle("hNClusters");
  addObject(hNClusters);
  formatAxes(hNClusters, "Number of clusters per Track", "Counts", 1, 1.10);
  hNClusters->SetStats(0);

  hNClustersReset = new TH1D("NClustersReset", "NClustersReset", 15, -0.5, 14.5);
  hNClustersReset->SetTitle("hNClusters in one cycle");
  addObject(hNClustersReset);
  formatAxes(hNClustersReset, "Number of clusters per Track", "Counts", 1, 1.10);
  hNClustersReset->SetStats(0);

  hTrackEta = new TH1D("EtaDistribution", "EtaDistribution", 40, -2.0, 2.0);
  hTrackEta->SetBit(TH1::kIsAverage);
  hTrackEta->SetTitle("Eta Distribution of tracks / n_vertices with at least 3 contrib");
  hTrackEta->SetMinimum(0);
  addObject(hTrackEta);
  formatAxes(hTrackEta, "#eta", "Counts / n_vertices", 1, 1.10);
  hTrackEta->SetStats(0);

  hTrackPhi = new TH1D("PhiDistribution", "PhiDistribution", 65, -0.1, TMath::TwoPi());
  hTrackPhi->SetBit(TH1::kIsAverage);
  hTrackPhi->SetTitle("Phi Distribution of tracks / n_vertices with at least 3 contrib");
  hTrackPhi->SetMinimum(0);
  addObject(hTrackPhi);
  formatAxes(hTrackPhi, "#phi", "Counts / n_vertices", 1, 1.10);
  hTrackPhi->SetStats(0);

  hVerticesRof = new TH1D("VerticesRof", "VerticesRof", 101, -0.5, 100.5);
  hVerticesRof->SetTitle("Distribution n_vertices / ROF");
  addObject(hVerticesRof);
  formatAxes(hVerticesRof, "vertices / ROF", "Counts", 1, 1.10);

  hVertexCoordinates = new TH2D("VertexCoordinates", "VertexCoordinates", (int)(mVertexXYsize * 2 / 0.01), -1. * mVertexXYsize, mVertexXYsize, (int)(mVertexXYsize * 2 / 0.01), -1 * mVertexXYsize, mVertexXYsize);
  hVertexCoordinates->SetTitle("Coordinates of track vertex");
  addObject(hVertexCoordinates);
  formatAxes(hVertexCoordinates, "X coordinate (cm)", "Y coordinate (cm)", 1, 1.10);
  hVertexCoordinates->SetStats(0);

  hVertexRvsZ = new TH2D("VertexRvsZ", "VertexRvsZ", (int)(mVertexZsize * 2 / 0.01), -mVertexZsize, mVertexZsize, (int)(mVertexRsize / 0.01), 0, mVertexRsize);
  hVertexRvsZ->SetTitle("Distance to primary vertex vs Z");
  addObject(hVertexRvsZ);
  formatAxes(hVertexRvsZ, "Z coordinate (cm)", "R (cm)", 1, 1.10);

  hVertexZ = new TH1D("VertexZ", "VertexZ", (int)(mVertexZsize * 2 / 0.01), -mVertexZsize, mVertexZsize);
  hVertexZ->SetTitle("Z coordinate of vertex");
  addObject(hVertexZ);
  formatAxes(hVertexZ, "Z coordinate (cm)", "counts", 1, 1.10);
  hVertexZ->SetStats(0);

  hVertexContributors = new TH1D("NVertexContributors", "NVertexContributors", 500, 0, 500);
  hVertexContributors->SetTitle("NVertexContributors");
  addObject(hVertexContributors);
  formatAxes(hVertexContributors, "Number of contributors for vertex", "Counts", 1, 1.10);
  hVertexContributors->SetStats(0);

  hAssociatedClusterFraction = new TH1D("AssociatedClusterFraction", "AssociatedClusterFraction", 100, 0, 1);
  hAssociatedClusterFraction->SetTitle("The fraction of clusters into tracks event by event");
  addObject(hAssociatedClusterFraction);
  formatAxes(hAssociatedClusterFraction, "Clusters in tracks / All clusters", "Counts", 1, 1.10);
  hAssociatedClusterFraction->SetStats(0);

  hNtracks = new TH1D("Ntracks", "Ntracks", (int)mNtracksMAX, 0, mNtracksMAX);
  hNtracks->SetTitle("The number of tracks event by event");
  addObject(hNtracks);
  formatAxes(hNtracks, "# tracks", "Counts", 1, 1.10);
  hNtracks->SetStats(0);

  hNtracksReset = new TH1D("NtracksReset", "NtracksReset", (int)mNtracksMAX, 0, mNtracksMAX);
  hNtracksReset->SetTitle("The number of tracks event by event for last QC cycle");
  addObject(hNtracksReset);
  formatAxes(hNtracksReset, "# tracks", "Counts", 1, 1.10);
  hNtracksReset->SetStats(0);

  hNClustersPerTrackEta = new TH2D("NClustersPerTrackEta", "NClustersPerTrackEta", 400, -2.0, 2.0, 15, -0.5, 14.5);
  hNClustersPerTrackEta->SetBit(TH1::kIsAverage);
  hNClustersPerTrackEta->SetTitle("Eta vs NClusters Per Track");
  addObject(hNClustersPerTrackEta);
  formatAxes(hNClustersPerTrackEta, "#eta", "# of Clusters per Track", 1, 1.10);
  hNClustersPerTrackEta->SetStats(0);

  hClusterVsBunchCrossing = new TH2D("BunchCrossingIDvsClusterRatio", "BunchCrossingIDvsClusterRatio", nBCbins, 0, 4095, 100, 0, 1);
  hClusterVsBunchCrossing->SetTitle("Bunch Crossing ID vs Cluster Ratio");
  addObject(hClusterVsBunchCrossing);
  formatAxes(hClusterVsBunchCrossing, "Bunch Crossing ID", "Fraction of clusters in tracks", 1, 1.10);
  hClusterVsBunchCrossing->SetStats(0);

  for (int i = 0; i < 2125; i++) {
    if (i <= 432) {
      mChipBins[i] = i + 1;
    } else {
      mChipBins[i] = i + 1 + 13 * (i - 432);
    }
  }
  mCoslBins[0] = 0;
  for (int i = 1; i < 25; i++) {
    if (mCoslBins[i - 1] + 0.5 < 6.05) {
      mCoslBins[i] = mCoslBins[i - 1] + 0.5;
    } else {
      mCoslBins[i] = mCoslBins[i - 1] + 0.75;
    }
  }

  hNClusterVsChipITS = new TH2D(Form("NClusterVsChipITS"), Form("NClusterVsChipITS"), 2124, mChipBins, 24, mCoslBins);
  hNClusterVsChipITS->SetTitle(Form("Corrected cluster size for track clusters vs Chip Full Detector"));
  hNClusterVsChipITS->SetBit(TH1::kIsAverage);
  addObject(hNClusterVsChipITS);
  formatAxes(hNClusterVsChipITS, "ChipID + 1", "(Cluster size x cos(#lambda)) / n_clusters", 1, 1.10);
  hNClusterVsChipITS->SetStats(0);
  hNClusterVsChipITS->SetMaximum(1);

  // NClusterVsChip Full Detector distinguishable
  for (int l = 0; l < NLayer + 1; l++) {
    auto line = new TLine(ChipBoundary[l], 0, ChipBoundary[l], 15);
    hNClusterVsChipITS->GetListOfFunctions()->Add(line);
  }
}

void ITSTrackTask::addObject(TObject* aObject)
{
  if (!aObject) {
    ILOG(Info, Support) << " ERROR: trying to add non-existent object " << ENDM;
    return;
  } else {
    mPublishedObjects.push_back(aObject);
  }
}

void ITSTrackTask::formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset, float yOffset)
{
  h->GetXaxis()->SetTitle(xTitle);
  h->GetYaxis()->SetTitle(yTitle);
  h->GetXaxis()->SetTitleOffset(xOffset);
  h->GetYaxis()->SetTitleOffset(yOffset);
}

void ITSTrackTask::publishHistos()
{
  for (unsigned int iObj = 0; iObj < mPublishedObjects.size(); iObj++) {
    getObjectsManager()->startPublishing(mPublishedObjects.at(iObj));
    ILOG(Info, Support) << " Object will be published: " << mPublishedObjects.at(iObj)->GetName() << ENDM;
  }
}

} // namespace o2::quality_control_modules::its
