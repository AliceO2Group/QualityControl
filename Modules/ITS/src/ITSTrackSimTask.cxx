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
/// MC simulation tool inspired by Detectors/ITS/macros/CheckTracks
//
///

#include "QualityControl/QcInfoLogger.h"
#include "ITS/ITSTrackSimTask.h"
#include "DataFormatsITS/TrackITS.h"
#include <DataFormatsITSMFT/ROFRecord.h>
#include <Framework/InputRecord.h>

#include "SimulationDataFormat/MCTrack.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include "Field/MagneticField.h"
#include "TGeoGlobalMagField.h"
#include "DetectorsBase/Propagator.h"

#include "DataFormatsITSMFT/CompCluster.h"
#include "Steer/MCKinematicsReader.h" //ADDED FOR MC FILE READING

#include <typeinfo>

#include "GPUCommonDef.h"
#include "ReconstructionDataFormats/Track.h"
#include "CommonDataFormat/RangeReference.h"
#include "CommonConstants/MathConstants.h"
#include "SimulationDataFormat/MCTruthContainer.h"
#include "SimulationDataFormat/ConstMCTruthContainer.h"
#include "TFile.h"
#include "TTree.h"
#include <fstream>
using namespace o2::constants::math;
using namespace o2::itsmft;
using namespace o2::its;

namespace o2::quality_control_modules::its
{

ITSTrackSimTask::ITSTrackSimTask() : TaskInterface()
{
}

ITSTrackSimTask::~ITSTrackSimTask()
{
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

  delete hPrimaryReco_pt;
  delete hPrimaryGen_pt;

  delete hAngularDistribution;

  delete hDuplicate_pt;
  delete hNumDuplicate_pt;
  delete hDuplicate_phi;
  delete hNumDuplicate_phi;
  delete hDuplicate_eta;
  delete hNumDuplicate_eta;
  delete hDuplicate_r;
  delete hNumDuplicate_r;
  delete hDuplicate_z;
  delete hNumDuplicate_z;
}

void ITSTrackSimTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize ITSTrackSimTask" << ENDM;
  mO2GrpPath = mCustomParameters["o2GrpPath"];
  mCollisionsContextPath = mCustomParameters["collisionsContextPath"];
  createAllHistos();
  publishHistos();
  o2::base::Propagator::initFieldFromGRP(mO2GrpPath.c_str());
  auto field = static_cast<o2::field::MagneticField*>(TGeoGlobalMagField::Instance()->GetField());
  double orig[3] = { 0., 0., 0. };
  bz = field->getBz(orig);

  o2::base::GeometryManager::loadGeometry();
  mGeom = o2::its::GeometryTGeo::Instance();
}

void ITSTrackSimTask::startOfActivity(Activity& activity)
{
  mRunNumber = activity.mId;
  ILOG(Info, Support) << "startOfActivity" << ENDM;
}

void ITSTrackSimTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void ITSTrackSimTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  ILOG(Info, Support) << "START DOING QC General" << ENDM;
  o2::steer::MCKinematicsReader reader(mCollisionsContextPath.c_str());
  info.resize(reader.getNEvents(0));
  for (int i = 0; i < reader.getNEvents(0); ++i) {
    std::vector<MCTrack> const& mcArr = reader.getTracks(i);
    info[i].resize(mcArr.size());
  }

  auto clusArr = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("compclus"); // used to get hit information
  auto clusLabArr = ctx.inputs().get<const dataformats::MCTruthContainer<MCCompLabel>*>("mcclustruth").release();

  for (int iCluster = 0; iCluster < clusArr.size(); iCluster++) {

    auto lab = (clusLabArr->getLabels(iCluster))[0];
    if (!lab.isValid() || lab.getSourceID() != 0)
      continue;

    int TrackID = lab.getTrackID();

    if (TrackID < 0) {
      continue;
    }

    if (!lab.isCorrect())
      continue;
    const auto& Cluster = (clusArr)[iCluster];
    unsigned short& ok = info[lab.getEventID()][lab.getTrackID()].clusters; // bitmask with track hits at each layer
    auto layer = mGeom->getLayer(Cluster.getSensorID());
    float r = 0.f;
    if (layer == 0)
      ok |= 0b1;
    if (layer == 1)
      ok |= 0b10;
    if (layer == 2)
      ok |= 0b100;
    if (layer == 3)
      ok |= 0b1000;
    if (layer == 4)
      ok |= 0b10000;
    if (layer == 5)
      ok |= 0b100000;
    if (layer == 6)
      ok |= 0b1000000;
  }

  for (int i = 0; i < reader.getNEvents(0); ++i) {
    std::vector<MCTrack> const& mcArr = reader.getTracks(i);
    auto mcHeader = reader.getMCEventHeader(0, i); // SourceID=0 for ITS

    for (int mc = 0; mc < mcArr.size(); mc++) {

      const auto& mcTrack = (mcArr)[mc];

      info[i][mc].isFilled = false;
      if (mcTrack.Vx() * mcTrack.Vx() + mcTrack.Vy() * mcTrack.Vy() > 1)
        continue;
      if (TMath::Abs(mcTrack.GetPdgCode()) != 211)
        continue; // Select pions
      if (TMath::Abs(mcTrack.GetEta()) > 1.2)
        continue;
      if (info[i][mc].clusters != 0b1111111)
        continue;

      Double_t distance = sqrt(pow(mcHeader.GetX() - mcTrack.Vx(), 2) + pow(mcHeader.GetY() - mcTrack.Vy(), 2) + pow(mcHeader.GetZ() - mcTrack.Vz(), 2));
      info[i][mc].isFilled = true;
      info[i][mc].r = distance;
      info[i][mc].pt = mcTrack.GetPt();
      info[i][mc].eta = mcTrack.GetEta();
      info[i][mc].phi = mcTrack.GetPhi();
      info[i][mc].z = mcTrack.Vz();
      info[i][mc].isPrimary = mcTrack.isPrimary();
      if (mcTrack.isPrimary()) {
        hPrimaryGen_pt->Fill(mcTrack.GetPt());
      }

      hDenTrue_r->Fill(distance);
      hDenTrue_pt->Fill(mcTrack.GetPt());
      hDenTrue_eta->Fill(mcTrack.GetEta());
      hDenTrue_phi->Fill(mcTrack.GetPhi());
      hDenTrue_z->Fill(mcTrack.Vz());
    }
  }

  auto trackArr = ctx.inputs().get<gsl::span<o2::its::TrackITS>>("tracks"); // MC Tracks
  auto MCTruth = ctx.inputs().get<gsl::span<o2::MCCompLabel>>("mstruth");   // MC track label, contains info about EventID, TrackID, SourceID etc

  for (int itrack = 0; itrack < trackArr.size(); itrack++) {
    const auto& track = trackArr[itrack];
    const auto& MCinfo = MCTruth[itrack];
    if (MCinfo.isNoise())
      continue;
    Float_t ip[2]{ 0., 0. };
    Float_t vx = 0., vy = 0., vz = 0.; // Assumed primary vertex at 0,0,0
    track.getImpactParams(vx, vy, vz, bz, ip);

    hAngularDistribution->Fill(track.getEta(), track.getPhi());

    if (info[MCinfo.getEventID()][MCinfo.getTrackID()].isFilled) {

      if (info[MCinfo.getEventID()][MCinfo.getTrackID()].isReco != 0) {
        hNumDuplicate_pt->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].pt);
        hNumDuplicate_phi->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].phi);
        hNumDuplicate_eta->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].eta);
        hNumDuplicate_z->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].z);
        hNumDuplicate_r->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].r);
        continue;
      }
      info[MCinfo.getEventID()][MCinfo.getTrackID()].isReco++;

      if (MCinfo.isFake()) {

        hNumRecoFake_pt->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].pt);
        hNumRecoFake_phi->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].phi);
        hNumRecoFake_eta->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].eta);
        hNumRecoFake_z->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].z);
        hNumRecoFake_r->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].r);
        if (info[MCinfo.getEventID()][MCinfo.getTrackID()].isPrimary) {
          hTrackImpactTransvFake->Fill(ip[0]);
          hPrimaryReco_pt->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].pt);
        }
      } else {
        hNumRecoValid_pt->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].pt);
        hNumRecoValid_phi->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].phi);
        hNumRecoValid_eta->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].eta);
        hNumRecoValid_z->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].z);
        hNumRecoValid_r->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].r);
        if (info[MCinfo.getEventID()][MCinfo.getTrackID()].isPrimary) {
          hTrackImpactTransvValid->Fill(ip[0]);
          hPrimaryReco_pt->Fill(info[MCinfo.getEventID()][MCinfo.getTrackID()].pt);
        }
      }
    }
  }
  hEfficiency_pt->SetPassedHistogram(*hNumRecoValid_pt, "f");
  hEfficiency_pt->SetTotalHistogram(*hDenTrue_pt, "f");
  hFakeTrack_pt->SetPassedHistogram(*hNumRecoFake_pt, "f");
  hFakeTrack_pt->SetTotalHistogram(*hDenTrue_pt, "f");
  hDuplicate_pt->SetPassedHistogram(*hNumDuplicate_pt, "f");
  hDuplicate_pt->SetTotalHistogram(*hDenTrue_pt, "f");

  hFakeTrack_phi->SetPassedHistogram(*hNumRecoFake_phi, "f");
  hFakeTrack_phi->SetTotalHistogram(*hDenTrue_phi, "f");
  hEfficiency_phi->SetPassedHistogram(*hNumRecoValid_phi, "f");
  hEfficiency_phi->SetTotalHistogram(*hDenTrue_phi, "f");
  hDuplicate_phi->SetPassedHistogram(*hNumDuplicate_phi, "f");
  hDuplicate_phi->SetTotalHistogram(*hDenTrue_phi, "f");

  hFakeTrack_eta->SetPassedHistogram(*hNumRecoFake_eta, "f");
  hFakeTrack_eta->SetTotalHistogram(*hDenTrue_eta, "f");
  hEfficiency_eta->SetPassedHistogram(*hNumRecoValid_eta, "f");
  hEfficiency_eta->SetTotalHistogram(*hDenTrue_eta, "f");
  hDuplicate_eta->SetPassedHistogram(*hNumDuplicate_eta, "f");
  hDuplicate_eta->SetTotalHistogram(*hDenTrue_eta, "f");

  hFakeTrack_r->SetPassedHistogram(*hNumRecoFake_r, "f");
  hFakeTrack_r->SetTotalHistogram(*hDenTrue_r, "f");
  hEfficiency_r->SetPassedHistogram(*hNumRecoValid_r, "f");
  hEfficiency_r->SetTotalHistogram(*hDenTrue_r, "f");
  hDuplicate_r->SetPassedHistogram(*hNumDuplicate_r, "f");
  hDuplicate_r->SetTotalHistogram(*hDenTrue_r, "f");

  hFakeTrack_z->SetPassedHistogram(*hNumRecoFake_z, "f");
  hFakeTrack_z->SetTotalHistogram(*hDenTrue_z, "f");
  hEfficiency_z->SetPassedHistogram(*hNumRecoValid_z, "f");
  hEfficiency_z->SetTotalHistogram(*hDenTrue_z, "f");
  hDuplicate_z->SetPassedHistogram(*hNumDuplicate_z, "f");
  hDuplicate_z->SetTotalHistogram(*hDenTrue_z, "f");
}

