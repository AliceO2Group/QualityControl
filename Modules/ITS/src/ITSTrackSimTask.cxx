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
/// \file   ITSTrackSimTask.cxx
/// \author Artem Isakov
///

#include "QualityControl/QcInfoLogger.h"
#include "ITS/ITSTrackSimTask.h"
#include <DetectorsCommonDataFormats/NameConf.h>
#include "DataFormatsITS/TrackITS.h"
#include <DataFormatsITSMFT/ROFRecord.h>
#include <Framework/InputRecord.h>


#include "SimulationDataFormat/MCTrack.h" //NEW
#include "SimulationDataFormat/MCCompLabel.h"
#include "Field/MagneticField.h"
#include "TGeoGlobalMagField.h"
#include "DetectorsBase/Propagator.h"



#include <typeinfo>

#include "GPUCommonDef.h"
#include "ReconstructionDataFormats/Track.h"
#include "CommonDataFormat/RangeReference.h"
#include "CommonConstants/MathConstants.h"
#include "Steer/MCKinematicsReader.h"

#include <fstream>
using namespace o2::constants::math;
using namespace o2::itsmft;
using namespace o2::its;



namespace o2::quality_control_modules::its
{

ITSTrackSimTask::ITSTrackSimTask() : TaskInterface()
{
 // createAllHistos();
}

ITSTrackSimTask::~ITSTrackSimTask()
{
/*
  delete hNClusters;
  delete hTrackEta;
  delete hTrackPhi;
  delete hOccupancyROF;
  delete hClusterUsage;
  delete hAngularDistribution;
*/

  delete hNumRecoValid_pt;
  delete hNumRecoFake_pt;
  delete hDenTrue_pt;
  delete hFakeTrack_pt;
  delete hEfficiency_pt;

  delete hNumRecoValid_eta;
  delete hNumRecoFake_eta;
  delete hDenTrue_eta;
  delete hFakeTrack_eta;
  delete hEfficiency_eta;


  delete hNumRecoValid_phi;
  delete hNumRecoFake_phi;
  delete hDenTrue_phi;
  delete hFakeTrack_phi;
  delete hEfficiency_phi;

  

  delete hNumRecoValid_r;
  delete hNumRecoFake_r;
  delete hDenTrue_r;
  delete hFakeTrack_r;
  delete hEfficiency_r;

  delete hNumRecoValid_z;
  delete hNumRecoFake_z;
  delete hDenTrue_z;
  delete hFakeTrack_z;
  delete hEfficiency_z;
 

 

  delete hTrackImpactTransvFake;
  delete hTrackImpactTransvValid;

}

void ITSTrackSimTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize ITSTrackSimTask" << ENDM;

  mRunNumberPath = mCustomParameters["runNumberPath"];


  const Int_t nb = 100;
  Double_t xbins[nb + 1], ptcutl = 0.01, ptcuth = 10.;
  Double_t a = TMath::Log(ptcuth / ptcutl) / nb;
  for (Int_t i = 0; i <= nb; i++) xbins[i] = ptcutl * TMath::Exp(i * a);

  hEfficiency_pt = new TH1D("efficiency_pt", ";#it{p}_{T} (GeV/#it{c});t{p}_{T}Efficiency", nb, xbins);
  hEfficiency_pt->SetTitle("#it{p}_{T} efficiency of tracking");
  addObject(hEfficiency_pt);
  formatAxes(hEfficiency_pt, "#it{p}_{T} (GeV/#it{c})", "Efficiency", 1, 1.10);

  hFakeTrack_pt = new TH1D("faketrack_pt", ";#it{p}_{T} (GeV/#it{c});Fake-track rate", nb, xbins);
  hFakeTrack_pt->SetTitle("#it{p}_{T} fake-track rate");
  addObject(hFakeTrack_pt);
  formatAxes(hFakeTrack_pt, "#it{p}_{T} (GeV/#it{c})", "Fake-track rate", 1, 1.10);

  hNumRecoValid_pt = new TH1D("NumRecoValid_pt", "", nb, xbins);
  hNumRecoFake_pt = new TH1D("NumRecoFake_pt", "", nb, xbins);
  hDenTrue_pt = new TH1D("DenTrueMC_pt", "", nb, xbins);


  hEfficiency_phi = new TH1D("efficiency_phi", ";#phi;Efficiency", 60, 0, TMath::TwoPi());
  hEfficiency_phi->SetTitle("#phi efficiency of tracking");
  addObject(hEfficiency_phi);
  formatAxes(hEfficiency_phi, "#phi", "Efficiency", 1, 1.10);

