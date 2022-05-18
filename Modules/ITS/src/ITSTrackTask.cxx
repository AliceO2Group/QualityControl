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
  delete hTrackEta;
  delete hTrackPhi;
  delete hAngularDistribution;
  delete hVertexCoordinates;
  delete hVertexRvsZ;
  delete hVertexZ;
  delete hVertexContributors;
  delete hAssociatedClusterFraction;
  delete hNtracks;
  delete hNClustersPerTrackEta;
  delete hClusterVsBunchCrossing;
  for (int l = 0; l < NLayer; l++) {
    delete hNClusterVsChip[l];
  }
  delete hNClusterVsChipITS;
}

void ITSTrackTask::initialize(o2::framework::InitContext& /*ctx*/)
{

  ILOG(Info, Support) << "initialize ITSTrackTask" << ENDM;

  mVertexXYsize = std::stof(mCustomParameters["vertexXYsize"]);
  mVertexZsize = std::stof(mCustomParameters["vertexZsize"]);
  mVertexRsize = std::stof(mCustomParameters["vertexRsize"]);
  mNtracksMAX = std::stof(mCustomParameters["NtracksMAX"]);
  mDoTTree = std::stoi(mCustomParameters["doTTree"]);
  nBCbins = std::stoi(mCustomParameters.find("nBCbins")->second);

  createAllHistos();
  publishHistos();

  // NClusterVsChip Full Detector distinguishable
  for (int l = 0; l < NLayer + 1; l++) {
    auto line = new TLine(ChipBoundary[l], 0, ChipBoundary[l], 10);
    hNClusterVsChipITS->GetListOfFunctions()->Add(line);
  }

  // get dict from ccdb
  mTimestamp = std::stol(mCustomParameters["dicttimestamp"]);
  long int ts = mTimestamp ? mTimestamp : o2::ccdb::getCurrentTimestamp();
  ILOG(Info, Support) << "Getting dictionary from ccdb - timestamp: " << ts << ENDM;
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setURL("http://alice-ccdb.cern.ch");
  mgr.setTimestamp(ts);
  mDict = mgr.get<o2::itsmft::TopologyDictionary>("ITS/Calib/ClusterDictionary");
  ILOG(Info, Support) << "Dictionary size: " << mDict->getSize() << ENDM;
}

void ITSTrackTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
}

void ITSTrackTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void ITSTrackTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  ILOG(Info, Support) << "START DOING QC General" << ENDM;
  auto trackArr = ctx.inputs().get<gsl::span<o2::its::TrackITS>>("tracks");
  auto trackRofArr = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("rofs");
  auto clusRofArr = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("clustersrof");
  auto clusArr = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("compclus");
  auto vertexArr = ctx.inputs().get<gsl::span<o2::dataformats::Vertex<o2::dataformats::TimeStamp<int>>>>("Vertices");

  auto clusIdx = ctx.inputs().get<gsl::span<int>>("clusteridx");
  auto clusPatternArr = ctx.inputs().get<gsl::span<unsigned char>>("patterns");
  auto pattIt = clusPatternArr.begin();

  for (const auto& vertex : vertexArr) {

    hVertexCoordinates->Fill(vertex.getX(), vertex.getY());
    hVertexRvsZ->Fill(vertex.getZ(), sqrt(vertex.getX() * vertex.getX() + vertex.getY() * vertex.getY()));
    hVertexZ->Fill(vertex.getZ());
    hVertexContributors->Fill(vertex.getNContributors());
  }

  for (int iROF = 0; iROF < trackRofArr.size(); iROF++) {

    vMap.clear();
    vEta.clear();
    vPhi.clear();

    int nClusterCntTrack = 0;
    int nTracks = trackRofArr[iROF].getNEntries();

    for (int itrack = 0; itrack < nTracks; itrack++) {

      auto& track = trackArr[itrack];
      auto out = track.getParamOut();
      Float_t Eta = -log(tan(out.getTheta() / 2.));
      hTrackEta->Fill(Eta);
      hTrackPhi->Fill(out.getPhi());
      hAngularDistribution->Fill(Eta, out.getPhi());
      hNClusters->Fill(track.getNumberOfClusters());

      vMap.emplace_back(track.getPattern());
      vEta.emplace_back(Eta);
      vPhi.emplace_back(out.getPhi());

      hNClustersPerTrackEta->Fill(Eta, track.getNumberOfClusters());
      nClusterCntTrack += track.getNumberOfClusters();
      for (int icluster = 0; icluster < track.getNumberOfClusters(); icluster++) {
        const int index = clusIdx[track.getFirstClusterEntry() + icluster];
        auto& cluster = clusArr[index];

        auto row = cluster.getRow();
        auto col = cluster.getCol();
        auto ChipID = cluster.getSensorID();
        auto ClusterID = cluster.getPatternID(); // used for normal (frequent) cluster shapes
        int npix = -1;
        int isGrouped = -1;

        if (ClusterID != o2::itsmft::CompCluster::InvalidPatternID && !mDict->isGroup(ClusterID)) { // Normal (frequent) cluster shapes
          npix = mDict->getNpixels(ClusterID);
          isGrouped = 0;
        } else {
          o2::itsmft::ClusterPattern patt(pattIt);
          npix = patt.getNPixels();
          isGrouped = 1;
        }

        int layer = 0;
        while (ChipID > ChipBoundary[layer])
          layer++;
        layer--;

        double clusterSizeWithCorrection = (double)npix * cos(std::atan(out.getTgl()));
        hNClusterVsChip[layer]->Fill(ChipID, clusterSizeWithCorrection);
        hNClusterVsChipITS->Fill(ChipID, clusterSizeWithCorrection);
      }
    }

    int nTotCls = clusRofArr[iROF].getNEntries();

    float clusterRatio = nTotCls > 0 ? (float)nClusterCntTrack / (float)nTotCls : -1;
    hAssociatedClusterFraction->Fill(clusterRatio);
    hNtracks->Fill(nTracks);

    const auto bcdata = trackRofArr[iROF].getBCData();
    hClusterVsBunchCrossing->Fill(bcdata.bc, clusterRatio);

    if (mDoTTree)
      tClusterMap->Fill();
  }

  mNTracks += trackArr.size();
  mNClusters += clusArr.size();
  mNRofs += trackRofArr.size();

  if (mNRofs >= NROFOCCUPANCY) {
    mNTracks = 0;
    mNRofs = 0;
    mNClusters = 0;
  }
}

void ITSTrackTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void ITSTrackTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ITSTrackTask::reset()
{
  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  hAngularDistribution->Reset();
  hNClusters->Reset();
  hTrackPhi->Reset();
  hTrackEta->Reset();

  hVertexCoordinates->Reset();
  hVertexRvsZ->Reset();
  hVertexZ->Reset();
  hVertexContributors->Reset();

  hAssociatedClusterFraction->Reset();
  hNtracks->Reset();
  hNClustersPerTrackEta->Reset();
  hClusterVsBunchCrossing->Reset();
  for (int l = 0; l < NLayer; l++) {
    hNClusterVsChip[l]->Reset();
  }
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

  hAngularDistribution = new TH2D("AngularDistribution", "AngularDistribution", 30, -1.5, 1.5, 60, 0, TMath::TwoPi());
  hAngularDistribution->SetTitle("AngularDistribution");
  addObject(hAngularDistribution);
  formatAxes(hAngularDistribution, "#eta", "#phi", 1, 1.10);
  hAngularDistribution->SetStats(0);

  hNClusters = new TH1D("NClusters", "NClusters", 20, 0, 20);
  hNClusters->SetTitle("hNClusters");
  addObject(hNClusters);
  formatAxes(hNClusters, "Number of clusters per Track", "Counts", 1, 1.10);
  hNClusters->SetStats(0);

  hTrackEta = new TH1D("EtaDistribution", "EtaDistribution", 30, -1.5, 1.5);
  hTrackEta->SetTitle("Eta Distribution of tracks");
  addObject(hTrackEta);
  formatAxes(hTrackEta, "#eta", "counts", 1, 1.10);
  hTrackEta->SetStats(0);

  hTrackPhi = new TH1D("PhiDistribution", "PhiDistribution", 60, 0, TMath::TwoPi());
  hTrackPhi->SetTitle("Phi Distribution of tracks");
  hTrackPhi->SetMinimum(0);
  addObject(hTrackPhi);
  formatAxes(hTrackPhi, "#phi", "counts", 1, 1.10);
  hTrackPhi->SetStats(0);

  hVertexCoordinates = new TH2D("VertexCoordinates", "VertexCoordinates", (int)(mVertexXYsize * 2 / 0.01), -1. * mVertexXYsize, mVertexXYsize, (int)(mVertexXYsize * 2 / 0.01), -1 * mVertexXYsize, mVertexXYsize);
  hVertexCoordinates->SetTitle("Coordinates of track vertex");
  addObject(hVertexCoordinates);
  formatAxes(hVertexCoordinates, "X coordinate (cm)", "Y coordinate (cm)", 1, 1.10);
  hVertexCoordinates->SetStats(0);

  hVertexRvsZ = new TH2D("VertexRvsZ", "VertexRvsZ", (int)(mVertexZsize * 2 / 0.01), -mVertexZsize, mVertexZsize, (int)(mVertexRsize / 0.01), 0, mVertexRsize);
  hVertexRvsZ->SetTitle("Distance to primary vertex vs Z");
  addObject(hVertexRvsZ);
  formatAxes(hVertexRvsZ, "Z coordinate (cm)", "R (cm)", 1, 1.10);
  hVertexRvsZ->SetStats(0);

  hVertexZ = new TH1D("VertexZ", "VertexZ", (int)(mVertexZsize * 2 / 0.01), -mVertexZsize, mVertexZsize);
  hVertexZ->SetTitle("Z coordinate of vertex");
  addObject(hVertexZ);
  formatAxes(hVertexZ, "Z coordinate (cm)", "counts", 1, 1.10);
  hVertexZ->SetStats(0);

  hVertexContributors = new TH1D("NVertexContributors", "NVertexContributors", 100, 0, 100);
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

  hNClustersPerTrackEta = new TH2D("NClustersPerTrackEta", "NClustersPerTrackEta", 300, -1.5, 1.5, 13, 1.5, 14.5);
  hNClustersPerTrackEta->SetTitle("Eta vs NClusters Per Track");
  addObject(hNClustersPerTrackEta);
  formatAxes(hNClustersPerTrackEta, "#eta", "# of Clusters per Track", 1, 1.10);
  hNClustersPerTrackEta->SetStats(0);

  hClusterVsBunchCrossing = new TH2D("BunchCrossingIDvsClusterRatio", "BunchCrossingIDvsClusterRatio", nBCbins, 0, 4095, 100, 0, 1);
  hClusterVsBunchCrossing->SetTitle("Bunch Crossing ID vs Cluster Ratio");
  addObject(hClusterVsBunchCrossing);
  formatAxes(hClusterVsBunchCrossing, "Bunch Crossing ID", "Fraction of clusters in tracks", 1, 1.10);
  hClusterVsBunchCrossing->SetStats(0);

  for (int l = 0; l < NLayer; l++) {
    hNClusterVsChip[l] = new TH2D(Form("NClusterVsChipInLayer%d", l), Form("NClusterVsChipInLayer%d", l), (int)(ChipBoundary[l + 1] - ChipBoundary[l]), ChipBoundary[l], ChipBoundary[l + 1], 60, 0, 15);
    hNClusterVsChip[l]->SetTitle(Form("Corrected cluster size for track clusters vs Chip in layer %d", l));
    addObject(hNClusterVsChip[l]);
    formatAxes(hNClusterVsChip[l], "chipID", "cluster size x cos(#lambda)", 1, 1.10);
    hNClusterVsChip[l]->SetStats(0);
  }

  hNClusterVsChipITS = new TH2D(Form("NClusterVsChipITS"), Form("NClusterVsChipITS"), (int)ChipBoundary[NLayer], 0, ChipBoundary[NLayer], 60, 0, 15);
  hNClusterVsChipITS->SetTitle(Form("Corrected cluster size for track clusters vs Chip Full Detector"));
  addObject(hNClusterVsChipITS);
  formatAxes(hNClusterVsChipITS, "chipID", "cluster size x cos(#lambda)", 1, 1.10);
  hNClusterVsChipITS->SetStats(0);
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