void ITSTrackSimTask::endOfCycle()
{

  for (unsigned int iObj = 0; iObj < mPublishedObjects.size(); iObj++)
    getObjectsManager()->addMetadata(mPublishedObjects.at(iObj)->GetName(), "Run", std::to_string(mRunNumber));
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void ITSTrackSimTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ITSTrackSimTask::reset()
{
  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  hNumRecoValid_pt->Reset();
  hNumRecoFake_pt->Reset();
  hDenTrue_pt->Reset();

  hNumRecoValid_phi->Reset();
  hNumRecoFake_phi->Reset();
  hDenTrue_phi->Reset();

  hNumRecoValid_eta->Reset();
  hNumRecoFake_eta->Reset();
  hDenTrue_eta->Reset();

  hNumRecoValid_r->Reset();
  hNumRecoFake_r->Reset();
  hDenTrue_r->Reset();

  hNumRecoValid_z->Reset();
  hNumRecoFake_z->Reset();
  hDenTrue_z->Reset();

  hTrackImpactTransvValid->Reset();
  hTrackImpactTransvFake->Reset();

  hPrimaryGen_pt->Reset();
  hPrimaryReco_pt->Reset();

  hAngularDistribution->Reset();
  hNumDuplicate_pt->Reset();
}

void ITSTrackSimTask::createAllHistos()
{
  const Int_t nb = 100;
  Double_t xbins[nb + 1], ptcutl = 0.01, ptcuth = 10.;
  Double_t a = TMath::Log(ptcuth / ptcutl) / nb;
  for (Int_t i = 0; i <= nb; i++)
    xbins[i] = ptcutl * TMath::Exp(i * a);

  hDuplicate_pt = new TEfficiency("Duplicate_pt", "#it{p}_{T} fraction of mother duplicate track;#it{p}_{T} (GeV/#it{c});Fraction", nb, xbins);
  addObject(hDuplicate_pt);
  hEfficiency_pt = new TEfficiency("efficiency_pt", "#it{p}_{T} efficiency of tracking; #it{p}_{T} (GeV/#it{c}); {p}_{T}Efficiency", nb, xbins);
  addObject(hEfficiency_pt);
  hFakeTrack_pt = new TEfficiency("faketrack_pt", "#it{p}_{T} fake-track rate;#it{p}_{T} (GeV/#it{c});Fake-track rate", nb, xbins);
  addObject(hFakeTrack_pt);
  hNumDuplicate_pt = new TH1D("NumDuplicate_pt", "", nb, xbins);
  hNumRecoValid_pt = new TH1D("NumRecoValid_pt", "", nb, xbins);
  hNumRecoFake_pt = new TH1D("NumRecoFake_pt", "", nb, xbins);
  hDenTrue_pt = new TH1D("DenTrueMC_pt", "", nb, xbins);

  hEfficiency_phi = new TEfficiency("efficiency_phi", "#phi efficiency of tracking;#phi;Efficiency", 60, 0, TMath::TwoPi());
  addObject(hEfficiency_phi);
  hFakeTrack_phi = new TEfficiency("faketrack_phi", "#phi fake-track rate;#phi;Fake-track rate", 60, 0, TMath::TwoPi());
  addObject(hFakeTrack_phi);
  hDuplicate_phi = new TEfficiency("Duplicate_phi", "#phi fraction of mother duplicate track;#phi;Fraction", 60, 0, TMath::TwoPi());
  addObject(hDuplicate_phi);
  hNumDuplicate_phi = new TH1D("NumDuplicate_phi", "", 60, 0, TMath::TwoPi());
  hNumRecoValid_phi = new TH1D("NumRecoValid_phi", "", 60, 0, TMath::TwoPi());
  hNumRecoFake_phi = new TH1D("NumRecoFake_phi", "", 60, 0, TMath::TwoPi());
  hDenTrue_phi = new TH1D("DenTrueMC_phi", "", 60, 0, TMath::TwoPi());

  hEfficiency_eta = new TEfficiency("efficiency_eta", "#eta efficiency of tracking;#eta;Efficiency", 30, -1.5, 1.5);
  addObject(hEfficiency_eta);
  hFakeTrack_eta = new TEfficiency("faketrack_eta", "#eta fake-track rate;#eta;Fake-track rate", 30, -1.5, 1.5);
  addObject(hFakeTrack_eta);
  hDuplicate_eta = new TEfficiency("Duplicate_eta", "#eta fraction of mother duplicate track;#eta;Fraction", 30, -1.5, 1.5);
  addObject(hDuplicate_eta);
  hNumDuplicate_eta = new TH1D("NumDuplicate_eta", "", 30, -1.5, 1.5);
  hNumRecoValid_eta = new TH1D("NumRecoValid_eta", "", 30, -1.5, 1.5);
  hNumRecoFake_eta = new TH1D("NumRecoFake_eta", "", 30, -1.5, 1.5);
  hDenTrue_eta = new TH1D("DenTrueMC_eta", "", 30, -1.5, 1.5);

  hEfficiency_r = new TEfficiency("efficiency_r", "r efficiency of tracking;r (cm);Efficiency", 100, 0, 5);
  addObject(hEfficiency_r);
  hFakeTrack_r = new TEfficiency("faketrack_r", "r fake-track rate;r (cm);Fake-track rate", 100, 0, 5);
  addObject(hFakeTrack_r);
  hDuplicate_r = new TEfficiency("Duplicate_r", "r fraction of mother duplicate track;r (cm);Fraction", 100, 0, 5);
  addObject(hDuplicate_r);
  hNumRecoValid_r = new TH1D("NumRecoValid_r", "", 100, 0, 5);
  hNumRecoFake_r = new TH1D("NumRecoFake_r", "", 100, 0, 5);
  hNumDuplicate_r = new TH1D("NumDuplicate_r", "", 100, 0, 5);
  hDenTrue_r = new TH1D("DenTrueMC_r", "", 100, 0, 5);

  hEfficiency_z = new TEfficiency("efficiency_z", "z efficiency of tracking;z (cm);Efficiency", 101, -5, 5);
  addObject(hEfficiency_z);
  hFakeTrack_z = new TEfficiency("faketrack_z", "z fake-track rate;z (cm);Fake-track rate", 101, -5, 5);
  addObject(hFakeTrack_z);
  hDuplicate_z = new TEfficiency("Duplicate_z", "z fraction of mother duplicate track;z (cm);Fraction", 101, -5, 5);
  addObject(hDuplicate_z);
  hNumRecoValid_z = new TH1D("NumRecoValid_z", "", 101, -5, 5);
  hNumRecoFake_z = new TH1D("NumRecoFake_z", "", 101, -5, 5);
  hNumDuplicate_z = new TH1D("NumDuplicate_z", "", 101, -5, 5);
  hDenTrue_z = new TH1D("DenTrueMC_z", "", 101, -5, 5);

  hTrackImpactTransvValid = new TH1F("ImpactTransvVaild", "Transverse impact parameter for valid primary tracks; D (cm)", 60, -0.1, 0.1);
  hTrackImpactTransvValid->SetTitle("Transverse impact parameter distribution of valid primary tracks");
  addObject(hTrackImpactTransvValid);
  formatAxes(hTrackImpactTransvValid, "D (cm)", "counts", 1, 1.10);

  hTrackImpactTransvFake = new TH1F("ImpactTransvFake", "Transverse impact parameter for fake primary tracks; D (cm)", 60, -0.1, 0.1);
  hTrackImpactTransvFake->SetTitle("Transverse impact parameter distribution of fake primary tracks");
  addObject(hTrackImpactTransvFake);
  formatAxes(hTrackImpactTransvFake, "D (cm)", "counts", 1, 1.10);

  hPrimaryGen_pt = new TH1D("PrimaryGen_pt", ";#it{p}_{T} (GeV/#it{c});counts", nb, xbins);
  hPrimaryGen_pt->SetTitle("#it{p}_{T} of primary generated particles");
  addObject(hPrimaryGen_pt);
  formatAxes(hPrimaryGen_pt, "#it{p}_{T} (GeV/#it{c})", "counts", 1, 1.10);

  hPrimaryReco_pt = new TH1D("PrimaryReco_pt", ";#it{p}_{T} (GeV/#it{c});counts", nb, xbins);
  hPrimaryReco_pt->SetTitle("#it{p}_{T} of primary reconstructed particles");
  addObject(hPrimaryReco_pt);
  formatAxes(hPrimaryReco_pt, "#it{p}_{T} (GeV/#it{c})", "counts", 1, 1.10);

  hAngularDistribution = new TH2D("AngularDistribution", "AngularDistribution", 30, -1.5, 1.5, 60, 0, TMath::TwoPi());
  hAngularDistribution->SetTitle("AngularDistribution");
  addObject(hAngularDistribution);
  formatAxes(hAngularDistribution, "#eta", "#phi", 1, 1.10);
  hAngularDistribution->SetStats(0);
}

void ITSTrackSimTask::addObject(TObject* aObject)
{
  if (!aObject) {
    ILOG(Info, Support) << " ERROR: trying to add non-existent object " << ENDM;
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
    ILOG(Info, Support) << " Object will be published: " << mPublishedObjects.at(iObj)->GetName() << ENDM;
  }
}

} // namespace o2::quality_control_modules::its