  hFakeTrack_phi = new TH1D("faketrack_phi", ";#phi;Fake-track rate", 60, 0, TMath::TwoPi());
  hFakeTrack_phi->SetTitle("#phi fake-track rate");
  addObject(hFakeTrack_phi);
  formatAxes(hFakeTrack_phi, "#phi", "Fake-track rate", 1, 1.10);

  hNumRecoValid_phi = new TH1D("NumRecoValid_phi", "", 60, 0, TMath::TwoPi());
  hNumRecoFake_phi = new TH1D("NumRecoFake_phi", "", 60, 0, TMath::TwoPi());
  hDenTrue_phi = new TH1D("DenTrueMC_phi", "", 60, 0, TMath::TwoPi());

 

  hEfficiency_eta = new TH1D("efficiency_eta", ";#eta;Efficiency",  30, -1.5, 1.5);
  hEfficiency_eta->SetTitle("#eta efficiency of tracking");
  addObject(hEfficiency_eta);
  formatAxes(hEfficiency_eta, "#eta", "Efficiency", 1, 1.10);

  hFakeTrack_eta = new TH1D("faketrack_eta", ";#eta;Fake-track rate",  30, -1.5, 1.5);
  hFakeTrack_eta->SetTitle("#eta fake-track rate");
  addObject(hFakeTrack_eta);
  formatAxes(hFakeTrack_eta, "#eta", "Fake-track rate", 1, 1.10);

  hNumRecoValid_eta = new TH1D("NumRecoValid_eta", "",  30, -1.5, 1.5);
  hNumRecoFake_eta = new TH1D("NumRecoFake_eta", "",  30, -1.5, 1.5);
  hDenTrue_eta = new TH1D("DenTrueMC_eta", "", 30, -1.5, 1.5);



  hEfficiency_r = new TH1D("efficiency_r", ";r;Efficiency",  50, 0, 5);
  hEfficiency_r->SetTitle("r efficiency of tracking");
  addObject(hEfficiency_r);
  formatAxes(hEfficiency_r, "r", "Efficiency", 1, 1.10);

  hFakeTrack_r = new TH1D("faketrack_r", ";r;Fake-track rate",  50, 0, 5);
  hFakeTrack_r->SetTitle("r fake-track rate");
  addObject(hFakeTrack_r);
  formatAxes(hFakeTrack_r, "r", "Fake-track rate", 1, 1.10);

  hNumRecoValid_r = new TH1D("NumRecoValid_r", "",  50,0, 5);
  hNumRecoFake_r = new TH1D("NumRecoFake_r", "",  50,0,5);
  hDenTrue_r = new TH1D("DenTrueMC_r", "", 50, 0, 50);


  hEfficiency_z = new TH1D("efficiency_z", ";z;Efficiency",  100, -5, 5);
  hEfficiency_z->SetTitle("z efficiency of tracking");
  addObject(hEfficiency_z);
  formatAxes(hEfficiency_z, "z", "Efficiency", 1, 1.10);

  hFakeTrack_z = new TH1D("faketrack_z", ";z;Fake-track rate",  100, -5, 5);
  hFakeTrack_z->SetTitle("z fake-track rate");
  addObject(hFakeTrack_z);
  formatAxes(hFakeTrack_z, "z", "Fake-track rate", 1, 1.10);

  hNumRecoValid_z = new TH1D("NumRecoValid_z", "",  100,-5, 5);
  hNumRecoFake_z = new TH1D("NumRecoFake_z", "", 100,-5, 5);
  hDenTrue_z = new TH1D("DenTrueMC_z", "", 100, -5, 5);

  hTrackImpactTransvValid = new TH1F("ImpactTransvVaild", "Transverse impact parameter for valid tracks; D (cm)", 60, -0.1, 0.1);
  hTrackImpactTransvValid->SetTitle("Transverse impact parameter distribution of valid tracks");
  addObject(hTrackImpactTransvValid);
  formatAxes(hTrackImpactTransvValid, "D (cm)", "counts", 1, 1.10);


  hTrackImpactTransvFake = new TH1F("ImpactTransvFake", "Transverse impact parameter for fake tracks; D (cm)", 60, -0.1, 0.1);
  hTrackImpactTransvFake->SetTitle("Transverse impact parameter distribution of fake tracks");
  addObject(hTrackImpactTransvFake);
  formatAxes(hTrackImpactTransvFake, "D (cm)", "counts", 1, 1.10);


