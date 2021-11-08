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
#include <DetectorsCommonDataFormats/NameConf.h>
#include <DataFormatsITS/TrackITS.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <Framework/InputRecord.h>
#include "ReconstructionDataFormats/Vertex.h"
#include "ReconstructionDataFormats/PrimaryVertex.h"



#include <Framework/DataSpecUtils.h>


using namespace o2::itsmft;
using namespace o2::its;
//using Vertex = o2::dataformats::Vertex<o2::dataformats::TimeStamp<int>>;


namespace o2::quality_control_modules::its
{

ITSTrackTask::ITSTrackTask() : TaskInterface()
{
  createAllHistos();
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

  QcInfoLogger::GetInstance() << "initialize ITSTrackTask" << AliceO2::InfoLogger::InfoLogger::endm;

  mRunNumberPath = mCustomParameters["runNumberPath"];
  mVertexXYsize = std::stoi(mCustomParameters["vertexXYsize"]);
  mVertexZsize = std::stoi(mCustomParameters["vertexZsize"]);
  mVertexRsize = std::stoi(mCustomParameters["vertexRsize"]);


  std::cout<<" %%%%%%%%%%%%%%%%%%%%%%%%%% mVertexXYsize "<<mVertexXYsize<<" mVertexZsize "<<mVertexZsize<< " mVertexRsize "<<mVertexRsize<<std::endl; 

  publishHistos();
}

void ITSTrackTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ITSTrackTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ITSTrackTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  QcInfoLogger::GetInstance() << "START DOING QC General" << AliceO2::InfoLogger::InfoLogger::endm;
  auto trackArr = ctx.inputs().get<gsl::span<o2::its::TrackITS>>("tracks");
  auto rofArr = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("rofs");
  auto clusArr = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("compclus");

  auto vertexArr = ctx.inputs().get< gsl::span<o2::dataformats::Vertex<o2::dataformats::TimeStamp<int>>> >("Vertices");


  //auto vertexArr = ctx.inputs().get<o2::dataformats::VertexBase>("vertices");
//  std::vector<o2::dataformats::PrimaryVertex> vertexArr = ctx.inputs().get<o2::dataformats::PrimaryVertex>("Vertices");
  // std::vector<o2::dataformats::VertexBase> vertexArr = ctx.inputs().get<gsl::span<o2::dataformats::VertexBase>>("vertices");
 //auto vertexArr = ctx.inputs().get<gsl::span<  o2::dataformats::Vertex<o2::dataformats::TimeStamp<int>>        >>("vertices");
//std::vector<Vertex> vertexArr = ctx.inputs().get<gsl::span<Vertex>>("Vertices");
//std::vector<Vertex> vertexArr = ctx.inputs().get<Vertex>("Vertices");
//  std::vector<o2::dataformats::Vertex<int>>  vertexArr = ctx.inputs().get<gsl::span<  o2::dataformats::Vertex<int>>>("vertices");
//   auto vertexArr = DataRefUtils::as< o2::dataformats::Vertex<int   >(ctx.inputs().get("vertices"));
   //std::vector<o2::dataformats::Vertex<int>>  *vertexArr = ctx.inputs().get<o2::dataformats::Vertex<int>>("vertices");
//    auto vertexArr = ctx.inputs().get<o2::dataformats::VertexBase>("vertices");
// auto vertexArr = ctx.inputs().get<gsl::span<  o2::dataformats::Vertex<int>>>("vertices");

  std::cout<<"!!! obtained vector with verticies: "<<vertexArr.size() <<std::endl;


  for (const auto& vertex: vertexArr){
  
      hVertexCoordinates->Fill(vertex.getX(),vertex.getY());

      std::cout<< " R = " <<sqrt(vertex.getX()*vertex.getX()+ vertex.getY()*vertex.getY()) << " X: "<<vertex.getX() << " Y: "<< vertex.getY() << " Z: "<<vertex.getZ()<<std::endl;
      hVertexRvsZ->Fill(sqrt(vertex.getX()*vertex.getX()+ vertex.getY()*vertex.getY()),vertex.getZ());
      hVertexZ->Fill(vertex.getZ());
      hVertexContributors->Fill(vertex.getNContributors());
  }





