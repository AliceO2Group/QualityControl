// Link Giacomo
// RecoContainer: https://github.com/AliceO2Group/AliceO2/tree/dev/DataFormats/Detectors/GlobalTracking
// MatchInfoHMP: https://github.com/AliceO2Group/AliceO2/tree/dev/DataFormats/Reconstruction/include/ReconstructionDataFormats

// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   HmpidTaskMatchInfo.cxx
/// \author Nicola Nicassio, Giacomo Volpe
/// \brief  Task to produce HMPID matchingInfo QC plots
/// \since  20/10/2023
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "QualityControl/QcInfoLogger.h"
#include "HMPID/HmpidTaskMatchInfo.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/InputSpec.h>
#include "GlobalTrackingWorkflowHelpers/InputHelper.h"
#include "ReconstructionDataFormats/TrackTPCITS.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "DataFormatsGlobalTracking/RecoContainerCreateTracksVariadic.h"
#include "ReconstructionDataFormats/TrackParametrization.h"
#include "DetectorsBase/Propagator.h"

#include "HMPID/utils.h"
#include "DataFormatsHMP/Cluster.h"
#include "ReconstructionDataFormats/MatchInfoHMP.h"

using GTrackID = o2::dataformats::GlobalTrackID;

namespace o2::quality_control_modules::hmpid
{

HmpidTaskMatchInfo::HmpidTaskMatchInfo() : TaskInterface() {}

HmpidTaskMatchInfo::~HmpidTaskMatchInfo()
{
  for (int iCh = 0; iCh < 7; iCh++) {
    delete mMatchInfoResidualsXTrackMIP[iCh];
    delete mMatchInfoResidualsYTrackMIP[iCh];
    delete mMatchInfoChargeClusterMIP[iCh];
    delete mMatchInfoChargeClusterPhotons[iCh];
    delete mMatchInfoThetaCherenkovVsMom[iCh];
  }
}

void HmpidTaskMatchInfo::initialize(o2::framework::InitContext& /*ctx*/)
{
  Printf("**********************************in initialiaze**********************************************");

  ILOG(Debug, Devel) << "initialize HmpidTaskMatchInfo" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  mDataRequest = std::make_shared<o2::globaltracking::DataRequest>();
  //  mDataRequest->requestTracks(mSrc, mUseMC);

  // std::array<std::string, 1> title{"HMP"};
  for (int iCh = 0; iCh < 7; iCh++) {

    mMatchInfoResidualsXTrackMIP[iCh] = new TH1F(Form("mMatchInfoResidualsXTrackMIP_%i", iCh), Form("mMatchInfoResidualsXTrackMIP chamber %s;#it{x}_{trk} - it{x}_{MIP};Counts", iCh), 1000, -10.f, 10.f);
    mMatchInfoResidualsYTrackMIP[iCh] = new TH1F(Form("mMatchInfoResidualsYTrackMIP_%i", iCh), Form("mMatchInfoResidualsYTrackMIP chamber %i;#it{y}_{trk} - it{y}_{MIP};Counts", iCh), 1000, -10.f, 10.f);
    mMatchInfoChargeClusterMIP[iCh] = new TH1F(Form("mMatchInfoChargeClusterMIP_%i", iCh), Form("mMatchInfoChargeClusterMIP chmaber %i;ADC charge;Counts", iCh), 1000, 0.f, 2000.f);
    mMatchInfoChargeClusterPhotons[iCh] = new TH1F(Form("mMatchInfoChargeClusterPhotons_%s", iCh), Form("mMatchInfoChargeClusterPhotons chamber %i);ADC charge;Counts", iCh), 1000, 0.f, 2000.0f);
    mMatchInfoThetaCherenkovVsMom[iCh] = new TH2F(Form("mMatchInfoThetaCherenkovVsMom_%s", iCh), Form("mMatchInfoThetaCherenkovVsMom chmaber %i;#it{p} (GeV/#it{c});#theta_{c} (rad);Counts", iCh), 200, 0.f, 10.f, 1000, 0.f, 1.f);

    getObjectsManager()->startPublishing(mMatchInfoResidualsXTrackMIP[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoResidualsYTrackMIP[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoChargeClusterMIP[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoChargeClusterPhotons[iCh]);
    getObjectsManager()->startPublishing(mMatchInfoThetaCherenkovVsMom[iCh]);
  }
}

void HmpidTaskMatchInfo::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void HmpidTaskMatchInfo::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void HmpidTaskMatchInfo::monitorData(o2::framework::ProcessingContext& ctx)
{

  Printf("**********************************in monitor data**********************************************");

  /*++mTF;
  LOG(debug) << " ************************ ";
  LOG(debug) << " *** Processing TF " << mTF << " *** ";
  LOG(debug) << " ************************ ";
  */

  mRecoCont.collectData(ctx, *mDataRequest.get());

  // HMP
  // if (mRecoCont.isTrackSourceLoaded(GID::HMP)) {
  mHMPMatches = mRecoCont.getHMPMatches();
  //  LOG(debug) << "We found " << mHMPMatches.size() << " Tracks matched to HMPID";

  // loop over HMPID MatchInfo
  for (const auto& matchHMP : mHMPMatches) {

    int ch = matchHMP.getIdxHMPClus() / 1000000;

    const float* vPhotCharge = matchHMP.getPhotCharge();

    for (int i = 0; i < 10; i++) {
      if (vPhotCharge[i] > 0) {
        mMatchInfoChargeClusterPhotons[ch]->Fill(vPhotCharge[i]);
      }
      delete vPhotCharge;

      mMatchInfoResidualsXTrackMIP[ch]->Fill(matchHMP.getTrkX() - matchHMP.getMipX());
      mMatchInfoResidualsYTrackMIP[ch]->Fill(matchHMP.getTrkY() - matchHMP.getMipY());
      mMatchInfoChargeClusterMIP[ch]->Fill(matchHMP.getMipClusSize());
      mMatchInfoThetaCherenkovVsMom[ch]->Fill(TMath::Abs(matchHMP.getHmpMom()), matchHMP.getHMPsignal());
    }
  }
}

void HmpidTaskMatchInfo::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void HmpidTaskMatchInfo::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void HmpidTaskMatchInfo::reset()
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

} // namespace o2::quality_control_modules::hmpid
