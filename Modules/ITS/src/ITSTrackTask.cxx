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

#include <Framework/DataSpecUtils.h>

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
  delete hOccupancyROF;
  delete hClusterUsage;
  delete hAngularDistribution;
  delete hVertexCoordinates;
  delete hVertexRvsZ;
  delete hVertexZ;
  delete hVertexContributors;
}

void ITSTrackTask::initialize(o2::framework::InitContext& /*ctx*/)
{

  ILOG(Info, Support) << "initialize ITSTrackTask" << ENDM;

  mRunNumberPath = mCustomParameters["runNumberPath"];
  mVertexXYsize = std::stof(mCustomParameters["vertexXYsize"]);
  mVertexZsize = std::stof(mCustomParameters["vertexZsize"]);
  mVertexRsize = std::stof(mCustomParameters["vertexRsize"]);
  mDoTTree = std::stoi(mCustomParameters["doTTree"]);
  createAllHistos();
  publishHistos();
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
  auto rofArr = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("rofs");
  auto clusArr = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("compclus");
  auto vertexArr = ctx.inputs().get<gsl::span<o2::dataformats::Vertex<o2::dataformats::TimeStamp<int>>>>("Vertices");

  for (const auto& vertex : vertexArr) {

    hVertexCoordinates->Fill(vertex.getX(), vertex.getY());
    hVertexRvsZ->Fill(sqrt(vertex.getX() * vertex.getX() + vertex.getY() * vertex.getY()), vertex.getZ());
    hVertexZ->Fill(vertex.getZ());
    hVertexContributors->Fill(vertex.getNContributors());
  }

  for (const auto& ROF : rofArr) {

    vMap.clear();
    vEta.clear();
    vPhi.clear();

    for (int itrack = ROF.getFirstEntry(); itrack < ROF.getFirstEntry() + ROF.getNEntries(); itrack++) {

      auto& track = trackArr[itrack];
      auto out = track.getParamOut();
      Float_t Eta = -log(tan(out.getTheta() / 2.));
      hTrackEta->Fill(Eta);
      hTrackPhi->Fill(out.getPhi());
      hAngularDistribution->Fill(Eta, out.getPhi());
      hNClusters->Fill(track.getNumberOfClusters());
      mNClustersInTracks += track.getNumberOfClusters();

      vMap.emplace_back(track.getPattern());
      vEta.emplace_back(Eta);
      vPhi.emplace_back(out.getPhi());
    }

    if (mDoTTree) tClusterMap->Fill();
  }

  mNTracks += trackArr.size();
  mNClusters += clusArr.size();
  mNRofs += rofArr.size();

  if (mNRofs >= NROFOCCUPANCY) {
    hOccupancyROF->SetBinContent(1, 1. * mNTracks / mNRofs);
    mNTracks = 0;
    mNRofs = 0;
    hClusterUsage->SetBinContent(1, 1. * mNClustersInTracks / mNClusters);
    mNClustersInTracks = 0;
    mNClusters = 0;
  }
}

void ITSTrackTask::endOfCycle()
{

  std::ifstream runNumberFile(mRunNumberPath.c_str());
  if (runNumberFile) {

    std::string runNumber;
    runNumberFile >> runNumber;
    if (runNumber != mRunNumber) {
      for (unsigned int iObj = 0; iObj < mPublishedObjects.size(); iObj++)
        getObjectsManager()->addMetadata(mPublishedObjects.at(iObj)->GetName(), "Run", runNumber);
      mRunNumber = runNumber;
    }
    ILOG(Info, Support) << "endOfCycle" << ENDM;
  }
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
  hOccupancyROF->Reset();
  hClusterUsage->Reset();

  hVertexCoordinates->Reset();
  hVertexRvsZ->Reset();
  hVertexZ->Reset();
  hVertexContributors->Reset();
}

void ITSTrackTask::createAllHistos()
{
  tClusterMap = new TTree("ClusterMap", "Cluster Map");
  tClusterMap->Branch("bitmap", &vMap);
  tClusterMap->Branch("eta", &vEta);
  tClusterMap->Branch("phi", &vPhi);
  if (mDoTTree) addObject(tClusterMap);

  hAngularDistribution = new TH2D("AngularDistribution", "AngularDistribution", 30, -1.5, 1.5, 60, 0, TMath::TwoPi());
  hAngularDistribution->SetTitle("AngularDistribution");
  addObject(hAngularDistribution);
  formatAxes(hAngularDistribution, "#eta", "#phi", 1, 1.10);
  hAngularDistribution->SetStats(0);

  hNClusters = new TH1D("NClusters", "NClusters", 100, 0, 100);
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

  hOccupancyROF = new TH1D("OccupancyROF", "OccupancyROF", 1, 0, 1);
  hOccupancyROF->SetTitle("Track occupancy in ROF");
  addObject(hOccupancyROF);
  formatAxes(hOccupancyROF, "", "nTracks/ROF", 1, 1.10);
  hOccupancyROF->SetStats(0);
  hOccupancyROF->SetBit(TH1::kIsAverage);

  hClusterUsage = new TH1D("ClusterUsage", "ClusterUsage", 1, 0, 1);
  hClusterUsage->SetTitle("Fraction of clusters used in tracking");
  addObject(hClusterUsage);
  formatAxes(hClusterUsage, "", "nCluster in track / Total cluster", 1, 1.10);
  hClusterUsage->SetStats(0);
  hClusterUsage->SetBit(TH1::kIsAverage);

  hVertexCoordinates = new TH2D("VertexCoordinates", "VertexCoordinates", 1000, -1. * mVertexXYsize, mVertexXYsize, 1000, -1 * mVertexXYsize, mVertexXYsize);
  hVertexCoordinates->SetTitle("Coordinates of track vertex");
  addObject(hVertexCoordinates);
  formatAxes(hVertexCoordinates, "X coordinate (cm)", "Y coordinate (cm)", 1, 1.10);
  hVertexCoordinates->SetStats(0);

  hVertexRvsZ = new TH2D("VertexRvsZ", "VertexRvsZ", 1000, 0, mVertexRsize, 2000, -mVertexZsize, mVertexZsize);
  hVertexRvsZ->SetTitle("Distance to primary vertex vs Z");
  addObject(hVertexRvsZ);
  formatAxes(hVertexRvsZ, "R (cm) ", "Z coordinate (cm)", 1, 1.10);
  hVertexRvsZ->SetStats(0);

  hVertexZ = new TH1D("VertexZ", "VertexZ", 2000, -mVertexZsize, mVertexZsize);
  hVertexZ->SetTitle("Z coordinate of vertex");
  addObject(hVertexZ);
  formatAxes(hVertexZ, "Z coordinate (cm)", "counts", 1, 1.10);
  hVertexRvsZ->SetStats(0);

  hVertexContributors = new TH1D("NVertexContributors", "NVertexContributors", 100, 0, 100);
  hVertexContributors->SetTitle("NVertexContributors");
  addObject(hVertexContributors);
  formatAxes(hVertexContributors, "Number of contributors for vertex", "Counts", 1, 1.10);
  hVertexContributors->SetStats(0);
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
