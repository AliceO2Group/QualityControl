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
  // K0s
  mMatchITSTPCQC.setDoK0QC(getFromConfig(mCustomParameters, "doK0QC", true));
  mMatchITSTPCQC.setMaxK0Eta(getFromConfig(mCustomParameters, "maxK0Eta", 0.8f));
  mMatchITSTPCQC.setRefitK0(getFromConfig(mCustomParameters, "refitK0", true));
  mMatchITSTPCQC.setCutK0Mass(getFromConfig(mCustomParameters, "cutK0Mass", 0.05f));
  if (auto param = mCustomParameters.find("trackSourcesK0"); param != mCustomParameters.end()) {
    mMatchITSTPCQC.setTrkSources(o2::dataformats::GlobalTrackID::getSourcesMask(param->second));
  }

  mMatchITSTPCQC.initDataRequest();
  mMatchITSTPCQC.init();
  mMatchITSTPCQC.publishHistograms(getObjectsManager());
}

void ITSTPCMatchingTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  mMatchITSTPCQC.reset();
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
    auto makeRatio = [](TEfficiency* eff) {
      std::string name = eff->GetName();
      name += "_Hist";
      auto ratio = std::make_unique<common::TH1FRatio>(name.c_str(), eff->GetTitle(),
                                                       eff->GetPassedHistogram()->GetXaxis()->GetNbins(),
                                                       eff->GetPassedHistogram()->GetXaxis()->GetXmin(),
                                                       eff->GetPassedHistogram()->GetXaxis()->GetXmax());
      ratio->SetBit(TH1::EStatusBits::kNoStats);
      if (!ratio->getNum()->Add(eff->GetPassedHistogram())) {
        ILOG(Error) << "Add operation for numerator histogram of " << name << " failed; efficiency will be skewed" << ENDM;
      }
      if (!ratio->getDen()->Add(eff->GetTotalHistogram())) {
        ILOG(Error) << "Add operation for denominator histogram of " << name << " failed; efficiency will be skewed" << ENDM;
      }
      ratio->GetXaxis()->SetTitle(eff->GetPassedHistogram()->GetXaxis()->GetTitle());
      ratio->GetYaxis()->SetTitle(eff->GetPassedHistogram()->GetYaxis()->GetTitle());
      ratio->Sumw2();
      ratio->setHasBinominalErrors();
      ratio->update();
      return ratio;
    };

    // Pt
    mEffPt = makeRatio(mMatchITSTPCQC.getFractionITSTPCmatch(gloqc::MatchITSTPCQC::ITS));
    getObjectsManager()->startPublishing(mEffPt.get(), PublicationPolicy::Once);
    getObjectsManager()->setDefaultDrawOptions(mEffPt->GetName(), "logx");

    // Eta
    mEffEta = makeRatio(mMatchITSTPCQC.getFractionITSTPCmatchEta(gloqc::MatchITSTPCQC::ITS));
    getObjectsManager()->startPublishing(mEffEta.get(), PublicationPolicy::Once);

    // Phi
    mEffPhi = makeRatio(mMatchITSTPCQC.getFractionITSTPCmatchPhi(gloqc::MatchITSTPCQC::ITS));
    getObjectsManager()->startPublishing(mEffPhi.get(), PublicationPolicy::Once);
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
