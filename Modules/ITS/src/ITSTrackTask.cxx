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
#include "Common/Utils.h"

#include <Framework/DataSpecUtils.h>
#include "ITStracking/Constants.h"

#include "DCAFitter/DCAFitterN.h"

using namespace o2::itsmft;
using namespace o2::its;

namespace o2::quality_control_modules::its
{

ITSTrackTask::ITSTrackTask() : TaskInterface()
{
  // createAllHistos();
}

ITSTrackTask::~ITSTrackTask() // make_shared objects will be delete automatically
{
  delete hVertexCoordinates;
  delete hVertexRvsZ;
  delete hVertexZ;
  delete hVertexContributors;
  delete hAssociatedClusterFraction;
  delete hNtracks;
  delete hTrackPtVsEta;
  delete hTrackPtVsPhi;
}

void ITSTrackTask::initialize(o2::framework::InitContext& /*ctx*/)
{

  ILOG(Debug, Devel) << "initialize ITSTrackTask" << ENDM;

  mVertexXYsize = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "vertexXYsize", mVertexXYsize);
  mVertexZsize = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "vertexZsize", mVertexZsize);
  mVertexRsize = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "vertexRsize", mVertexRsize);
  mNtracksMAX = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "NtracksMAX", mNtracksMAX);
  nBCbins = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "nBCbins", nBCbins);
  mDoNorm = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "doNorm", mDoNorm);
  mInvMasses = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "InvMasses", mInvMasses);
  mPublishMore = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "publishMore", mPublishMore);

  // pt bins definition: 20 MeV/c width up to 1 GeV/c, 100 MeV/c afterwards
  ptBins[0] = 0.;
  for (int i = 1; i < 141; i++) {
    ptBins[i] = i <= 50 ? ptBins[i - 1] + 0.02 : ptBins[i - 1] + 0.1;
  }

  createAllHistos();
  publishHistos();
}

void ITSTrackTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
}

void ITSTrackTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void ITSTrackTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  ILOG(Debug, Devel) << "START DOING QC General" << ENDM;

  if (mTimestamp == -1) { // get dict from ccdb
    mTimestamp = std::stol(o2::quality_control_modules::common::getFromConfig<string>(mCustomParameters, "dicttimestamp", "0"));
    long int ts = mTimestamp ? mTimestamp : ctx.services().get<o2::framework::TimingInfo>().creation;
    ILOG(Debug, Devel) << "Getting dictionary from ccdb - timestamp: " << ts << ENDM;

    std::map<std::string, std::string> metadata;
    mDict = TaskInterface::retrieveConditionAny<o2::itsmft::TopologyDictionary>("ITS/Calib/ClusterDictionary", metadata, ts);
    ILOG(Debug, Devel) << "Dictionary size: " << mDict->getSize() << ENDM;
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

  // Multiply cos(lambda) plot before refilling
  if (mPublishMore) {
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

  // DCAFitter2 class initialization  for the v0 part
  using Vec3D = ROOT::Math::SVector<double, 3>; // this is a type of the fitted vertex
  o2::vertexing::DCAFitter2 ft;
  ft.setBz(5.0);
  ft.setPropagateToPCA(true);
  ft.setMaxR(30);
  ft.setMaxDZIni(0.1);
  ft.setMaxDXYIni(0.1);
  ft.setMinParamChange(1e-3);
  ft.setMinRelChi2Change(0.9);
  ft.setMaxChi2(10);
  // prepare variables for v0
  float vx = 0, vy = 0, vz = 0;
  float dca[2]{ 0., 0. };
  float bz = 5.0;

  // loop on tracks per ROF
  for (int iROF = 0; iROF < trackRofArr.size(); iROF++) {

    int nClusterCntTrack = 0;
    int nTracks = trackRofArr[iROF].getNEntries();
    int start = trackRofArr[iROF].getFirstEntry();
    int end = start + trackRofArr[iROF].getNEntries();
    for (int itrack = start; itrack < end; itrack++) {

      auto& track = trackArr[itrack];
      auto out = track.getParamOut();
      Float_t Eta = -log(tan(out.getTheta() / 2.));
      hTrackEta->getNum()->Fill(Eta);
      hTrackPhi->getNum()->Fill(out.getPhi());
      hAngularDistribution->getNum()->Fill(Eta, out.getPhi());
      hNClusters->getNum()->Fill(track.getNumberOfClusters());

      hTrackPtVsEta->Fill(out.getPt(), Eta);
      hTrackPtVsPhi->Fill(out.getPt(), out.getPhi());

      hNClustersPerTrackEta->getNum()->Fill(Eta, track.getNumberOfClusters());
      hNClustersPerTrackPhi->getNum()->Fill(out.getPhi(), track.getNumberOfClusters());
      hNClustersPerTrackPt->getNum()->Fill(out.getPt(), track.getNumberOfClusters());

      for (int iLayer = 0; iLayer < NLayer; iLayer++) {
        if (track.getPattern() & (0x1 << iLayer)) { // check first layer (from inside) on which there is a hit
          hHitFirstLayerPhiAll->getNum()->Fill(out.getPhi(), iLayer);
          if (track.getNumberOfClusters() == 4) {
            hHitFirstLayerPhi4cls->getNum()->Fill(out.getPhi(), iLayer);
          } else if (track.getNumberOfClusters() == 5) {
            hHitFirstLayerPhi5cls->getNum()->Fill(out.getPhi(), iLayer);
          } else if (track.getNumberOfClusters() == 6) {
            hHitFirstLayerPhi6cls->getNum()->Fill(out.getPhi(), iLayer);
          } else if (track.getNumberOfClusters() == 7) {
            hHitFirstLayerPhi7cls->getNum()->Fill(out.getPhi(), iLayer);
          }
          break;
        }
      }
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
        if (mPublishMore) {
          hNClusterVsChipITS->Fill(ChipID + 1, clusterSizeWithCorrection);
        }
      }

      // Find V0s
      if (mInvMasses == 1) {
        if (track.getSign() < 0) // choose only positive tracks
          continue;
        track.getImpactParams(vx, vy, vz, bz, dca);
        if ((track.getNumberOfClusters() < 6) || (abs(track.getTgl()) > 1.5) || (abs(dca[0]) < 0.06)) // conditions for the track acceptance
          continue;

        for (int intrack = start; intrack < end; intrack++) { // goes through the tracks one more time
          auto& ntrack = trackArr[intrack];
          if (ntrack.getSign() > 0) // choose only negative tracks
            continue;
          ntrack.getImpactParams(vx, vy, vz, bz, dca);
          if ((ntrack.getNumberOfClusters() < 6) || (abs(ntrack.getTgl()) > 1.5) || (abs(dca[0]) < 0.06)) // conditions for the track acceptance
            continue;

          int nc = 0;
          try {
            nc = ft.process(track, ntrack);
          } catch (...) {
            continue;
          }
          if (nc == 0)
            continue;

          int ibest = 0;
          float bestChi2 = 1e7;
          for (int i = 0; i < nc; i++) {
            auto chi2 = ft.getChi2AtPCACandidate(i);
            if (chi2 > bestChi2)
              continue;
            bestChi2 = chi2;
            ibest = i;
          }
          // conditions for v0
          auto vtx = ft.getPCACandidate(ibest);
          auto x = vtx[0] + 0.02985;
          auto y = vtx[1] + 0.01949;
          auto r = sqrt(x * x + y * y);
          if ((r < 0.5) || (r > 3.5))
            continue;

          const auto& t0 = ft.getTrack(0, ibest); // Positive daughter track
          const auto& t1 = ft.getTrack(1, ibest); // Negative daughter track

          auto r0 = t0.getXYZGlo();
          auto r1 = t1.getXYZGlo();
          auto dx = r0.X() - r1.X();
          auto dy = r0.Y() - r1.Y();
          auto dz = r0.Z() - r1.Z();
          auto d = sqrt(dx * dx + dy * dy + dz * dz);
          if (d > 0.02)
            continue;
          std::array<float, 3> p0; // Positive daughter momentum
          t0.getPxPyPzGlo(p0);
          std::array<float, 3> p1; // Negative daughter momentum
          t1.getPxPyPzGlo(p1);
          std::array<float, 3> v0p; // V0 particle momentum
          v0p = { p0[0] + p1[0], p0[1] + p1[1], p0[2] + p1[2] };

          // Strangness inv mass calculation
          auto pV0 = sqrt(v0p[0] * v0p[0] + v0p[1] * v0p[1] + v0p[2] * v0p[2]); // Particle momentum
          auto p2DaughterPos = p0[0] * p0[0] + p0[1] * p0[1] + p0[2] * p0[2];   // Positive daughter momentum
          auto p2DaughterNeg = p1[0] * p1[0] + p1[1] * p1[1] + p1[2] * p1[2];   // Negative daughter momentum
                                                                                // K0s
          auto enDaughterPos = sqrt(mPiInvMass * mPiInvMass + p2DaughterPos);   // Positive daughter energy
          auto enDaughterNeg = sqrt(mPiInvMass * mPiInvMass + p2DaughterNeg);   // Negative daughter energy
          auto enV0 = enDaughterPos + enDaughterNeg;
          auto K0sInvMass = sqrt(enV0 * enV0 - pV0 * pV0);
          hInvMassK0s->Fill(K0sInvMass);
          // Lambda
          enDaughterPos = sqrt(mProtonInvMass * mProtonInvMass + p2DaughterPos); // Positive daughter energy
          enDaughterNeg = sqrt(mPiInvMass * mPiInvMass + p2DaughterNeg);         // Negative daughter energy
          enV0 = enDaughterPos + enDaughterNeg;
          auto LambdaInvMass = sqrt(enV0 * enV0 - pV0 * pV0);
          hInvMassLambda->Fill(LambdaInvMass);
          // LambdaBar
          enDaughterPos = sqrt(mPiInvMass * mPiInvMass + p2DaughterPos);         // Positive daughter energy
          enDaughterNeg = sqrt(mProtonInvMass * mProtonInvMass + p2DaughterNeg); // Negative daughter energy
          enV0 = enDaughterPos + enDaughterNeg;
          auto LambdaBarInvMass = sqrt(enV0 * enV0 - pV0 * pV0);
          hInvMassLambdaBar->Fill(LambdaBarInvMass);
        }
      }
    }

    int nTotCls = clusRofArr[iROF].getNEntries();

    float clusterRatio = nTotCls > 0 ? (float)nClusterCntTrack / (float)nTotCls : -1;
    hAssociatedClusterFraction->Fill(clusterRatio);
    hNtracks->Fill(nTracks);

    const auto bcdata = trackRofArr[iROF].getBCData();
    hClusterVsBunchCrossing->Fill(bcdata.bc, clusterRatio);

  } // end loop on ROFs

  mNRofs += trackRofArr.size();

  // Scale angular distributions by latest number of vertices
  Double_t normalization = 1.0;

  if (mDoNorm == 1 && nVertices > 0)
    normalization = 1. * nVertices;
  else if (mDoNorm == 2)
    normalization = 1. * mNRofs;

  hAngularDistribution->getDen()->SetBinContent(1, 1, normalization);
  hTrackEta->getDen()->SetBinContent(1, normalization);
  hTrackPhi->getDen()->SetBinContent(1, normalization);
  hNClusters->getDen()->SetBinContent(1, normalization);
  hNClustersPerTrackEta->getDen()->SetBinContent(1, 1, normalization);
  hNClustersPerTrackPhi->getDen()->SetBinContent(1, 1, normalization);
  hNClustersPerTrackPt->getDen()->SetBinContent(1, 1, normalization);
  hHitFirstLayerPhiAll->getDen()->SetBinContent(1, 1, normalization);
  hHitFirstLayerPhi4cls->getDen()->SetBinContent(1, 1, normalization);
  hHitFirstLayerPhi5cls->getDen()->SetBinContent(1, 1, normalization);
  hHitFirstLayerPhi6cls->getDen()->SetBinContent(1, 1, normalization);
  hHitFirstLayerPhi7cls->getDen()->SetBinContent(1, 1, normalization);
  if (mPublishMore) {
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
}

void ITSTrackTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;

  hAngularDistribution->update();
  hTrackEta->update();
  hTrackPhi->update();
  hNClusters->update();
  hNClustersPerTrackEta->update();
  hNClustersPerTrackPhi->update();
  hNClustersPerTrackPt->update();
  hHitFirstLayerPhiAll->update();
  hHitFirstLayerPhi4cls->update();
  hHitFirstLayerPhi5cls->update();
  hHitFirstLayerPhi6cls->update();
  hHitFirstLayerPhi7cls->update();
}

