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
#include <unordered_map>
#include <algorithm>
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
  delete hEfficiency_pt;

  delete hNumRecoValid_eta;
  delete hEfficiency_eta;

  delete hNumRecoValid_phi;
  delete hEfficiency_phi;

  delete hNumRecoValid_r;
  delete hEfficiency_r;

  delete hNumRecoValid_z;
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

  for (Int_t icls = 0; icls < 4; icls++) {
    delete hNumRecoFake_pt[icls];
    delete hDenTrue_pt[icls];
    delete hFakeTrack_pt[icls];
    delete hNumRecoFake_eta[icls];
    delete hDenTrue_eta[icls];
    delete hFakeTrack_eta[icls];
    delete hNumRecoFake_phi[icls];
    delete hDenTrue_phi[icls];
    delete hFakeTrack_phi[icls];
    delete hNumRecoFake_r[icls];
    delete hDenTrue_r[icls];
    delete hFakeTrack_r[icls];
    delete hNumRecoFake_z[icls];
    delete hDenTrue_z[icls];
    delete hFakeTrack_z[icls];
    delete hNumRecoFake_QoverPt[icls];
    delete hDenTrue_QoverPt[icls];
    delete hFakeTrack_QoverPt[icls];
  }
}

void ITSTrackSimTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize ITSTrackSimTask" << ENDM;
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

void ITSTrackSimTask::startOfActivity(const Activity& activity)
{
  mRunNumber = activity.mId;
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
}

void ITSTrackSimTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void ITSTrackSimTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  ILOG(Debug, Devel) << "START DOING QC General" << ENDM;
  o2::steer::MCKinematicsReader reader(mCollisionsContextPath.c_str());
  const int nEvents = reader.getNEvents(0);

  auto clusArr = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("compclus"); // used to get hit information
  auto clusLabArr = ctx.inputs().get<const dataformats::MCTruthContainer<MCCompLabel>*>("mcclustruth");

  // layers with a cluster of an MC track, as a bit mask; indexed [event][track], sized from the labels only
  std::vector<std::vector<unsigned short>> layerMask(nEvents);
  {
    std::vector<int> nTracks(nEvents, 0);
    for (int iCluster = 0; iCluster < (int)clusArr.size(); iCluster++) {
      auto lab = (clusLabArr->getLabels(iCluster))[0];
      if (!lab.isValid() || lab.getSourceID() != 0 || lab.getTrackID() < 0 || lab.getEventID() >= nEvents || !lab.isCorrect()) {
        continue;
      }
      nTracks[lab.getEventID()] = std::max(nTracks[lab.getEventID()], lab.getTrackID() + 1);
    }
    for (int i = 0; i < nEvents; ++i) {
      layerMask[i].assign(nTracks[i], 0);
    }
  }
  for (int iCluster = 0; iCluster < (int)clusArr.size(); iCluster++) {
    auto lab = (clusLabArr->getLabels(iCluster))[0];
    if (!lab.isValid() || lab.getSourceID() != 0 || lab.getTrackID() < 0 || lab.getEventID() >= nEvents || !lab.isCorrect()) {
      continue;
    }
    const auto layer = mGeom->getLayer(clusArr[iCluster].getSensorID());
    if (layer >= 0 && layer <= 6) {
      layerMask[lab.getEventID()][lab.getTrackID()] |= (1 << layer); // bitmask with track hits at each layer
    }
  }

  // MC truth of the tracks passing the selection, keyed by (event, track); the kinematics are read one event at a time
  struct SelectedTrack {
    float r, pt, eta, phi, z;
    bool isPrimary;
    int isReco = 0;
  };
  std::unordered_map<uint64_t, SelectedTrack> selected;
  auto keyOf = [](int event, int track) { return (uint64_t(uint32_t(event)) << 32) | uint32_t(track); };

  for (int i = 0; i < nEvents; ++i) {
    {
      std::vector<MCTrack> const& mcArr = reader.getTracks(i);
      auto mcHeader = reader.getMCEventHeader(0, i); // SourceID=0 for ITS
      const auto& mask = layerMask[i];

      for (int mc = 0; mc < (int)mcArr.size(); mc++) {
        const auto& mcTrack = (mcArr)[mc];

        if (mc >= (int)mask.size() || mask[mc] != 0b1111111)
          continue;
        if (mcTrack.Vx() * mcTrack.Vx() + mcTrack.Vy() * mcTrack.Vy() > 1)
          continue;
        if (TMath::Abs(mcTrack.GetPdgCode()) != 211)
          continue; // Select pions
        if (TMath::Abs(mcTrack.GetEta()) > 1.2)
          continue;
        Double_t distance = sqrt(pow(mcHeader.GetX() - mcTrack.Vx(), 2) + pow(mcHeader.GetY() - mcTrack.Vy(), 2) + pow(mcHeader.GetZ() - mcTrack.Vz(), 2));
        SelectedTrack& sel = selected[keyOf(i, mc)];
        sel.r = distance;
        sel.pt = mcTrack.GetPt();
        sel.eta = mcTrack.GetEta();
        sel.phi = mcTrack.GetPhi();
        sel.z = mcTrack.Vz();
        sel.isPrimary = mcTrack.isPrimary();
        if (mcTrack.isPrimary()) {
          hPrimaryGen_pt->Fill(mcTrack.GetPt());
          // True Generated primaries: denominator of the efficiency plots
          hDenTrue_r[4]->Fill(distance);
          hDenTrue_pt[4]->Fill(mcTrack.GetPt());
          hDenTrue_eta[4]->Fill(mcTrack.GetEta());
          hDenTrue_phi[4]->Fill(mcTrack.GetPhi());
          hDenTrue_z[4]->Fill(mcTrack.Vz());
        }
      }
    }
    reader.releaseTracksForSourceAndEvent(0, i); // the tracks of this event are not needed any more
    std::vector<unsigned short>().swap(layerMask[i]);
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
    Int_t iNClusters = track.getNumberOfClusters();

    hAngularDistribution->Fill(track.getEta(), track.getPhi());

    auto selIt = selected.find(keyOf(MCinfo.getEventID(), MCinfo.getTrackID()));
    if (selIt != selected.end()) {
      SelectedTrack& rec = selIt->second;

      if (rec.isPrimary) {
        // True Generated primaries for QoverPt plot, because MCTrack does not have charge function
        hDenTrue_QoverPt[4]->Fill(track.getSign() / rec.pt);
        if (iNClusters == 4) {
          hDenTrue_QoverPt[0]->Fill(track.getSign() / rec.pt);
          hDenTrue_r[0]->Fill(rec.r);
          hDenTrue_pt[0]->Fill(rec.pt);
          hDenTrue_eta[0]->Fill(rec.eta);
          hDenTrue_phi[0]->Fill(rec.phi);
          hDenTrue_z[0]->Fill(rec.z);
        } else if (iNClusters == 5) {
          hDenTrue_QoverPt[1]->Fill(track.getSign() / rec.pt);
          hDenTrue_r[1]->Fill(rec.r);
          hDenTrue_pt[1]->Fill(rec.pt);
          hDenTrue_eta[1]->Fill(rec.eta);
          hDenTrue_phi[1]->Fill(rec.phi);
          hDenTrue_z[1]->Fill(rec.z);
        } else if (iNClusters == 6) {
          hDenTrue_QoverPt[2]->Fill(track.getSign() / rec.pt);
          hDenTrue_r[2]->Fill(rec.r);
          hDenTrue_pt[2]->Fill(rec.pt);
          hDenTrue_eta[2]->Fill(rec.eta);
          hDenTrue_phi[2]->Fill(rec.phi);
          hDenTrue_z[2]->Fill(rec.z);
        } else if (iNClusters == 7) {
          hDenTrue_QoverPt[3]->Fill(track.getSign() / rec.pt);
          hDenTrue_r[3]->Fill(rec.r);
          hDenTrue_pt[3]->Fill(rec.pt);
          hDenTrue_eta[3]->Fill(rec.eta);
          hDenTrue_phi[3]->Fill(rec.phi);
          hDenTrue_z[3]->Fill(rec.z);
        }
      }

      if (rec.isReco != 0) {
        hNumDuplicate_pt->Fill(rec.pt);
        hNumDuplicate_phi->Fill(rec.phi);
        hNumDuplicate_eta->Fill(rec.eta);
        hNumDuplicate_z->Fill(rec.z);
        hNumDuplicate_r->Fill(rec.r);
        continue;
      }
      rec.isReco++;

      if (MCinfo.isFake()) {
        hNumRecoFake_pt[4]->Fill(rec.pt);
        hNumRecoFake_phi[4]->Fill(rec.phi);
        hNumRecoFake_eta[4]->Fill(rec.eta);
        hNumRecoFake_z[4]->Fill(rec.z);
        hNumRecoFake_r[4]->Fill(rec.r);
        hNumRecoFake_QoverPt[4]->Fill(track.getSign() / rec.pt);
        if (iNClusters == 4) {
          hNumRecoFake_pt[0]->Fill(rec.pt);
          hNumRecoFake_phi[0]->Fill(rec.phi);
          hNumRecoFake_eta[0]->Fill(rec.eta);
          hNumRecoFake_z[0]->Fill(rec.z);
          hNumRecoFake_r[0]->Fill(rec.r);
          hNumRecoFake_QoverPt[0]->Fill(track.getSign() / rec.pt);
        } else if (iNClusters == 5) {
          hNumRecoFake_pt[1]->Fill(rec.pt);
          hNumRecoFake_phi[1]->Fill(rec.phi);
          hNumRecoFake_eta[1]->Fill(rec.eta);
          hNumRecoFake_z[1]->Fill(rec.z);
          hNumRecoFake_r[1]->Fill(rec.r);
          hNumRecoFake_QoverPt[1]->Fill(track.getSign() / rec.pt);
        } else if (iNClusters == 6) {
          hNumRecoFake_pt[2]->Fill(rec.pt);
          hNumRecoFake_phi[2]->Fill(rec.phi);
          hNumRecoFake_eta[2]->Fill(rec.eta);
          hNumRecoFake_z[2]->Fill(rec.z);
          hNumRecoFake_r[2]->Fill(rec.r);
          hNumRecoFake_QoverPt[2]->Fill(track.getSign() / rec.pt);
        } else if (iNClusters == 7) {
          hNumRecoFake_pt[3]->Fill(rec.pt);
          hNumRecoFake_phi[3]->Fill(rec.phi);
          hNumRecoFake_eta[3]->Fill(rec.eta);
          hNumRecoFake_z[3]->Fill(rec.z);
          hNumRecoFake_r[3]->Fill(rec.r);
          hNumRecoFake_QoverPt[3]->Fill(track.getSign() / rec.pt);
        }

        if (rec.isPrimary) {
          hTrackImpactTransvFake->Fill(ip[0]);
          hPrimaryReco_pt->Fill(rec.pt);
        }
      } else {
        if (rec.isPrimary) {
          // True primaries reconstructed: numerator of efficiency plots
          hNumRecoValid_pt->Fill(rec.pt);
          hNumRecoValid_phi->Fill(rec.phi);
          hNumRecoValid_eta->Fill(rec.eta);
          hNumRecoValid_z->Fill(rec.z);
          hNumRecoValid_r->Fill(rec.r);

          hTrackImpactTransvValid->Fill(ip[0]);
          hPrimaryReco_pt->Fill(rec.pt);
        }
      }
    }
  }
  hEfficiency_pt->SetPassedHistogram(*hNumRecoValid_pt, "f");
  hEfficiency_pt->SetTotalHistogram(*hDenTrue_pt[4], "f");
  hDuplicate_pt->SetPassedHistogram(*hNumDuplicate_pt, "f");
  hDuplicate_pt->SetTotalHistogram(*hDenTrue_pt[4], "f");

  hEfficiency_phi->SetPassedHistogram(*hNumRecoValid_phi, "f");
  hEfficiency_phi->SetTotalHistogram(*hDenTrue_phi[4], "f");
  hDuplicate_phi->SetPassedHistogram(*hNumDuplicate_phi, "f");
  hDuplicate_phi->SetTotalHistogram(*hDenTrue_phi[4], "f");

  hEfficiency_eta->SetPassedHistogram(*hNumRecoValid_eta, "f");
  hEfficiency_eta->SetTotalHistogram(*hDenTrue_eta[4], "f");
  hDuplicate_eta->SetPassedHistogram(*hNumDuplicate_eta, "f");
  hDuplicate_eta->SetTotalHistogram(*hDenTrue_eta[4], "f");

  hEfficiency_r->SetPassedHistogram(*hNumRecoValid_r, "f");
  hEfficiency_r->SetTotalHistogram(*hDenTrue_r[4], "f");
  hDuplicate_r->SetPassedHistogram(*hNumDuplicate_r, "f");
  hDuplicate_r->SetTotalHistogram(*hDenTrue_r[4], "f");

  hEfficiency_z->SetPassedHistogram(*hNumRecoValid_z, "f");
  hEfficiency_z->SetTotalHistogram(*hDenTrue_z[4], "f");
  hDuplicate_z->SetPassedHistogram(*hNumDuplicate_z, "f");
  hDuplicate_z->SetTotalHistogram(*hDenTrue_z[4], "f");

  for (Int_t icls = 0; icls < 5; icls++) {
    hFakeTrack_pt[icls]->SetPassedHistogram(*hNumRecoFake_pt[icls], "f");
    hFakeTrack_pt[icls]->SetTotalHistogram(*hDenTrue_pt[icls], "f");
    hFakeTrack_phi[icls]->SetPassedHistogram(*hNumRecoFake_phi[icls], "f");
    hFakeTrack_phi[icls]->SetTotalHistogram(*hDenTrue_phi[icls], "f");
    hFakeTrack_eta[icls]->SetPassedHistogram(*hNumRecoFake_eta[icls], "f");
    hFakeTrack_eta[icls]->SetTotalHistogram(*hDenTrue_eta[icls], "f");
    hFakeTrack_r[icls]->SetPassedHistogram(*hNumRecoFake_r[icls], "f");
    hFakeTrack_r[icls]->SetTotalHistogram(*hDenTrue_r[icls], "f");
    hFakeTrack_z[icls]->SetPassedHistogram(*hNumRecoFake_z[icls], "f");
    hFakeTrack_z[icls]->SetTotalHistogram(*hDenTrue_z[icls], "f");
    hFakeTrack_QoverPt[icls]->SetPassedHistogram(*hNumRecoFake_QoverPt[icls], "f");
    hFakeTrack_QoverPt[icls]->SetTotalHistogram(*hDenTrue_QoverPt[icls], "f");
  }
}