  o2::steer::MCKinematicsReader reader("o2sim", o2::steer::MCKinematicsReader::Mode::kMCKine);
 
  for (int iSource=0;iSource < (int) reader.getNSources();iSource++){
   for (int iEvent=0;iEvent < (int) reader.getNEvents(iSource);iEvent++){
      
      auto mcTracks = reader.getTracks(iSource, iEvent);
      auto mcHeader= reader.getMCEventHeader(iSource, iEvent);
      for (auto mcTrack : mcTracks){

        //if (mcTrack.isPrimary()){
           if (mcTrack.Vx() * mcTrack.Vx() + mcTrack.Vy() * mcTrack.Vy() > 1) continue; 
           if ( abs(mcTrack.Vz())  > 10 ) continue;
        //}
        Int_t pdg = mcTrack.GetPdgCode();
        if (TMath::Abs(pdg) != 211) continue; // Select pions
        if (TMath::Abs(mcTrack.GetEta()) > 1.2) continue;
  
        Double_t distance = sqrt(    pow(mcHeader.GetX()-mcTrack.Vx(),2) +  pow(mcHeader.GetY()-mcTrack.Vy(),2) +  pow(mcHeader.GetZ()-mcTrack.Vz(),2) );   
        hDenTrue_r->Fill(distance);
        hDenTrue_pt->Fill(mcTrack.GetPt());
        hDenTrue_eta->Fill(mcTrack.GetEta());
        hDenTrue_phi->Fill(mcTrack.GetPhi());  
        hDenTrue_z->Fill(mcTrack.Vz());

      }
    }
  }



  publishHistos();
  o2::base::Propagator::initFieldFromGRP("./o2sim_grp.root");
  auto field = static_cast<o2::field::MagneticField*>(TGeoGlobalMagField::Instance()->GetField());
  double orig[3] = {0., 0., 0.};
  bz = field->getBz(orig);

}

void ITSTrackSimTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
   //reset();

}