  for (const auto& ROF : rofArr) {
    for (int itrack = ROF.getFirstEntry(); itrack < ROF.getFirstEntry() + ROF.getNEntries(); itrack++) {

      auto& track = trackArr[itrack];

      auto out = track.getParamOut();
      Double_t Eta = -log(tan(out.getTheta() / 2.));
      hTrackEta->Fill(Eta);
      hTrackPhi->Fill(out.getPhi());
      hAngularDistribution->Fill(Eta, out.getPhi());

      hNClusters->Fill(track.getNumberOfClusters());
      mNClustersInTracks += track.getNumberOfClusters();
   }
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
    QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
  }
}

void ITSTrackTask::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ITSTrackTask::reset()
{
  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
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

  hAngularDistribution = new TH2D("AngularDistribution", "AngularDistribution", 30, -1.5, 1.5, 60, 0, TMath::TwoPi());
  hAngularDistribution->SetTitle("AngularDistribution");
  addObject(hAngularDistribution);
  formatAxes(hAngularDistribution, "#eta", "#phi", 1, 1.10);

  hNClusters = new TH1D("NClusters", "NClusters", 100, 0, 100);
  hNClusters->SetTitle("hNClusters");
  addObject(hNClusters);
  formatAxes(hNClusters, "Number of clusters per Track", "Counts", 1, 1.10);

  hTrackEta = new TH1D("EtaDistribution", "EtaDistribution", 30, -1.5, 1.5);
  hTrackEta->SetTitle("Eta Distribution of tracks");
  addObject(hTrackEta);
  formatAxes(hTrackEta, "#eta", "counts", 1, 1.10);

  hTrackPhi = new TH1D("PhiDistribution", "PhiDistribution", 60, 0, TMath::TwoPi());
  hTrackPhi->SetTitle("Phi Distribution of tracks");
  addObject(hTrackPhi);
  formatAxes(hTrackPhi, "#phi", "counts", 1, 1.10);

  hOccupancyROF = new TH1D("OccupancyROF", "OccupancyROF", 1, 0, 1);
  hOccupancyROF->SetTitle("Track occupancy in ROF");
  addObject(hOccupancyROF);
  formatAxes(hOccupancyROF, "", "nTracks/ROF", 1, 1.10);

  hClusterUsage = new TH1D("ClusterUsage", "ClusterUsage", 1, 0, 1);
  hClusterUsage->SetTitle("Fraction of clusters used in tracking");
  addObject(hClusterUsage);
  formatAxes(hClusterUsage, "", "nCluster in track / Total cluster", 1, 1.10);

  hVertexCoordinates = new TH2D("VertexCoordinates", "VertexCoordinates",200,-1. * mVertexXYsize,mVertexXYsize,200,-1* mVertexXYsize,mVertexXYsize);
  hVertexCoordinates->SetTitle("Coordinates of track vertex");
  addObject(hVertexCoordinates);
  formatAxes(hVertexCoordinates, "X coordinate (cm)", "Y coordinate (cm)", 1, 1.10);

  hVertexRvsZ = new TH2D("VertexRvsZ", "VertexRvsZ",200,0,mVertexRsize,200,-mVertexZsize,mVertexZsize);
  hVertexRvsZ->SetTitle("Distance to primary vertex vs Z");
  addObject(hVertexRvsZ);
  formatAxes(hVertexRvsZ, "R (cm) ", "Z coordinate (cm)", 1, 1.10);

  hVertexZ = new TH1D("VertexZ", "VertexZ", 200, -1. *mVertexXYsize,mVertexXYsize);
  hVertexZ->SetTitle("Z coordinate of vertex");
  addObject(hVertexZ);
  formatAxes(hVertexZ, "Z coordinate (cm)","counts", 1, 1.10);

  hVertexContributors = new TH1D("NVertexContributors", "NVertexContributors", 100, 0, 100);
  hVertexContributors->SetTitle("NVertexContributors");
  addObject(hVertexContributors); 
  formatAxes(hVertexContributors, "Number of contributors for vertex", "Counts", 1, 1.10);



}

void ITSTrackTask::addObject(TObject* aObject)
{
  if (!aObject) {
    LOG(INFO) << " ERROR: trying to add non-existent object ";
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
    LOG(INFO) << " Object will be published: " << mPublishedObjects.at(iObj)->GetName();
  }
}

} // namespace o2::quality_control_modules::its
