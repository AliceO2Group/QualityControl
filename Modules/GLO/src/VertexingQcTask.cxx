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
/// \file   VertexingQcTask.cxx
/// \author Chiara Zampolli
///

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TF1.h>
#include <TProfile.h>
#include <TEfficiency.h>

#include "ReconstructionDataFormats/PrimaryVertex.h"

#include "QualityControl/QcInfoLogger.h"
#include "GLO/VertexingQcTask.h"
#include <Framework/InputRecord.h>

namespace o2::quality_control_modules::glo
{

VertexingQcTask::~VertexingQcTask()
{
  delete mX;
  delete mY;
  delete mZ;
  delete mNContributors;
  delete mTimeUncVsNContrib;
  delete mBeamSpot;
  if (mUseMC) {
    delete mPurityVsMult;
    delete mNPrimaryMCEvWithVtx;
    delete mNPrimaryMCGen;
    delete mRatioNPrimaryMCEvWithVtxvsNPrimaryMCGen;
    delete mVtxEffVsMult;
    delete mCloneFactorVsMult;
    delete mVtxResXVsMult;
    delete mVtxResYVsMult;
    delete mVtxResZVsMult;
    delete mVtxPullsXVsMult;
    delete mVtxPullsYVsMult;
    delete mVtxPullsZVsMult;
  }
}

void VertexingQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize VertexingQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("verbose"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - verbose (= verbose printouts): " << param->second << ENDM;
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mVerbose = true;
    }
  }

  if (auto param = mCustomParameters.find("isMC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - isMC: " << param->second << ENDM;
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mUseMC = true;
      mMCReader.initFromDigitContext("collisioncontext.root");
      mPurityVsMult = new TProfile("purityVsMult", "purityVsMult; MC primary mult; vtx purity", 10000, -0.5, 9999.5, 0.f, 1.f);
      mNPrimaryMCEvWithVtx = new TH1F("NPrimaryMCEvWithVtx", "NPrimaryMCEvWithVtx; MC primary mult; n. events", 10000, -0.5, 9999.5);
      mNPrimaryMCEvWithVtx->Sumw2();
      mNPrimaryMCGen = new TH1F("NPrimaryMCGen", "NPrimaryMCGen; MC primary mult; n. events with vtx", 10000, -0.5, 9999.5);
      mNPrimaryMCGen->Sumw2();
      mRatioNPrimaryMCEvWithVtxvsNPrimaryMCGen = new TH1F("RatioNPrimaryMCEvWithVtxvsNPrimaryMCGen", "Ratio NPrimaryMCEvWithVtx vs. NPrimaryMCGen", 10000, -0.5, 9999.5);
      mRatioNPrimaryMCEvWithVtxvsNPrimaryMCGen->Sumw2();
      mVtxEffVsMult = new TEfficiency("vtxEffVsMult", "vtxEffVsMult; MC primary mult; vtx reco efficiency", 10000, -0.5, 9999.5);
      mCloneFactorVsMult = new TProfile("cloneFactorVsMult", "cloneFactorVsMult; MC primary mult; n. cloned vertices", 100, -0.5, 9999.5, 0.f, 1.f);
      mVtxResXVsMult = new TProfile("vtxResXVsMult", "vtxRes (X) vs mult; n. contributors; res on X (cm)", 100, -0.5, 9999.5, 0.f, 100.f);
      mVtxResYVsMult = new TProfile("vtxResYVsMult", "vtxRes (Y) vs mult; n. conrtibutors; res on Y (cm)", 100, -0.5, 9999.5, 0.f, 100.f);
      mVtxResZVsMult = new TProfile("vtxResZVsMult", "vtxRes (Z) vs mult; n. contriobutors; res on Z (cm)", 100, -0.5, 9999.5, 0.f, 100.f);
      mVtxPullsXVsMult = new TProfile("vtxPullsXVsMult", "vtxPulls (X) vs mult; MC primary mult; pulls for X", 100, -0.5, 9999.5, 0.f, 100.f);
      mVtxPullsYVsMult = new TProfile("vtxPullsYVsMult", "vtxPulls (Y) vs mult; MC primary mult; pulls for Y", 100, -0.5, 9999.5, 0.f, 100.f);
      mVtxPullsZVsMult = new TProfile("vtxPullsZVsMult", "vtxPulls (Z) vs mult; MC primary mult; pulls for Z", 100, -0.5, 9999.5, 0.f, 100.f);
      getObjectsManager()->startPublishing(mPurityVsMult);
      mNPrimaryMCEvWithVtx->SetOption("logy");
      getObjectsManager()->startPublishing(mNPrimaryMCEvWithVtx);
      mNPrimaryMCGen->SetOption("logy");
      getObjectsManager()->startPublishing(mNPrimaryMCGen);
      getObjectsManager()->startPublishing(mRatioNPrimaryMCEvWithVtxvsNPrimaryMCGen);
      getObjectsManager()->startPublishing(mVtxEffVsMult);
      getObjectsManager()->startPublishing(mCloneFactorVsMult);
      getObjectsManager()->startPublishing(mVtxResXVsMult);
      getObjectsManager()->startPublishing(mVtxResYVsMult);
      getObjectsManager()->startPublishing(mVtxResZVsMult);
      getObjectsManager()->startPublishing(mVtxPullsXVsMult);
      getObjectsManager()->startPublishing(mVtxPullsYVsMult);
      getObjectsManager()->startPublishing(mVtxPullsZVsMult);

      mPurityVsMult->GetYaxis()->SetTitleOffset(1.4);
      mNPrimaryMCEvWithVtx->GetYaxis()->SetTitleOffset(1.4);
      mNPrimaryMCGen->GetYaxis()->SetTitleOffset(1.4);
      mRatioNPrimaryMCEvWithVtxvsNPrimaryMCGen->GetYaxis()->SetTitleOffset(1.4);
      mCloneFactorVsMult->GetYaxis()->SetTitleOffset(1.4);
      mVtxResXVsMult->GetYaxis()->SetTitleOffset(1.4);
      mVtxResYVsMult->GetYaxis()->SetTitleOffset(1.4);
      mVtxResZVsMult->GetYaxis()->SetTitleOffset(1.4);
      mVtxPullsXVsMult->GetYaxis()->SetTitleOffset(1.4);
      mVtxPullsYVsMult->GetYaxis()->SetTitleOffset(1.4);
      mVtxPullsZVsMult->GetYaxis()->SetTitleOffset(1.4);
    }
  }

  mX = new TH1F("vertex_X", "vertex_X; vtx_X (cm); entries", 300, -0.3, 0.3);
  fX = new TF1("fX", "gaus");
  mY = new TH1F("vertex_Y", "vertex_Y; vtx_Y (cm); entries", 300, -0.3, 0.3);
  fY = new TF1("fY", "gaus");
  mZ = new TH1F("vertex_Z", "vertex_Z; vtx_Z (cm);entries", 1000, -20, 20);
  mNContributors = new TH1F("vertex_NContributors", "vertex_NContributors; n. contributors; entries", 1000, -0.5, 999.5);
  mTimeUncVsNContrib = new TProfile("timeUncVsNContrib", "timeUncVsNContrib; n. contributors; time uncertainty (us)", 100, -0.5, 999.5, 0.f, 10.f);
  mBeamSpot = new TH2F("beamSpot", "beam spot; vtx_X (cm); vtx_Y (cm)", 300, -0.3, 0.3, 300, -0.3, 0.3);

  mX->SetOption("logy");
  getObjectsManager()->startPublishing(mX);
  mY->SetOption("logy");
  getObjectsManager()->startPublishing(mY);
  mZ->SetOption("logy");
  getObjectsManager()->startPublishing(mZ);
  mNContributors->SetOption("logy");
  getObjectsManager()->startPublishing(mNContributors);
  mTimeUncVsNContrib->SetOption("logy");
  getObjectsManager()->startPublishing(mTimeUncVsNContrib);
  mBeamSpot->SetOption("colz");
  getObjectsManager()->startPublishing(mBeamSpot);

  mX->GetYaxis()->SetTitleOffset(1.4);
  mY->GetYaxis()->SetTitleOffset(1.4);
  mZ->GetYaxis()->SetTitleOffset(1.4);
  mNContributors->GetYaxis()->SetTitleOffset(1.4);
  mTimeUncVsNContrib->GetYaxis()->SetTitleOffset(1.4);
  mBeamSpot->GetYaxis()->SetTitleOffset(1.4);
}

void VertexingQcTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void VertexingQcTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void VertexingQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  // get the payload of a specific input, which is a char array. "random" is the binding specified in the config file.
  const auto pvertices = ctx.inputs().get<gsl::span<o2::dataformats::PrimaryVertex>>("pvtx");
  gsl::span<const o2::MCEventLabel> mcLbl;
  if (mUseMC) {
    mcLbl = ctx.inputs().get<gsl::span<o2::MCEventLabel>>("pvtxLbl");
    for (const auto& lbl : mcLbl) {
      if (lbl.getSourceID() != 0) { // using only underlying event,  which is source 0
        continue;
      }
      ILOG(Debug, Support) << "From source " << lbl.getSourceID() << ", event " << lbl.getEventID() << " has a vertex" << ENDM;
      mMapEvIDSourceID[{ lbl.getEventID(), lbl.getSourceID() }]++;
      if (mMapEvIDSourceID[{ lbl.getEventID(), lbl.getSourceID() }] == 1) { // filling numerator for efficiency
        auto header = mMCReader.getMCEventHeader(lbl.getSourceID(), lbl.getEventID());
        auto mult = header.GetNPrim();
        ILOG(Debug, Support) << "Found vertex for event with mult = " << mult << ENDM;
        mNPrimaryMCEvWithVtx->Fill(mult);
        // mRatioNPrimaryMCEvWithVtxvsNPrimaryMCGen = (TH1F *)mNPrimaryMCEvWithVtx->Clone(); //did not work
        mRatioNPrimaryMCEvWithVtxvsNPrimaryMCGen->Fill(mult);
      }
    }
    for (const auto& lbl : mcLbl) {
      if (lbl.getSourceID() != 0) { // using only underlying event,  which is source 0
        continue;
      }
      auto header = mMCReader.getMCEventHeader(lbl.getSourceID(), lbl.getEventID());
      auto mult = header.GetNPrim();
      auto nVertices = mMapEvIDSourceID[{ lbl.getEventID(), lbl.getSourceID() }];
      if (nVertices == 1) {
        ILOG(Debug, Support) << "Found " << nVertices << " vertex for event with mult = " << mult << ENDM;
      } else {
        ILOG(Debug, Support) << "Found " << nVertices << " vertices for event with mult = " << mult << ENDM;
      }
      mCloneFactorVsMult->Fill(mult, nVertices);
    }

    for (size_t i = 0; i < mMCReader.getNEvents(0); ++i) { // we use the underlying event, which is source 0
      auto header = mMCReader.getMCEventHeader(0, i);      // we use the underlying event, which is source 0
      auto mult = header.GetNPrim();
      ILOG(Debug, Support) << "Found Gen event with mult = " << mult << ENDM;
      mNPrimaryMCGen->Fill(mult);
    }
    mRatioNPrimaryMCEvWithVtxvsNPrimaryMCGen->Divide(mNPrimaryMCGen);
  }

  for (uint64_t i = 0; i < pvertices.size(); ++i) {
    auto x = pvertices[i].getX();
    auto y = pvertices[i].getY();
    auto z = pvertices[i].getZ();
    auto nContr = pvertices[i].getNContributors();
    auto timeUnc = pvertices[i].getTimeStamp().getTimeStampError();
    ILOG(Debug, Support) << "x = " << x << ", y = " << y << ", z = " << z << ", nContributors = " << nContr << ", timeUnc = " << timeUnc << ENDM;
    mX->Fill(x);
    mX->Fit("fX", "Q", "", mX->GetMean() - mX->GetRMS(), mX->GetMean() + mX->GetRMS());
    mY->Fill(y);
    mY->Fit("fY", "Q", "", mY->GetMean() - mY->GetRMS(), mY->GetMean() + mY->GetRMS());
    mZ->Fill(z);
    mNContributors->Fill(nContr);
    mTimeUncVsNContrib->Fill(nContr, timeUnc);
    mBeamSpot->Fill(x, y);

    if (mUseMC && mcLbl[i].isSet()) { // make sure the label was set
      auto header = mMCReader.getMCEventHeader(mcLbl[i].getSourceID(), mcLbl[i].getEventID());
      auto purity = mcLbl[i].getCorrWeight();
      auto mult = header.GetNPrim();
      ILOG(Debug, Support) << "purity = " << purity << ", mult = " << mult << ENDM;
      mPurityVsMult->Fill(mult, purity);
      TVector3 vtMC;
      header.GetVertex(vtMC);
      mVtxResXVsMult->Fill(mult, vtMC[0] - pvertices[i].getX());
      mVtxResYVsMult->Fill(mult, vtMC[1] - pvertices[i].getY());
      mVtxResZVsMult->Fill(mult, vtMC[2] - pvertices[i].getZ());
      mVtxPullsXVsMult->Fill(mult, (vtMC[0] - pvertices[i].getX()) / std::sqrt(pvertices[i].getSigmaX2()));
      mVtxPullsYVsMult->Fill(mult, (vtMC[1] - pvertices[i].getY()) / std::sqrt(pvertices[i].getSigmaY2()));
      mVtxPullsZVsMult->Fill(mult, (vtMC[2] - pvertices[i].getZ()) / std::sqrt(pvertices[i].getSigmaZ2()));
    }
  }

  // 3. Access CCDB. If it is enough to retrieve it once, do it in initialize().
  // Remember to delete the object when the pointer goes out of scope or it is no longer needed.
  //   TObject* condition = TaskInterface::retrieveCondition("QcTask/example"); // put a valid condition path here
  //   if (condition) {
  //     LOG(info) << "Retrieved " << condition->ClassName();
  //     delete condition;
  //   }
}

void VertexingQcTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;

  if (mUseMC) {

    if (!mVtxEffVsMult->SetTotalHistogram(*mNPrimaryMCGen, "f") ||
        !mVtxEffVsMult->SetPassedHistogram(*mNPrimaryMCEvWithVtx, "")) {
      ILOG(Fatal, Support) << "Something went wrong in defining the efficiency histograms!!";
    } else {
      if (mVerbose) {
        for (int ibin = 0; ibin < mNPrimaryMCEvWithVtx->GetNbinsX(); ibin++) {
          if (mNPrimaryMCEvWithVtx->GetBinContent(ibin + 1) != 0 && mNPrimaryMCGen->GetBinContent(ibin + 1) != 0) {
            ILOG(Info, Support) << "ibin = " << ibin + 1 << ", mNPrimaryMCEvWithVtx->GetBinContent(ibin + 1) = " << mNPrimaryMCEvWithVtx->GetBinContent(ibin + 1) << ", mNPrimaryMCGen->GetBinContent(ibin + 1) = " << mNPrimaryMCGen->GetBinContent(ibin + 1) << ", efficiency = " << mVtxEffVsMult->GetEfficiency(ibin + 1) << ENDM;
            ILOG(Info, Support) << "ibin = " << ibin + 1 << ", mNPrimaryMCEvWithVtx->GetBinError(ibin + 1) = " << mNPrimaryMCEvWithVtx->GetBinError(ibin + 1) << ", mNPrimaryMCGen->GetBinError(ibin + 1) = " << mNPrimaryMCGen->GetBinError(ibin + 1) << ", efficiency error low = " << mVtxEffVsMult->GetEfficiencyErrorLow(ibin + 1) << ", efficiency error up = " << mVtxEffVsMult->GetEfficiencyErrorUp(ibin + 1) << ENDM;
          }
        }
        ILOG(Info, Support) << "mNPrimaryMCEvWithVtx entries = " << mNPrimaryMCEvWithVtx->GetEntries() << ", mNPrimaryMCGen entries = " << mNPrimaryMCGen->GetEntries() << ENDM;
      }
    }
  }
}

void VertexingQcTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void VertexingQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mX->Reset();
  mY->Reset();
  mZ->Reset();
  mNContributors->Reset();
  mBeamSpot->Reset();
  if (mUseMC) {
    mPurityVsMult->Reset();
    mNPrimaryMCEvWithVtx->Reset();
    mNPrimaryMCGen->Reset();
    mRatioNPrimaryMCEvWithVtxvsNPrimaryMCGen->Reset();
    mCloneFactorVsMult->Reset();
    mVtxResXVsMult->Reset();
    mVtxResYVsMult->Reset();
    mVtxResZVsMult->Reset();
    mVtxPullsXVsMult->Reset();
    mVtxPullsYVsMult->Reset();
    mVtxPullsZVsMult->Reset();
  }
}

} // namespace o2::quality_control_modules::glo