void ITSTrackSimTask::startOfCycle()
{
   ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void ITSTrackSimTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  ILOG(Info, Support) << "START DOING QC General" << ENDM;


  auto trackArr = ctx.inputs().get<gsl::span<o2::its::TrackITS>>("tracks");
  auto MCTruth= ctx.inputs().get<gsl::span<o2::MCCompLabel>>("mstruth");

  o2::steer::MCKinematicsReader reader("o2sim", o2::steer::MCKinematicsReader::Mode::kMCKine);

  for (int itrack = 0; itrack < trackArr.size(); itrack++) {
      const auto& track = trackArr[itrack];
      const auto& MCinfo = MCTruth[itrack];
      
      if (MCinfo.isNoise()) continue;

      auto* mcTrack = reader.getTrack(MCinfo.getSourceID(), MCinfo.getEventID(), MCinfo.getTrackID());
      auto mcHeader= reader.getMCEventHeader(MCinfo.getSourceID(), MCinfo.getEventID());
      Float_t ip[2]{0., 0.};
      Float_t vx = 0., vy = 0., vz = 0.; // Assumed primary vertex
      track.getImpactParams(vx, vy, vz, bz, ip);

      if (mcTrack) {

 //        if  (mcTrack->Vx() * mcTrack->Vx() + mcTrack->Vy() * mcTrack->Vy() > 0) std::cout<<"Track found: isPrimary: " <<  mcTrack->isPrimary() << " isFake "<<  MCinfo.isFake() << " x= "<< mcTrack->Vx() <<   " y= "<< mcTrack->Vy() <<  " z= "<< mcTrack->Vz()<< std::endl;
               

/*
        if (!mcTrack->isPrimary() && MCinfo.isFake()) std::cout<<"Secondary fake track found: "<< mcTrack->Vx() * mcTrack->Vx() + mcTrack->Vy() * mcTrack->Vy() <<std::endl;       
        else 
           if (MCinfo.isFake()) std::cout<<"Primary fake track found: "<< mcTrack->Vx() * mcTrack->Vx() + mcTrack->Vy() * mcTrack->Vy() <<std::endl;

        if (!mcTrack->isPrimary() && !MCinfo.isFake()) std::cout<<"Secondary true track found: "<< mcTrack->Vx() * mcTrack->Vx() + mcTrack->Vy() * mcTrack->Vy() <<std::endl;
        else
           if (!MCinfo.isFake()) std::cout<<"Primary true track found: "<< mcTrack->Vx() * mcTrack->Vx() + mcTrack->Vy() * mcTrack->Vy() <<std::endl;

*/


   //     if (mcTrack->isPrimary()){
           if (mcTrack->Vx() * mcTrack->Vx() + mcTrack->Vy() * mcTrack->Vy() > 1) continue; 
           if ( abs(mcTrack->Vz())  > 10 ) continue;
     //   }

        Int_t pdg = mcTrack->GetPdgCode();
        if (TMath::Abs(pdg) != 211) continue; // Select pions
                if (!mcTrack->isPrimary() && MCinfo.isFake()) std::cout<<"3"<<std::endl;
        if (TMath::Abs(mcTrack->GetEta()) > 1.2) continue;
        
        if (!mcTrack->isPrimary() && MCinfo.isFake())  std::cout<<"and it survived "<<std::endl;

        Double_t distance = sqrt(   pow(mcHeader.GetX()-mcTrack->Vx(),2) +  pow(mcHeader.GetY()-mcTrack->Vy(),2) +  pow(mcHeader.GetZ()-mcTrack->Vz(),2));
         if (MCinfo.isFake()) {
            hNumRecoFake_pt->Fill(mcTrack->GetPt());
            hNumRecoFake_phi->Fill(mcTrack->GetPhi());
            hNumRecoFake_eta->Fill(mcTrack->GetEta());
            hNumRecoFake_z->Fill(mcTrack->Vz());
            hNumRecoFake_r->Fill(distance); 
            if (mcTrack->isPrimary())  hTrackImpactTransvFake->Fill(ip[0]);
            else std::cout<< " I am fake but I am Secondary particle" <<std::endl;

         }
         else {
            hNumRecoValid_pt->Fill(mcTrack->GetPt());
            hNumRecoValid_phi->Fill(mcTrack->GetPhi());
            hNumRecoValid_eta->Fill(mcTrack->GetEta());
            hNumRecoValid_z->Fill(mcTrack->Vz());
            hNumRecoValid_r->Fill(distance);
            hTrackImpactTransvValid->Fill(ip[0]); 
            if (mcTrack->isPrimary()) hTrackImpactTransvValid->Fill(ip[0]);
         }
      }

   }
    
 
  hFakeTrack_pt->Divide(hNumRecoFake_pt, hDenTrue_pt, 1, 1);
  hEfficiency_pt->Divide(hNumRecoValid_pt, hDenTrue_pt, 1, 1);

  hFakeTrack_phi->Divide(hNumRecoFake_phi, hDenTrue_phi, 1, 1);
  hEfficiency_phi->Divide(hNumRecoValid_phi, hDenTrue_phi, 1, 1);

  hFakeTrack_eta->Divide(hNumRecoFake_eta, hDenTrue_eta, 1, 1);
  hEfficiency_eta->Divide(hNumRecoValid_eta, hDenTrue_eta, 1, 1);
 
  hFakeTrack_r->Divide(hNumRecoFake_r, hDenTrue_r, 1, 1);
  hEfficiency_r->Divide(hNumRecoValid_r, hDenTrue_r, 1, 1);

  hFakeTrack_z->Divide(hNumRecoFake_z, hDenTrue_z, 1, 1);
  hEfficiency_z->Divide(hNumRecoValid_z, hDenTrue_z, 1, 1);




std::cout<<"---------------------  END OF THE TASK --------------------"<<std::endl;

}

void ITSTrackSimTask::endOfCycle()
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

