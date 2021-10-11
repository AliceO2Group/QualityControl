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
#include <TH1.h>
#include <TProfile.h>

#include "ReconstructionDataFormats/PrimaryVertex.h"

#include "QualityControl/QcInfoLogger.h"
#include "REC/VertexingQcTask.h"
#include <Framework/InputRecord.h>

namespace o2::quality_control_modules::rec
{

VertexingQcTask::~VertexingQcTask()
{
  delete mX;
  delete mY;
  delete mZ;
  delete mNContributors;
  delete mTimeUncVsNContrib;
  if (mUseMC) {
    delete mPurityVsMult;
    delete mNPrimaryMCEvWithVtx;
    delete mNPrimaryMCGen;
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
  ILOG(Info, Support) << "initialize VertexingQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("isMC"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - isMC: " << param->second << ENDM;
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mUseMC = true;
      mMCReader.initFromDigitContext("collisioncontext.root");
      mPurityVsMult = new TProfile("purityVsMult", "purityVsMult", 100, -0.5, 9999.5, 0.f, 1.f);
      mNPrimaryMCEvWithVtx = new TH1F("NPrimaryMCEvWithVtx", "NPrimaryMCEvWithVtx", 100, -0.5, 9999.5);
      mNPrimaryMCEvWithVtx->Sumw2();
      mNPrimaryMCGen = new TH1F("NPrimaryMCGen", "NPrimaryMCGen", 100, -0.5, 9999.5);
      mNPrimaryMCGen->Sumw2();
      mVtxEffVsMult = new TH1F("vtxEffVsMult", "vtxEffVsMult", 100, -0.5, 9999.5);
      mCloneFactorVsMult = new TProfile("cloneFactorVsMult", "cloneFactorVsMult", 100, -0.5, 9999.5, 0.f, 1.f);
      mVtxResXVsMult = new TProfile("vtxResXVsMult", "vtxRes (X) vs mult", 100, -0.5, 9999.5, 0.f, 100.f);
      mVtxResYVsMult = new TProfile("vtxResYVsMult", "vtxRes (Y) vs mult", 100, -0.5, 9999.5, 0.f, 100.f);
      mVtxResZVsMult = new TProfile("vtxResZVsMult", "vtxRes (Z) vs mult", 100, -0.5, 9999.5, 0.f, 100.f);
      mVtxPullsXVsMult = new TProfile("vtxPullsXVsMult", "vtxPulls (X) vs mult", 100, -0.5, 9999.5, 0.f, 100.f);
      mVtxPullsYVsMult = new TProfile("vtxPullsYVsMult", "vtxPulls (Y) vs mult", 100, -0.5, 9999.5, 0.f, 100.f);
      mVtxPullsZVsMult = new TProfile("vtxPullsZVsMult", "vtxPulls (Z) vs mult", 100, -0.5, 9999.5, 0.f, 100.f);
      getObjectsManager()->startPublishing(mPurityVsMult);
      getObjectsManager()->startPublishing(mNPrimaryMCEvWithVtx);
      getObjectsManager()->startPublishing(mNPrimaryMCGen);
      getObjectsManager()->startPublishing(mVtxEffVsMult);
      getObjectsManager()->startPublishing(mCloneFactorVsMult);
      getObjectsManager()->startPublishing(mVtxResXVsMult);
      getObjectsManager()->startPublishing(mVtxResYVsMult);
      getObjectsManager()->startPublishing(mVtxResZVsMult);
      getObjectsManager()->startPublishing(mVtxPullsXVsMult);
      getObjectsManager()->startPublishing(mVtxPullsYVsMult);
      getObjectsManager()->startPublishing(mVtxPullsZVsMult);
    }
  }

  mX = new TH1F("vertex_X", "vertex_X", 1000, -1, 1);
  mY = new TH1F("vertex_Y", "vertex_Y", 1000, -1, 1);
  mZ = new TH1F("vertex_Z", "vertex_Z", 1000, -20, 20);
  mNContributors = new TH1F("vertex_NContributors", "vertex_NContributors", 1000, -0.5, 999.5);
  mTimeUncVsNContrib = new TProfile("timeUncVsNContrib", "timeUncVsNContrib", 100, -0.5, 999.5, 0.f, 10.f);

  getObjectsManager()->startPublishing(mX);
  getObjectsManager()->startPublishing(mY);
  getObjectsManager()->startPublishing(mZ);
  getObjectsManager()->startPublishing(mNContributors);
  getObjectsManager()->startPublishing(mTimeUncVsNContrib);
}

void VertexingQcTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void VertexingQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
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
      }
    }
    for (const auto& lbl : mcLbl) {
      auto header = mMCReader.getMCEventHeader(lbl.getSourceID(), lbl.getEventID());
      if (lbl.getSourceID() != 0) { // using only underlying event,  which is source 0
        continue;
      }
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
  }

  for (uint64_t i = 0; i < pvertices.size(); ++i) {
    auto x = pvertices[i].getX();
    auto y = pvertices[i].getY();
    auto z = pvertices[i].getZ();
    auto nContr = pvertices[i].getNContributors();
    auto timeUnc = pvertices[i].getTimeStamp().getTimeStampError();
    ILOG(Debug, Support) << "x = " << x << ", y = " << y << ", z = " << z << ", nContributors = " << nContr << ", timeUnc = " << timeUnc << ENDM;
    mX->Fill(x);
    mY->Fill(y);
    mZ->Fill(z);
    mNContributors->Fill(nContr);
    mTimeUncVsNContrib->Fill(nContr, timeUnc);

    if (mUseMC && mcLbl[i].isSet()) { // make sure the label was set
      auto header = mMCReader.getMCEventHeader(mcLbl[i].getSourceID(), mcLbl[i].getEventID());
      auto purity = mcLbl[i].getCorrWeight();
      auto mult = header.GetNPrim();
      ILOG(Info, Support) << "purity = " << purity << ", mult = " << mult << ENDM;
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
  ILOG(Info, Support) << "endOfCycle" << ENDM;

  if (mUseMC) {
    mVtxEffVsMult->Divide(mNPrimaryMCEvWithVtx, mNPrimaryMCGen, 1, 1, "B");
  }
}

void VertexingQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void VertexingQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mX->Reset();
  mY->Reset();
  mZ->Reset();
  mNContributors->Reset();
  if (mUseMC) {
    mPurityVsMult->Reset();
    mNPrimaryMCEvWithVtx->Reset();
    mNPrimaryMCGen->Reset();
    mVtxEffVsMult->Reset();
    mCloneFactorVsMult->Reset();
    mVtxResXVsMult->Reset();
    mVtxResYVsMult->Reset();
    mVtxResZVsMult->Reset();
    mVtxPullsXVsMult->Reset();
    mVtxPullsYVsMult->Reset();
    mVtxPullsZVsMult->Reset();
  }
}

} // namespace o2::quality_control_modules::rec
