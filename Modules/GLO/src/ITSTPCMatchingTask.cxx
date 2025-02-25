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

#include <TCanvas.h>
#include <TH1.h>
#include <TH1D.h>
#include <TFitResult.h>
#include <TFitResultPtr.h>

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
  mMatchITSTPCQC.setEtaITSCut(getFromConfig(mCustomParameters, "minEtaITSCut", 1.4f));
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

    mFitBackground.mRejLeft = common::getFromConfig(mCustomParameters, "k0sBackgroundRejLeft", 0.48);
    mFitBackground.mRejRight = common::getFromConfig(mCustomParameters, "k0sBackgroundRejRight", 0.51);
    mBackgroundRangeLeft = common::getFromConfig(mCustomParameters, "k0sBackgroundRangeLeft", 0.45);
    mBackgroundRangeRight = common::getFromConfig(mCustomParameters, "k0sBackgroundRangeRight", 0.54);
    mBackground.reset(new TF1("gloFitK0sMassBackground", mFitBackground, mBackgroundRangeLeft, mBackgroundRangeRight, mFitBackground.mNPar));
    mSignalAndBackground.reset(new TF1("gloFitK0sMassSignal", "[0] + [1] * x + [2] * x * x + gaus(3)", mBackgroundRangeLeft, mBackgroundRangeRight));
    mK0sMassTrend.reset(new TGraphErrors);
    mK0sMassTrend->SetNameTitle("mK0MassTrend", "K0s Mass + Sigma;Cycle;K0s mass (GeV/c^{2})");
    getObjectsManager()->startPublishing(mK0sMassTrend.get(), PublicationPolicy::ThroughStop);
  }

  mMatchITSTPCQC.initDataRequest();
  mMatchITSTPCQC.init();
  mMatchITSTPCQC.publishHistograms(getObjectsManager());
}

void ITSTPCMatchingTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  mMatchITSTPCQC.reset();
  mIsPbPb = activity.mBeamType == "Pb-Pb";
  mNCycle = 0;
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
      mK0sCycle->Reset();
      mK0sCycle->Add(k0s, mK0sIntegral.get(), 1., -1.);
      mK0sIntegral->Reset();
      mK0sIntegral->Add(k0s);

      getObjectsManager()->startPublishing(mK0sCycle.get(), PublicationPolicy::Once);
      getObjectsManager()->startPublishing(mK0sIntegral.get(), PublicationPolicy::Once);

      TH1D* h{ nullptr };
      getObjectsManager()->startPublishing((h = mK0sCycle->ProjectionY("mK0sMassVsPtVsOcc_Cycle_pmass")), PublicationPolicy::Once);
      if (fitK0sMass(h)) {
        mK0sMassTrend->AddPoint(mNCycle, mSignalAndBackground->GetParameter(4));
        mK0sMassTrend->SetPointError(mK0sMassTrend->GetN() - 1, 0., mSignalAndBackground->GetParameter(5));
      }
      getObjectsManager()->startPublishing((h = mK0sIntegral->ProjectionY("mK0sMassVsPtVsOcc_Integral_pmass")), PublicationPolicy::Once);
      fitK0sMass(h);
    }
  }
  ++mNCycle;
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

bool ITSTPCMatchingTask::fitK0sMass(TH1* h)
{
  if (h->GetEntries() == 0) {
    ILOG(Warning, Devel) << "Cannot fit empty histogram: " << h->GetName() << ENDM;
    return false;
  }
  TFitResultPtr res = h->Fit(mBackground.get(), "SRNQ");
  if (res->Status() > 1) {
    ILOG(Warning, Devel) << "Failed k0s background fit for histogram: " << h->GetName() << ENDM;
    return false;
  }
  mSignalAndBackground->SetParameter(0, mBackground->GetParameter(0));                  // p0
  mSignalAndBackground->SetParameter(1, mBackground->GetParameter(1));                  // p1
  mSignalAndBackground->SetParameter(2, mBackground->GetParameter(2));                  // p2
  mSignalAndBackground->SetParameter(3, h->GetMaximum() - mBackground->Eval(mMassK0s)); // A (best guess)
  mSignalAndBackground->SetParameter(4, mMassK0s);                                      // mean
  mSignalAndBackground->SetParameter(5, 0.005);                                         // sigma
  h->Fit(mSignalAndBackground.get(), "RMQ");
  return true;
}

} // namespace o2::quality_control_modules::glo