void ITSTrackTask::endOfActivity(const Activity& /*activity*/)
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
  mNRofs = 0;
  hVertexCoordinates->Reset();
  hVertexRvsZ->Reset();
  hVertexZ->Reset();
  hVertexContributors->Reset();

  hAssociatedClusterFraction->Reset();
  hNtracks->Reset();
  hNClustersPerTrackEta->Reset();
  hNClustersPerTrackPhi->Reset();
  hNClustersPerTrackPt->Reset();
  hHitFirstLayerPhiAll->Reset();
  hHitFirstLayerPhi4cls->Reset();
  hHitFirstLayerPhi5cls->Reset();
  hHitFirstLayerPhi6cls->Reset();
  hHitFirstLayerPhi7cls->Reset();
  hClusterVsBunchCrossing->Reset();
  if (mPublishMore) {
    hNClusterVsChipITS->Reset();
  }

  hInvMassK0s->Reset();
  hInvMassLambda->Reset();
  hInvMassLambdaBar->Reset();

  hTrackPtVsEta->Reset();
  hTrackPtVsPhi->Reset();
}

void ITSTrackTask::createAllHistos()
{
  std::string titleNorm = mDoNorm == 1 ? "/ n_vertices" : mDoNorm == 2 ? "/ n_rofs"
                                                                       : "";
  hAngularDistribution = std::make_unique<TH2FRatio>("AngularDistribution", "AngularDistribution", 40, -2.0, 2.0, 60, 0, TMath::TwoPi(), true);
  hAngularDistribution->SetTitle("AngularDistribution");
  addObject(hAngularDistribution.get());
  formatAxes(hAngularDistribution.get(), "#eta", "#phi", 1, 1.10);
  hAngularDistribution->SetStats(0);

  hNClusters = std::make_unique<TH1FRatio>("NClusters", "NClusters", 15, -0.5, 14.5, true);
  hNClusters->SetTitle("hNClusters");
  addObject(hNClusters.get());
  formatAxes(hNClusters.get(), "Number of clusters per Track", Form("Counts %s", titleNorm.c_str()), 1, 1.10);
  hNClusters->SetStats(0);
  hNClusters->SetOption("HIST");

  hTrackEta = std::make_unique<TH1FRatio>("EtaDistribution", "EtaDistribution", 40, -2.0, 2.0, true);
  hTrackEta->SetTitle(Form("Eta Distribution of tracks %s ", titleNorm.c_str()));
  hTrackEta->SetMinimum(0);
  addObject(hTrackEta.get());
  formatAxes(hTrackEta.get(), "#eta", Form("Counts %s", titleNorm.c_str()), 1, 1.10);
  hTrackEta->SetStats(0);

  hTrackPhi = std::make_unique<TH1FRatio>("PhiDistribution", "PhiDistribution", 65, -0.1, TMath::TwoPi(), true);
  hTrackPhi->SetTitle(Form("Phi Distribution of tracks %s", titleNorm.c_str()));
  hTrackPhi->SetMinimum(0);
  addObject(hTrackPhi.get());
  formatAxes(hTrackPhi.get(), "#phi", Form("Counts %s", titleNorm.c_str()), 1, 1.10);
  hTrackPhi->SetStats(0);

  hVerticesRof = new TH1F("VerticesRof", "VerticesRof", 101, -0.5, 100.5);
  hVerticesRof->SetTitle("Distribution n_vertices / ROF");
  addObject(hVerticesRof);
  formatAxes(hVerticesRof, "vertices / ROF", "Counts", 1, 1.10);

  hVertexCoordinates = new TH2F("VertexCoordinates", "VertexCoordinates", (int)(mVertexXYsize * 2 / 0.01), -1. * mVertexXYsize, mVertexXYsize, (int)(mVertexXYsize * 2 / 0.01), -1 * mVertexXYsize, mVertexXYsize);
  hVertexCoordinates->SetTitle("Coordinates of track vertex");
  addObject(hVertexCoordinates);
  formatAxes(hVertexCoordinates, "X coordinate (cm)", "Y coordinate (cm)", 1, 1.10);
  hVertexCoordinates->SetStats(0);

  hVertexRvsZ = new TH2F("VertexRvsZ", "VertexRvsZ", (int)(mVertexZsize * 2 / 0.01), -mVertexZsize, mVertexZsize, (int)(mVertexRsize / 0.01), 0, mVertexRsize);
  hVertexRvsZ->SetTitle("Distance to primary vertex vs Z");
  addObject(hVertexRvsZ);
  formatAxes(hVertexRvsZ, "Z coordinate (cm)", "R (cm)", 1, 1.10);

  hVertexZ = new TH1F("VertexZ", "VertexZ", (int)(mVertexZsize * 2 / 0.01), -mVertexZsize, mVertexZsize);
  hVertexZ->SetTitle("Z coordinate of vertex");
  addObject(hVertexZ);
  formatAxes(hVertexZ, "Z coordinate (cm)", "counts", 1, 1.10);
  hVertexZ->SetStats(0);

  hVertexContributors = new TH1F("NVertexContributors", "NVertexContributors", 500, 0, 500);
  hVertexContributors->SetTitle("NVertexContributors");
  addObject(hVertexContributors);
  formatAxes(hVertexContributors, "Number of contributors for vertex", "Counts", 1, 1.10);
  hVertexContributors->SetStats(0);

  hAssociatedClusterFraction = new TH1F("AssociatedClusterFraction", "AssociatedClusterFraction", 100, 0, 1);
  hAssociatedClusterFraction->SetTitle("The fraction of clusters into tracks event by event");
  addObject(hAssociatedClusterFraction);
  formatAxes(hAssociatedClusterFraction, "Clusters in tracks / All clusters", "Counts", 1, 1.10);
  hAssociatedClusterFraction->SetStats(0);

  hNtracks = new TH1F("Ntracks", "Ntracks", (int)mNtracksMAX, 0, mNtracksMAX);
  hNtracks->SetTitle("The number of tracks event by event");
  addObject(hNtracks);
  formatAxes(hNtracks, "# tracks", "Counts", 1, 1.10);
  hNtracks->SetStats(0);

  hNClustersPerTrackEta = std::make_unique<TH2FRatio>("NClustersPerTrackEta", "NClustersPerTrackEta", 400, -2.0, 2.0, 15, -0.5, 14.5, true);
  hNClustersPerTrackEta->SetTitle("Eta vs NClusters Per Track");
  addObject(hNClustersPerTrackEta.get());
  formatAxes(hNClustersPerTrackEta.get(), "#eta", "# of Clusters per Track", 1, 1.10);
  hNClustersPerTrackEta->SetStats(0);

  hNClustersPerTrackPhi = std::make_unique<TH2FRatio>("NClustersPerTrackPhi", "NClustersPerTrackPhi", 65, 0, TMath::TwoPi(), 15, -0.5, 14.5, true);
  hNClustersPerTrackPhi->SetTitle("Phi vs NClusters Per Track");
  addObject(hNClustersPerTrackPhi.get());
  formatAxes(hNClustersPerTrackPhi.get(), "#phi", "# of Clusters per Track", 1, 1.10);
  hNClustersPerTrackPhi->SetStats(0);

  hNClustersPerTrackPt = std::make_unique<TH2FRatio>("NClustersPerTrackPt", "NClustersPerTrackPt", 150, 0, 10, 15, -0.5, 14.5, true);
  hNClustersPerTrackPt->SetTitle("#it{p_{T}} vs NClusters Per Track");
  addObject(hNClustersPerTrackPt.get());
  formatAxes(hNClustersPerTrackPt.get(), "#it{p_{T}} (GeV/c)", "# of Clusters per Track", 1, 1.10);
  hNClustersPerTrackPt->SetStats(0);

  hHitFirstLayerPhiAll = std::make_unique<TH2FRatio>("HitFirstLayerAll", "HitFirstLayerPhiAll", 65, -0.1, TMath::TwoPi(), 4, -0.5, 3.5, true);
  hHitFirstLayerPhiAll->SetTitle("Layer with 1st track hit vs Phi - all tracks");
  addObject(hHitFirstLayerPhiAll.get());
  formatAxes(hHitFirstLayerPhiAll.get(), "#phi", "Layer with 1st hit", 1, 1.10);
  hHitFirstLayerPhiAll->SetStats(0);

  hHitFirstLayerPhi4cls = std::make_unique<TH2FRatio>("HitFirstLayer4cls", "HitFirstLayerPhi4cls", 65, -0.1, TMath::TwoPi(), 4, -0.5, 3.5, true);
  hHitFirstLayerPhi4cls->SetTitle("Layer with 1st track hit vs Phi - 4 cls tracks");
  addObject(hHitFirstLayerPhi4cls.get());
  formatAxes(hHitFirstLayerPhi4cls.get(), "#phi", "Layer with 1st hit", 1, 1.10);
  hHitFirstLayerPhi4cls->SetStats(0);

  hHitFirstLayerPhi5cls = std::make_unique<TH2FRatio>("HitFirstLayer5cls", "HitFirstLayerPhi5cls", 65, -0.1, TMath::TwoPi(), 4, -0.5, 3.5, true);
  hHitFirstLayerPhi5cls->SetTitle("Layer with 1st track hit vs Phi - 5 cls tracks");
  addObject(hHitFirstLayerPhi5cls.get());
  formatAxes(hHitFirstLayerPhi5cls.get(), "#phi", "Layer with 1st hit", 1, 1.10);
  hHitFirstLayerPhi5cls->SetStats(0);

  hHitFirstLayerPhi6cls = std::make_unique<TH2FRatio>("HitFirstLayer6cls", "HitFirstLayerPhi6cls", 65, -0.1, TMath::TwoPi(), 4, -0.5, 3.5, true);
  hHitFirstLayerPhi6cls->SetTitle("Layer with 1st track hit vs Phi - 6 cls tracks");
  addObject(hHitFirstLayerPhi6cls.get());
  formatAxes(hHitFirstLayerPhi6cls.get(), "#phi", "Layer with 1st hit", 1, 1.10);
  hHitFirstLayerPhi6cls->SetStats(0);

  hHitFirstLayerPhi7cls = std::make_unique<TH2FRatio>("HitFirstLayer7cls", "HitFirstLayerPhi7cls", 65, -0.1, TMath::TwoPi(), 4, -0.5, 3.5, true);
  hHitFirstLayerPhi7cls->SetTitle("Layer with 1st track hit vs Phi - 7 cls tracks");
  addObject(hHitFirstLayerPhi7cls.get());
  formatAxes(hHitFirstLayerPhi7cls.get(), "#phi", "Layer with 1st hit", 1, 1.10);
  hHitFirstLayerPhi7cls->SetStats(0);

  hClusterVsBunchCrossing = new TH2F("BunchCrossingIDvsClusterRatio", "BunchCrossingIDvsClusterRatio", nBCbins, 0, 4095, 100, 0, 1);
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

  if (mPublishMore) {
    hNClusterVsChipITS = new TH2F(Form("NClusterVsChipITS"), Form("NClusterVsChipITS"), 2124, mChipBins, 24, mCoslBins);
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

  // Invariant mass K0s, Lambda, LambdaBar
  hInvMassK0s = new TH1F("hInvMassK0s", "K0s invariant mass", 160, 0.0, 1.0);
  hInvMassK0s->SetTitle(Form("Invariant mass of K0s"));
  addObject(hInvMassK0s);
  formatAxes(hInvMassK0s, "m_{inv} (Gev/c)", "Counts", 1, 1.10);
  hInvMassK0s->SetStats(0);

  hInvMassLambda = new TH1F("hInvMassLambda", "Lambda invariant mass", 400, 1.0, 2.0);
  hInvMassLambda->SetTitle(Form("Invariant mass of Lambda"));
  addObject(hInvMassLambda);
  formatAxes(hInvMassLambda, "m_{inv} (Gev/c)", "Counts", 1, 1.10);
  hInvMassLambda->SetStats(0);

  hInvMassLambdaBar = new TH1F("hInvMassLambdaBar", "LambdaBar invariant mass", 400, 1.0, 2.0);
  hInvMassLambdaBar->SetTitle(Form("Invariant mass of LambdaBar"));
  addObject(hInvMassLambdaBar);
  formatAxes(hInvMassLambdaBar, "m_{inv} (Gev/c)", "Counts", 1, 1.10);
  hInvMassLambdaBar->SetStats(0);

  hTrackPtVsEta = new TH2F("hTrackPtVsEta", "Track #it{p}_{T} Vs #eta", 150, 0, 15, 40, -2.0, 2.0);
  addObject(hTrackPtVsEta);
  formatAxes(hTrackPtVsEta, "#it{p}_{T} (GeV/#it{c})", "#eta", 1, 1.10);
  hTrackPtVsEta->SetStats(0);

  hTrackPtVsPhi = new TH2F("hTrackPtVsPhi", "Track #it{p}_{T} Vs #phi", 150, 0, 15, 65, 0, TMath::TwoPi());
  addObject(hTrackPtVsPhi);
  formatAxes(hTrackPtVsPhi, "#it{p}_{T} (GeV/#it{c})", "#phi", 1, 1.10);
  hTrackPtVsPhi->SetStats(0);
}

void ITSTrackTask::addObject(TObject* aObject)
{
  if (!aObject) {
    ILOG(Debug, Devel) << " ERROR: trying to add non-existent object " << ENDM;
    return;
  } else {
    mPublishedObjects.push_back(aObject);
  }
}

void ITSTrackTask::publishHistos()
{
  for (unsigned int iObj = 0; iObj < mPublishedObjects.size(); iObj++) {
    getObjectsManager()->startPublishing(mPublishedObjects.at(iObj));
    ILOG(Debug, Devel) << " Object will be published: " << mPublishedObjects.at(iObj)->GetName() << ENDM;
  }
}

} // namespace o2::quality_control_modules::its
