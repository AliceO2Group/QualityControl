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

#ifdef WITH_OPENMP
#include <omp.h>
#endif

namespace o2::quality_control_modules::hmpid
{

HmpidTaskMatches::HmpidTaskMatches() : TaskInterface() {}

HmpidTaskMatches::~HmpidTaskMatches()
{
  ILOG(Info, Support) << "destructor called" << ENDM;
  for (int iCh = 0; iCh < 7; iCh++) {
    delete mMatchInfoResidualsXTrackMIP[iCh];
    delete mMatchInfoResidualsYTrackMIP[iCh];
    delete mMatchInfoChargeClusterMIP[iCh];
    delete mMatchInfoChargeClusterPhotons[iCh];
    delete mMatchInfoThetaCherenkovVsMom[iCh];
  }
}

void HmpidTaskMatches::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TaskMatches" << ENDM;

  mDataRequest = std::make_shared<o2::globaltracking::DataRequest>();
  mDataRequest->requestTracks(mSrc, false);

  // getJsonParameters();

  BookHistograms();

  // publish histograms (Q: why publishing in initalize?)
  for (int iCh = 0; iCh < 7; iCh++) {

    getObjectsManager()->startPublishing(mMatchInfoResidualsXTrackMIP[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoResidualsYTrackMIP[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoChargeClusterMIP[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoChargeClusterPhotons[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoThetaCherenkovVsMom[iCh]);
  }

  ILOG(Info, Support) << "START DOING QC HMPID Matches" << ENDM;

  // here do the QC
}

void HmpidTaskMatches::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  reset();
}

void HmpidTaskMatches::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void HmpidTaskMatches::monitorData(o2::framework::ProcessingContext& ctx)
{
  ILOG(Info, Support) << "monitorData" << ENDM;

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
    if (matchHMP.getMipClusSize() < 20 && matchHMP.getMipClusSize() > 1)
      mMatchInfoChargeClusterMIP[ch]->Fill(q);
    mMatchInfoThetaCherenkovVsMom[ch]->Fill(TMath::Abs(matchHMP.getHmpMom()), matchHMP.getHMPsignal());
  } // match info loop
}

void HmpidTaskMatches::endOfCycle()
{

  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void HmpidTaskMatches::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void HmpidTaskMatches::BookHistograms()
{
  for (int iCh = 0; iCh < 7; iCh++) {

    mMatchInfoResidualsXTrackMIP[iCh] = new TH1F(Form("X Residuals chamber %i", iCh), Form("X Residuals chamber %i; X residuals (cm); Counts/1 cm", iCh), 100, -50., 50.);
    mMatchInfoResidualsYTrackMIP[iCh] = new TH1F(Form("Y Residuals chamber %i", iCh), Form("Y Residuals chamber %i; Y residuals (cm); Counts/1 cm", iCh), 100, -50., 50.);
    mMatchInfoChargeClusterMIP[iCh] = new TH1F(Form("MIP Cluster Charge chamber %i", iCh), Form("MIP Cluster Charge chamber %i; ADC charge; Counts/1 ADC", iCh), 2000, 200., 2200.);
    mMatchInfoChargeClusterPhotons[iCh] = new TH1F(Form("Photo-electron Cluster Charge chamber %i", iCh), Form("Photo-electron cluster charge chmaber %i; ADC charge; Counts/1 ADC", iCh), 180, 20., 200.);
    mMatchInfoThetaCherenkovVsMom[iCh] = new TH2F(Form("ThetaCherenkov Vs Mom chamber %i", iCh), Form("ThetaCherenkov Vs Mom chamber %i; #it{p} (GeV/#it{c}); #theta_{Ckov} (rad);", iCh), 100, 0., 10., 1000, 0., 1.);
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
    mMatchInfoThetaCherenkovVsMom[iCh]->Reset();
  }
}

void HmpidTaskMatches::getJsonParameters()
{
  ILOG(Info, Support) << "GetJsonParams" << ENDM;
}

} // namespace o2::quality_control_modules::hmpid
