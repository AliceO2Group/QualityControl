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
/// \file   HmpidTaskMatches.cxx
/// \author Nicola Nicassio, Giacomo Volpe
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "QualityControl/QcInfoLogger.h"
#include "HMPID/HmpidTaskMatches.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/InputSpec.h>
#include "GlobalTrackingWorkflowHelpers/InputHelper.h"
#include "ReconstructionDataFormats/TrackTPCITS.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "DataFormatsGlobalTracking/RecoContainerCreateTracksVariadic.h"
#include "ReconstructionDataFormats/TrackParametrization.h"
#include "DetectorsBase/Propagator.h"

#include "DataFormatsHMP/Cluster.h"
#include "ReconstructionDataFormats/MatchInfoHMP.h"

namespace o2::quality_control_modules::hmpid
{

HmpidTaskMatches::HmpidTaskMatches() : TaskInterface() {}

HmpidTaskMatches::~HmpidTaskMatches()
{
  for (int iCh = 0; iCh < 7; iCh++) {
    delete mMatchInfoResidualsXTrackMIP[iCh];
    delete mMatchInfoResidualsYTrackMIP[iCh];
    delete mMatchInfoChargeClusterMIP[iCh];
    delete mMatchInfoChargeClusterPhotons[iCh];
    delete mMatchInfoXMip[iCh];
    delete mMatchInfoYMip[iCh];
    delete mMatchInfoXTrack[iCh];
    delete mMatchInfoYTrack[iCh];
    delete mMatchInfoClusterMIPMap[iCh];
    delete mMatchInfoThetaCherenkovVsMom[iCh];
  }
}

void HmpidTaskMatches::initialize(o2::framework::InitContext& /*ctx*/)
{
  mDataRequest = std::make_shared<o2::globaltracking::DataRequest>();
  mDataRequest->requestTracks(mSrc, false);

  BookHistograms();

  for (int iCh = 0; iCh < 7; iCh++) {

    getObjectsManager()->startPublishing(mMatchInfoResidualsXTrackMIP[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoResidualsYTrackMIP[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoChargeClusterMIP[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoChargeClusterPhotons[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoClusterMIPMap[iCh]);
    getObjectsManager()->setDefaultDrawOptions(mMatchInfoClusterMIPMap[iCh], "colz");
    getObjectsManager()->setDisplayHint(mMatchInfoClusterMIPMap[iCh], "colz");
    getObjectsManager()->startPublishing(mMatchInfoXMip[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoYMip[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoXTrack[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoXTrack[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoThetaCherenkovVsMom[iCh]);
  }
}

void HmpidTaskMatches::startOfActivity(const Activity& /*activity*/)
{
  reset();
}

void HmpidTaskMatches::startOfCycle()
{
}

void HmpidTaskMatches::monitorData(o2::framework::ProcessingContext& ctx)
{
  mRecoCont.collectData(ctx, *mDataRequest.get());

  // HMP
  mHMPMatches = mRecoCont.getHMPMatches();
  LOG(debug) << "We found " << mHMPMatches.size() << " Tracks matched to HMPID";

  // loop over HMPID MatchInfo
  for (const auto& matchHMP : mHMPMatches) { // match info loop

    int ch = matchHMP.getIdxHMPClus() / 1000000;

    float xMip, yMip, xTrk, yTrk, theta, phi;
    int q, nPhot;

    matchHMP.getHMPIDtrk(xTrk, yTrk, theta, phi);

    matchHMP.getHMPIDmip(xMip, yMip, q, nPhot);

    auto vPhotCharge = matchHMP.getPhotCharge();

    for (int i = 0; i < 10; i++) { // photon array loop
      if (vPhotCharge[i] > 0) {
        mMatchInfoChargeClusterPhotons[ch]->Fill(vPhotCharge[i]);
      }
    } // photon array loop

    mMatchInfoResidualsXTrackMIP[ch]->Fill(xTrk - xMip);
    mMatchInfoResidualsYTrackMIP[ch]->Fill(yTrk - yMip);
    if (matchHMP.getMipClusSize() < 20 && matchHMP.getMipClusSize() > 1) {
      mMatchInfoChargeClusterMIP[ch]->Fill(q);
    }
    mMatchInfoXMip[ch]->Fill(xMip);
    mMatchInfoYMip[ch]->Fill(yMip);
    mMatchInfoXTrack[ch]->Fill(xTrk);
    mMatchInfoYTrack[ch]->Fill(yTrk);
    mMatchInfoClusterMIPMap[ch]->Fill(xMip, yMip);
    mMatchInfoThetaCherenkovVsMom[ch]->Fill(TMath::Abs(matchHMP.getHmpMom()), matchHMP.getHMPsignal());
  } // match info loop
}

void HmpidTaskMatches::endOfCycle()
{
}

void HmpidTaskMatches::endOfActivity(const Activity& /*activity*/)
{
}

void HmpidTaskMatches::BookHistograms()
{
  for (int iCh = 0; iCh < 7; iCh++) {

    mMatchInfoResidualsXTrackMIP[iCh] = new TH1F(Form("XResCham%i", iCh), Form("X Residuals chamber %i; X residuals (cm); Entries/1 cm", iCh), 100, -50., 50.);
    mMatchInfoResidualsYTrackMIP[iCh] = new TH1F(Form("YRescham%i", iCh), Form("Y Residuals chamber %i; Y residuals (cm); Entries/1 cm", iCh), 100, -50., 50.);
    mMatchInfoChargeClusterMIP[iCh] = new TH1F(Form("MipClusQch%i", iCh), Form("MIP Cluster Charge chamber %i; ADC charge; Entries/1 ADC", iCh), 2000, 200., 2200.);
    mMatchInfoChargeClusterPhotons[iCh] = new TH1F(Form("PhelQch%i", iCh), Form("Photo-electron cluster charge chmaber %i; ADC charge; Counts/1 ADC", iCh), 180, 20., 200.);
    mMatchInfoClusterMIPMap[iCh] = new TH2F(Form("MipClusMap%i", iCh), Form("MIP Cluster map chamber %i; X (cm); Y (cm)", iCh), 133, 0, 133, 125, 0, 125);
    mMatchInfoClusterMIPMap[iCh]->SetStats(0);
    mMatchInfoXMip[iCh] = new TH1F(Form("MipX%i", iCh), Form("X MIP in HMPID Chamber %i;  X (cm); Entries/1 cm", iCh), 133, 0, 133);
    mMatchInfoYMip[iCh] = new TH1F(Form("MipY%i", iCh), Form("Y MIP in HMPID Chamber %i;  Y (cm); Entries/1 cm", iCh), 125, 0, 125);
    mMatchInfoXTrack[iCh] = new TH1F(Form("TrackX%i", iCh), Form("X Track in HMPID Chamber %i;  X (cm); Entries/1 cm", iCh), 133, 0, 133);
    mMatchInfoYTrack[iCh] = new TH1F(Form("TrackY%i", iCh), Form("Y Track in HMPID Chamber %i;  Y (cm); Entries/1 cm", iCh), 125, 0, 125);
    mMatchInfoThetaCherenkovVsMom[iCh] = new TH2F(Form("ThetavsMom%i", iCh), Form("ThetaCherenkov Vs Mom chamber %i; #it{p} (GeV/#it{c}); #theta_{Ckov} (rad);", iCh), 100, 0., 10., 1000, 0., 1.);
  }
  //
}

void HmpidTaskMatches::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;

  for (int iCh = 0; iCh < 7; iCh++) {
    mMatchInfoResidualsXTrackMIP[iCh]->Reset();
    mMatchInfoResidualsYTrackMIP[iCh]->Reset();
    mMatchInfoChargeClusterMIP[iCh]->Reset();
    mMatchInfoChargeClusterPhotons[iCh]->Reset();
    mMatchInfoClusterMIPMap[iCh]->Reset();
    mMatchInfoXMip[iCh]->Reset();
    mMatchInfoYMip[iCh]->Reset();
    mMatchInfoXTrack[iCh]->Reset();
    mMatchInfoYTrack[iCh]->Reset();
    mMatchInfoThetaCherenkovVsMom[iCh]->Reset();
  }
}
} // namespace o2::quality_control_modules::hmpid