void ITSTrackSimTask::endOfCycle()
{

  for (unsigned int iObj = 0; iObj < mPublishedObjects.size(); iObj++)
    getObjectsManager()->addMetadata(mPublishedObjects.at(iObj)->GetName(), "Run", std::to_string(mRunNumber));
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void ITSTrackSimTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ITSTrackSimTask::reset()
{
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  hNumRecoValid_pt->Reset();
  hNumRecoValid_phi->Reset();
  hNumRecoValid_eta->Reset();
  hNumRecoValid_r->Reset();
  hNumRecoValid_z->Reset();

  hTrackImpactTransvValid->Reset();
  hTrackImpactTransvFake->Reset();

  hPrimaryGen_pt->Reset();
  hPrimaryReco_pt->Reset();

  hAngularDistribution->Reset();
  hNumDuplicate_pt->Reset();

  for (Int_t icls = 0; icls < 5; icls++) {
    hNumRecoFake_pt[icls]->Reset();
    hDenTrue_pt[icls]->Reset();
    hNumRecoFake_phi[icls]->Reset();
    hDenTrue_phi[icls]->Reset();
    hNumRecoFake_eta[icls]->Reset();
    hDenTrue_eta[icls]->Reset();
    hNumRecoFake_r[icls]->Reset();
    hDenTrue_r[icls]->Reset();
    hNumRecoFake_z[icls]->Reset();
    hDenTrue_z[icls]->Reset();
    hNumRecoFake_QoverPt[icls]->Reset();
    hDenTrue_QoverPt[icls]->Reset();
  }
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
  hEfficiency_pt = new TEfficiency("efficiency_pt", "Primary pions with 7cls - tracking efficiency vs #it{p}_{T}; #it{p}_{T} (GeV/#it{c}); {p}_{T}Efficiency", nb, xbins);
  addObject(hEfficiency_pt);
  hNumDuplicate_pt = new TH1D("NumDuplicate_pt", "", nb, xbins);
  hNumRecoValid_pt = new TH1D("NumRecoValid_pt", "", nb, xbins);
  hFakeTrack_pt[4] = new TEfficiency("faketrack_pt", "#it{p}_{T} fake-track rate;#it{p}_{T} (GeV/#it{c});Fake-track rate", nb, xbins);
  addObject(hFakeTrack_pt[4]);
  hNumRecoFake_pt[4] = new TH1D("NumRecoFake_pt", "", nb, xbins);
  hDenTrue_pt[4] = new TH1D("DenTrueMC_pt", "", nb, xbins);

  hEfficiency_phi = new TEfficiency("efficiency_phi", "Primary pions with 7cls - tracking efficiency vs #phi;#phi;Efficiency", 60, 0, TMath::TwoPi());
  addObject(hEfficiency_phi);
  hDuplicate_phi = new TEfficiency("Duplicate_phi", "#phi fraction of mother duplicate track;#phi;Fraction", 60, 0, TMath::TwoPi());
  addObject(hDuplicate_phi);
  hNumDuplicate_phi = new TH1D("NumDuplicate_phi", "", 60, 0, TMath::TwoPi());
  hNumRecoValid_phi = new TH1D("NumRecoValid_phi", "", 60, 0, TMath::TwoPi());
  hFakeTrack_phi[4] = new TEfficiency("faketrack_phi", "#phi fake-track rate;#phi;Fake-track rate", 60, 0, TMath::TwoPi());
  addObject(hFakeTrack_phi[4]);
  hNumRecoFake_phi[4] = new TH1D("NumRecoFake_phi", "_4Cluster", 60, 0, TMath::TwoPi());
  hDenTrue_phi[4] = new TH1D("DenTrueMC_phi", "", 60, 0, TMath::TwoPi());

  hEfficiency_eta = new TEfficiency("efficiency_eta", "Primary pions with 7 cls - tracking efficiency vs #eta;#eta;Efficiency", 30, -1.5, 1.5);
  addObject(hEfficiency_eta);
  hDuplicate_eta = new TEfficiency("Duplicate_eta", "#eta fraction of mother duplicate track;#eta;Fraction", 30, -1.5, 1.5);
  addObject(hDuplicate_eta);
  hNumDuplicate_eta = new TH1D("NumDuplicate_eta", "", 30, -1.5, 1.5);
  hNumRecoValid_eta = new TH1D("NumRecoValid_eta", "", 30, -1.5, 1.5);
  hFakeTrack_eta[4] = new TEfficiency("faketrack_eta", "#eta fake-track rate;#eta;Fake-track rate", 30, -1.5, 1.5);
  addObject(hFakeTrack_eta[4]);
  hNumRecoFake_eta[4] = new TH1D("NumRecoFake_eta", "", 30, -1.5, 1.5);
  hDenTrue_eta[4] = new TH1D("DenTrueMC_eta", "", 30, -1.5, 1.5);

  hEfficiency_r = new TEfficiency("efficiency_r", "Primary pions with 7 cls - tracking efficiency vs r;r (cm);Efficiency", 100, 0, 5);
  addObject(hEfficiency_r);
  hDuplicate_r = new TEfficiency("Duplicate_r", "r fraction of mother duplicate track;r (cm);Fraction", 100, 0, 5);
  addObject(hDuplicate_r);
  hNumRecoValid_r = new TH1D("NumRecoValid_r", "", 100, 0, 5);
  hNumDuplicate_r = new TH1D("NumDuplicate_r", "", 100, 0, 5);
  hFakeTrack_r[4] = new TEfficiency("faketrack_r", "r fake-track rate;r (cm);Fake-track rate", 100, 0, 5);
  addObject(hFakeTrack_r[4]);
  hNumRecoFake_r[4] = new TH1D("NumRecoFake_r", "", 100, 0, 5);
  hDenTrue_r[4] = new TH1D("DenTrueMC_r", "", 100, 0, 5);

  hEfficiency_z = new TEfficiency("efficiency_z", "Primary pions with 7 cls - tracking efficiency vs z;z (cm);Efficiency", 101, -5, 5);
  addObject(hEfficiency_z);
  hDuplicate_z = new TEfficiency("Duplicate_z", "z fraction of mother duplicate track;z (cm);Fraction", 101, -5, 5);
  addObject(hDuplicate_z);
  hNumRecoValid_z = new TH1D("NumRecoValid_z", "", 101, -5, 5);
  hNumDuplicate_z = new TH1D("NumDuplicate_z", "", 101, -5, 5);
  hFakeTrack_z[4] = new TEfficiency("faketrack_z", "z fake-track rate;z (cm);Fake-track rate", 101, -5, 5);
  addObject(hFakeTrack_z[4]);
  hNumRecoFake_z[4] = new TH1D("NumRecoFake_z", "", 101, -5, 5);
  hDenTrue_z[4] = new TH1D("DenTrueMC_z", "", 101, -5, 5);

  hFakeTrack_QoverPt[4] = new TEfficiency("faketrack_QoverPt", "#it{Q/p}_{T} fake-track rate;#it{Q/p}_{T} (GeV/#it{c});Fake-track rate", nb, xbins);
  addObject(hFakeTrack_QoverPt[4]);
  hNumRecoFake_QoverPt[4] = new TH1D("NumRecoFake_QoverPt", "", nb, xbins);
  hDenTrue_QoverPt[4] = new TH1D("DenTrueMC_QoverPt", "", nb, xbins);

  for (Int_t icls = 0; icls < 4; icls++) {
    hFakeTrack_pt[icls] = new TEfficiency(Form("faketrack_%dCluster_pt", icls + 4), Form("#it{p}_{T} fake-track rate %d cluster tracks;#it{p}_{T} (GeV/#it{c});Fake-track rate", icls + 4), nb, xbins);
    addObject(hFakeTrack_pt[icls]);
    hNumRecoFake_pt[icls] = new TH1D(Form("NumRecoFake_%dCluster_pt", icls + 4), "", nb, xbins);
    hDenTrue_pt[icls] = new TH1D(Form("DenTrueMC_%dCluster_pt", icls + 4), "", nb, xbins);
    hFakeTrack_phi[icls] = new TEfficiency(Form("faketrack_%dCluster_phi", icls + 4), Form("#phi fake-track rate %d cluster tracks;#phi;Fake-track rate", icls + 4), 60, 0, TMath::TwoPi());
    addObject(hFakeTrack_phi[icls]);
    hNumRecoFake_phi[icls] = new TH1D(Form("NumRecoFake_%dCluster_phi", icls + 4), "", 60, 0, TMath::TwoPi());
    hDenTrue_phi[icls] = new TH1D(Form("DenTrueMC_%dCluster_phi", icls + 4), "", 60, 0, TMath::TwoPi());
    hFakeTrack_eta[icls] = new TEfficiency(Form("faketrack_%dCluster_eta", icls + 4), Form("#eta fake-track rate %d cluster tracks;#eta;Fake-track rate", icls + 4), 30, -1.5, 1.5);
    addObject(hFakeTrack_eta[icls]);
    hNumRecoFake_eta[icls] = new TH1D(Form("NumRecoFake_%dCluster_eta", icls + 4), "", 30, -1.5, 1.5);
    hDenTrue_eta[icls] = new TH1D(Form("DenTrueMC_%dCluster_eta", icls + 4), "", 30, -1.5, 1.5);
    hFakeTrack_r[icls] = new TEfficiency(Form("faketrack_%dCluster_r", icls + 4), Form("r fake-track rate %d cluster tracks;r (cm);Fake-track rate", icls + 4), 100, 0, 5);
    addObject(hFakeTrack_r[icls]);
    hNumRecoFake_r[icls] = new TH1D(Form("NumRecoFake_%dCluster_r", icls + 4), "", 100, 0, 5);
    hDenTrue_r[icls] = new TH1D(Form("DenTrueMC_%dCluster_r", icls + 4), "", 100, 0, 5);
    hFakeTrack_z[icls] = new TEfficiency(Form("faketrack_%dCluster_z", icls + 4), Form("z fake-track rate %d cluster tracks;z (cm);Fake-track rate", icls + 4), 101, -5, 5);
    addObject(hFakeTrack_z[icls]);
    hNumRecoFake_z[icls] = new TH1D(Form("NumRecoFake_%dCluster_z", icls + 4), "", 101, -5, 5);
    hDenTrue_z[icls] = new TH1D(Form("DenTrueMC_%dCluster_z", icls + 4), "", 101, -5, 5);
    hFakeTrack_QoverPt[icls] = new TEfficiency(Form("faketrack_%dCluster_QoverPt", icls + 4), Form("#it{Q/p}_{T} fake-track rate %d cluster tracks;#it{Q/p}_{T} (GeV/#it{c});Fake-track rate", icls + 4), nb, xbins);
    addObject(hFakeTrack_QoverPt[icls]);
    hNumRecoFake_QoverPt[icls] = new TH1D(Form("NumRecoFake_%dCluster_QoverPt", icls + 4), "", nb, xbins);
    hDenTrue_QoverPt[icls] = new TH1D(Form("DenTrueMC_%dCluster_QoverPt", icls + 4), "", nb, xbins);
  }

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
    ILOG(Debug, Devel) << " ERROR: trying to add non-existent object " << ENDM;
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
    ILOG(Debug, Devel) << " Object will be published: " << mPublishedObjects.at(iObj)->GetName() << ENDM;
  }
}

} // namespace o2::quality_control_modules::its