void ITSTrackSimTask::endOfActivity(Activity& /*activity*/)
{
   ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ITSTrackSimTask::reset()
{
    ILOG(Info, Support) << "Resetting the histogram" << ENDM;
/*  hAngularDistribution->Reset();
  hNClusters->Reset();
  hTrackPhi->Reset();
  hTrackEta->Reset();
  hOccupancyROF->Reset();
  hClusterUsage->Reset();
*/
  hEfficiency_pt->Reset();
  hFakeTrack_pt->Reset();
  hNumRecoValid_pt->Reset();
  hNumRecoFake_pt->Reset();
  hDenTrue_pt->Reset();

  hEfficiency_phi->Reset();
  hFakeTrack_phi->Reset();
  hNumRecoValid_phi->Reset();
  hNumRecoFake_phi->Reset();
  hDenTrue_phi->Reset();

  hEfficiency_eta->Reset();
  hFakeTrack_eta->Reset();
  hNumRecoValid_eta->Reset();
  hNumRecoFake_eta->Reset();
  hDenTrue_eta->Reset();

  hEfficiency_r->Reset();
  hFakeTrack_r->Reset();
  hNumRecoValid_r->Reset();
  hNumRecoFake_r->Reset();
  hDenTrue_r->Reset();


  hEfficiency_z->Reset();
  hFakeTrack_z->Reset();
  hNumRecoValid_z->Reset();
  hNumRecoFake_z->Reset();
  hDenTrue_z->Reset();



  hTrackImpactTransvValid->Reset();
  hTrackImpactTransvFake->Reset();

  
}

void ITSTrackSimTask::createAllHistos()
{

  hAngularDistribution = new TH2D("AngularDistribution", "AngularDistribution", 30, -1.5, 1.5, 60, 0, TMath::TwoPi());
  hAngularDistribution->SetTitle("AngularDistribution");
  addObject(hAngularDistribution);
  formatAxes(hAngularDistribution, "#eta", "#phi", 1, 1.10);
  hAngularDistribution->SetStats(0);

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

  hTrackPt = new TH1D("PtDistribution", "PtDistribution", 200, 0, 100);
  hTrackPt->SetTitle("Pt Distribution of tracks");
  addObject(hTrackPt);
  formatAxes(hTrackPt, "#pt", "counts", 1, 1.10);

  hTrackImpactTransvValid = new TH1F("ImpactTransvVaild", "Transverse impact parameter for valid tracks; D (cm)", 30, -0.1, 0.1);
  hTrackImpactTransvValid->SetTitle("Transverse impact parameter distribution of valid tracks");
  addObject(hTrackImpactTransvValid);
  formatAxes(hTrackImpactTransvValid, "D (cm)", "counts", 1, 1.10);


  hTrackImpactTransvFake = new TH1F("ImpactTransvFake", "Transverse impact parameter for fake tracks; D (cm)", 30, -0.1, 0.1);
  hTrackImpactTransvFake->SetTitle("Transverse impact parameter distribution of fake tracks");
  addObject(hTrackImpactTransvFake);
  formatAxes(hTrackImpactTransvFake, "D (cm)", "counts", 1, 1.10);



  hOccupancyROF = new TH1D("OccupancyROF", "OccupancyROF", 1, 0, 1);
  hOccupancyROF->SetTitle("Track occupancy in ROF");
  addObject(hOccupancyROF);
  formatAxes(hOccupancyROF, "", "nTracks/ROF", 1, 1.10);

  hClusterUsage = new TH1D("ClusterUsage", "ClusterUsage", 1, 0, 1);
  hClusterUsage->SetTitle("Fraction of clusters used in tracking");
  addObject(hClusterUsage);
  formatAxes(hClusterUsage, "", "nCluster in track / Total cluster", 1, 1.10);

  /*
  hEfficiency = new TH1D("efficiency", ";#it{p}_{T} (GeV/#it{c});Efficiency (fake-track rate)", nb, xbins);
  hEfficiency->SetTitle("Efficiency of tracking");
  addObject(hEfficiency);
  formatAxes(hEfficiency, "#it{p}_{T} (GeV/#it{c})", "Efficiency", 1, 1.10); 

  hFakeTrack = new TH1D("faketrack", ";#it{p}_{T} (GeV/#it{c});Fake-track rate", nb, xbins);
  hFakeTrack->SetTitle("Fake-track rate");
  addObject(hFakeTrack);
  formatAxes(hFakeTrack, "#it{p}_{T} (GeV/#it{c})", "Fake-track rate", 1, 1.10);   
 
*/



}

void ITSTrackSimTask::addObject(TObject* aObject)
{
  if (!aObject) {
    LOG(INFO) << " ERROR: trying to add non-existent object ";
    return;
  } else {
    mPublishedObjects.push_back(aObject);
  }
}

void ITSTrackSimTask::formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset, float yOffset)
{
  h->GetXaxis()->SetTitle(xTitle);
  h->GetYaxis()->SetTitle(yTitle);
  h->GetXaxis()->SetTitleOffset(xOffset);
  h->GetYaxis()->SetTitleOffset(yOffset);
}

void ITSTrackSimTask::publishHistos()
{
  for (unsigned int iObj = 0; iObj < mPublishedObjects.size(); iObj++) {
    getObjectsManager()->startPublishing(mPublishedObjects.at(iObj));
    LOG(INFO) << " Object will be published: " << mPublishedObjects.at(iObj)->GetName();
  }
}

} // namespace o2::quality_control_modules::its
