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
/// \file   ITSTPCMatchingTask.cxx
/// \author Chiara Zampolli
///

#include <TH1D.h>
#include <TProfile.h>

#include "QualityControl/QcInfoLogger.h"
#include "GLO/ITSTPCMatchingTask.h"
#include "Common/Utils.h"

#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>

using matchType = o2::gloqc::MatchITSTPCQC::matchType;
using namespace o2::quality_control_modules::common;

namespace o2::quality_control_modules::glo
{

void ITSTPCMatchingTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize ITSTPCMatchingTask" << ENDM;

  // MC
  mMatchITSTPCQC.setUseMC(getFromConfig(mCustomParameters, "isMC", false));
  mMatchITSTPCQC.setUseTrkPID(getFromConfig(mCustomParameters, "useTrkPID", false));
  // ITS track
  mMatchITSTPCQC.setMinPtITSCut(getFromConfig(mCustomParameters, "minPtITSCut", 0.1f));
  mMatchITSTPCQC.setEtaITSCut(getFromConfig(mCustomParameters, "etaITSCut", 1.4f));
  mMatchITSTPCQC.setMinNClustersITS(getFromConfig(mCustomParameters, "minNITSClustersCut", 0));
  mMatchITSTPCQC.setMaxChi2PerClusterITS(getFromConfig(mCustomParameters, "maxChi2PerClusterITS", 1e10f));
  // TPC track
  mMatchITSTPCQC.setMinPtTPCCut(getFromConfig(mCustomParameters, "minPtTPCCut", 0.1f));
  mMatchITSTPCQC.setEtaTPCCut(getFromConfig(mCustomParameters, "etaTPCCut", 1.4f));
  mMatchITSTPCQC.setMinNTPCClustersCut(getFromConfig(mCustomParameters, "minNTPCClustersCut", 60));
  mMatchITSTPCQC.setMinDCAtoBeamPipeDistanceCut(getFromConfig(mCustomParameters, "minDCACut", 100.f));
  mMatchITSTPCQC.setMinDCAtoBeamPipeYCut(getFromConfig(mCustomParameters, "minDCACutY", 10.f));
  // ITS-TPC kinematics
  mMatchITSTPCQC.setPtCut(getFromConfig(mCustomParameters, "minPtCut", 0.1f));
  mMatchITSTPCQC.setMaxPtCut(getFromConfig(mCustomParameters, "maxPtCut", 20.f));
  mMatchITSTPCQC.setEtaCut(getFromConfig(mCustomParameters, "etaCut", 1.4f));
  // Sync
  mIsSync = common::getFromConfig(mCustomParameters, "isSync", false);
  // MTC ratios
  mDoMTCRatios = common::getFromConfig(mCustomParameters, "doMTCRatios", false);
  // K0s
  mMatchITSTPCQC.setDoK0QC((mDoK0s = getFromConfig(mCustomParameters, "doK0QC", false)));
  if (mIsSync && mDoK0s) {
    mMatchITSTPCQC.setMaxK0Eta(getFromConfig(mCustomParameters, "maxK0Eta", 0.8f));
    mMatchITSTPCQC.setRefitK0(getFromConfig(mCustomParameters, "refitK0", true));
    mMatchITSTPCQC.setCutK0Mass(getFromConfig(mCustomParameters, "cutK0Mass", 0.05f));
    if (auto param = mCustomParameters.find("trackSourcesK0"); param != mCustomParameters.end()) {
      mMatchITSTPCQC.setTrkSources(o2::dataformats::GlobalTrackID::getSourcesMask(param->second));
    }

    mPublishK0s3D = getFromConfig(mCustomParameters, "publishK0s3D", false);
    mSplitTPCOccupancy = getFromConfig(mCustomParameters, "splitK0sMassOccupancy", mSplitTPCOccupancy);
    mSplitPt = getFromConfig(mCustomParameters, "splitK0sMassPt", mSplitPt);
    mK0sFitter.init(mCustomParameters);
  }
  // PV
  mDoPVITS = common::getFromConfig(mCustomParameters, "doPVITSQC", false);

  mMatchITSTPCQC.initDataRequest();
  mMatchITSTPCQC.init();
  mMatchITSTPCQC.publishHistograms(getObjectsManager());
}

void ITSTPCMatchingTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  mMatchITSTPCQC.reset();
  mIsPbPb = activity.mBeamType == "Pb-Pb";
}

void ITSTPCMatchingTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void ITSTPCMatchingTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  ILOG(Debug) << "********** Starting monitoring" << ENDM;
  mMatchITSTPCQC.run(ctx);
}

void ITSTPCMatchingTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  mMatchITSTPCQC.finalize();

  // Sync Mode
  if (mIsSync) {
    if (mDoMTCRatios) {
      auto makeRatio = [](std::unique_ptr<common::TH1FRatio>& ratio, TEfficiency* eff) {
        if (ratio) {
          ratio->Reset();
        } else {
          std::string name = eff->GetName();
          name += "_Hist";
          ratio = std::make_unique<common::TH1FRatio>(name.c_str(), eff->GetTitle(),
                                                      eff->GetPassedHistogram()->GetXaxis()->GetNbins(),
                                                      eff->GetPassedHistogram()->GetXaxis()->GetXmin(),
                                                      eff->GetPassedHistogram()->GetXaxis()->GetXmax());
        }
        ratio->SetBit(TH1::EStatusBits::kNoStats);
        if (!ratio->getNum()->Add(eff->GetPassedHistogram())) {
          ILOG(Error) << "Add operation for numerator histogram of " << ratio->GetName() << " failed; efficiency will be skewed" << ENDM;
        }
        if (!ratio->getDen()->Add(eff->GetTotalHistogram())) {
          ILOG(Error) << "Add operation for denominator histogram of " << ratio->GetName() << " failed; efficiency will be skewed" << ENDM;
        }
        ratio->GetXaxis()->SetTitle(eff->GetPassedHistogram()->GetXaxis()->GetTitle());
        ratio->GetYaxis()->SetTitle(eff->GetPassedHistogram()->GetYaxis()->GetTitle());
        ratio->Sumw2();
        ratio->setHasBinominalErrors();
        ratio->update();
      };

      // Pt
      makeRatio(mEffPt, mMatchITSTPCQC.getFractionITSTPCmatch(gloqc::MatchITSTPCQC::ITS));
      getObjectsManager()->startPublishing(mEffPt.get(), PublicationPolicy::Once);
      getObjectsManager()->setDefaultDrawOptions(mEffPt->GetName(), "logx");

      // Eta
      makeRatio(mEffEta, mMatchITSTPCQC.getFractionITSTPCmatchEta(gloqc::MatchITSTPCQC::ITS));
      getObjectsManager()->startPublishing(mEffEta.get(), PublicationPolicy::Once);

      // Phi
      makeRatio(mEffPhi, mMatchITSTPCQC.getFractionITSTPCmatchPhi(gloqc::MatchITSTPCQC::ITS));
      getObjectsManager()->startPublishing(mEffPhi.get(), PublicationPolicy::Once);
    }

    if (mDoPVITS) {
      // const auto* h = (mIsPbPb) ? mMatchITSTPCQC.getHistoPVNContVsITSTracksPbPb() : mMatchITSTPCQC.getHistoPVNContVsITSTracks();
      // if (!h) {
      //   ILOG(Fatal) << "Could not retrieve pv ITS histogram!" << ENDM;
      // }

      // mPVITSCycle.reset();
      // mPVITSCycle.reset(dynamic_cast<TH2F*>(h->Clone("mPVNContVsITSTracks_Cycle")));
      // if (!mPVITSCycle) {
      //   ILOG(Fatal) << "Could not retrieve pv ITS histogram for current cycle!" << ENDM;
      // }
      // if (!mPVITSIntegral) {
      //   mPVITSIntegral.reset(dynamic_cast<TH2F*>(h->Clone("mPVNContVsITSTracks_Integral")));
      //   if (!mPVITSIntegral) {
      //     ILOG(Fatal) << "Could not retrieve pv ITS histogram for integral!" << ENDM;
      //   }
      // }
      // if (mPVITSCycle->GetEntries() != h->GetEntries()) {
      //   mPVITSCycle->Reset();
      //   mPVITSCycle->Add(h, mPVITSCycle.get(), 1., -1.);
      //   mPVITSIntegral->Reset();
      //   mPVITSIntegral->Add(h);

      //   getObjectsManager()->startPublishing(mPVITSCycle.get(), PublicationPolicy::Once);
      //   getObjectsManager()->startPublishing(mPVITSIntegral.get(), PublicationPolicy::Once);
      //   getObjectsManager()->startPublishing(mPVITSCycle->ProfileX(), PublicationPolicy::Once);
      //   getObjectsManager()->startPublishing(mPVITSIntegral->ProfileX(), PublicationPolicy::Once);
      // }
    }

    if (mDoK0s) {
      const auto* k0s = (mIsPbPb) ? mMatchITSTPCQC.getHistoK0MassVsPtVsOccPbPb() : mMatchITSTPCQC.getHistoK0MassVsPtVsOccpp();
      if (!k0s) {
        ILOG(Fatal) << "Could not retrieve k0s histogram for beam type: " << mIsPbPb << ENDM;
      }

      mK0sCycle.reset();
      mK0sCycle.reset(dynamic_cast<TH3F*>(k0s->Clone("mK0sMassVsPtVsOcc_Cycle")));
      if (!mK0sCycle) {
        ILOG(Fatal) << "Could not retrieve k0s histogram for current cycle" << ENDM;
      }
      if (!mK0sIntegral) {
        mK0sIntegral.reset(dynamic_cast<TH3F*>(k0s->Clone("mK0sMassVsPtVsOcc_Integral")));
        if (!mK0sIntegral) {
          ILOG(Fatal) << "Could not retrieve k0s histogram integral" << ENDM;
        }
      }
      if (k0s->GetEntries() != mK0sIntegral->GetEntries()) {
        mK0sCycle->Reset();
        mK0sCycle->Add(k0s, mK0sIntegral.get(), 1., -1.);
        mK0sIntegral->Reset();
        mK0sIntegral->Add(k0s);

        if (mPublishK0s3D) {
          getObjectsManager()->startPublishing(mK0sCycle.get(), PublicationPolicy::Once);
          getObjectsManager()->startPublishing(mK0sIntegral.get(), PublicationPolicy::Once);
        }

        TH1D* h{ nullptr };
        getObjectsManager()->startPublishing((h = mK0sCycle->ProjectionY("mK0sMassVsPtVsOcc_Cycle_pmass")), PublicationPolicy::Once);

        if (mSplitTPCOccupancy != OptValue<float>) {
          auto splitOccBin = mK0sCycle->GetZaxis()->FindBin(mSplitTPCOccupancy);
          getObjectsManager()->startPublishing(mK0sCycle->ProjectionY("mK0sMassVsPtVsOcc_Cycle_pmass_lowOcc", 0, -1, 0, splitOccBin - 1), PublicationPolicy::Once);
          getObjectsManager()->startPublishing(mK0sCycle->ProjectionY("mK0sMassVsPtVsOcc_Cycle_pmass_highOcc", 0, -1, splitOccBin), PublicationPolicy::Once);
        }

        if (mSplitPt != OptValue<float>) {
          auto splitPtBin = mK0sCycle->GetXaxis()->FindBin(mSplitPt);
          getObjectsManager()->startPublishing(mK0sCycle->ProjectionY("mK0sMassVsPtVsOcc_Cycle_pmass_lowPt", 0, splitPtBin - 1), PublicationPolicy::Once);
          getObjectsManager()->startPublishing(mK0sCycle->ProjectionY("mK0sMassVsPtVsOcc_Cycle_pmass_highPt", splitPtBin), PublicationPolicy::Once);
        }

        if (mK0sFitter.fit(h)) {
          if (mDoPVITS && mPVITSCycle->GetEntries() != 0) {
            mK0sFitter.mSignalAndBackground->SetParameter(helpers::K0sFitter::Parameters::Pol0, mPVITSCycle->GetEntries());
          }
          getObjectsManager()->startPublishing<true>(mK0sFitter.mSignalAndBackground.get(), PublicationPolicy::Once);
        }

        getObjectsManager()->startPublishing((h = mK0sIntegral->ProjectionY("mK0sMassVsPtVsOcc_Integral_pmass")), PublicationPolicy::Once);
      }
    }
  }
}

void ITSTPCMatchingTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ITSTPCMatchingTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mMatchITSTPCQC.reset();
  mEffPt.reset();
  mEffPhi.reset();
  mEffEta.reset();
}

} // namespace o2::quality_control_modules::glo
