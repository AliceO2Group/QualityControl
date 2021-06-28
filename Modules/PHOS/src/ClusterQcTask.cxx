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
/// \file   ClusterQcTask.cxx
/// \author Dmitri Peresunko
/// \author Dmitry Blau
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TMath.h>
#include <cfloat>

#include "QualityControl/QcInfoLogger.h"
#include "PHOS/ClusterQcTask.h"
#include "DataFormatsPHOS/TriggerRecord.h"
#include "Framework/InputRecord.h"

//using namespace o2::phos;

namespace o2::quality_control_modules::phos
{

ClusterQcTask::~ClusterQcTask()
{
  for (int i = kNhist1D; i--;) {
    if (mHist1D[i]) {
      mHist1D[i]->Delete();
      mHist1D[i] = nullptr;
    }
  }
  for (int i = kNhist2D; i--;) {
    if (mHist2D[i]) {
      mHist2D[i]->Delete();
      mHist2D[i] = nullptr;
    }
  }
}
void ClusterQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  using infoCONTEXT = AliceO2::InfoLogger::InfoLoggerContext;
  infoCONTEXT context;
  context.setField(infoCONTEXT::FieldName::Facility, "QC");
  context.setField(infoCONTEXT::FieldName::System, "QC");
  context.setField(infoCONTEXT::FieldName::Detector, "PHS");
  QcInfoLogger::GetInstance().setContext(context);
  QcInfoLogger::GetInstance() << "initialize ClusterQcTask" << AliceO2::InfoLogger::InfoLogger::endm;

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    QcInfoLogger::GetInstance() << "Custom parameter - myOwnKey : " << param->second << AliceO2::InfoLogger::InfoLogger::endm;
  }

  //read alignment to calculate cluster global coordinates
  mGeom = o2::phos::Geometry::GetInstance("Run3");

  //TODO: configure reading bad map from CCDB
  mBadMap.reset(new o2::phos::BadChannelsMap());

  //Prepare histograms
  for (Int_t mod = 0; mod < 4; mod++) {
    if (!mHist2D[kOccupancyM1 + mod]) {
      mHist2D[kOccupancyM1 + mod] = new TH2F(Form("ClusterOccupancyM%d", mod + 1), Form("Cluster occupancy, mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
      mHist2D[kOccupancyM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2D[kOccupancyM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2D[kOccupancyM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2D[kOccupancyM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2D[kOccupancyM1 + mod]->SetStats(0);
      mHist2D[kOccupancyM1 + mod]->SetMinimum(0);
      // mHist2D[kOccupancyM1+mod]->SetMaximum(100) ;
      getObjectsManager()->startPublishing(mHist2D[kOccupancyM1 + mod]);
    } else {
      mHist2D[kOccupancyM1 + mod]->Reset();
    }

    if (!mHist2D[kTimeEM1 + mod]) {
      mHist2D[kTimeEM1 + mod] = new TH2F(Form("TimevsE%d", mod + 1), Form("Cell time vs energy, mod %d", mod + 1), 50, 0., 10., 50, -2.e-7, 2.e-7);
      mHist2D[kTimeEM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2D[kTimeEM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2D[kTimeEM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2D[kTimeEM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2D[kTimeEM1 + mod]->SetStats(0);
      mHist2D[kTimeEM1 + mod]->SetMinimum(0);
      // mHist2D[kTimeEM1 + mod]->SetMaximum(100);
      getObjectsManager()->startPublishing(mHist2D[kTimeEM1 + mod]);
    } else {
      mHist2D[kTimeEM1 + mod]->Reset();
    }

    if (!mHist1D[kSpectrumM1 + mod]) {
      mHist1D[kSpectrumM1 + mod] = new TH1F(Form("SpectrumM%d", mod + 1), Form("Cluster spectrum in mod %d", mod + 1), 100, 0., 10.);
      mHist1D[kSpectrumM1 + mod]->GetXaxis()->SetTitle("GeV");
      mHist1D[kSpectrumM1 + mod]->SetStats(0);
      mHist1D[kSpectrumM1 + mod]->SetMinimum(0);
      getObjectsManager()->startPublishing(mHist1D[kSpectrumM1 + mod]);
    } else {
      mHist1D[kSpectrumM1 + mod]->Reset();
    }
    if (!mHist1D[kPi0M1 + mod]) {
      mHist1D[kPi0M1 + mod] = new TH1F(Form("InvMassM%d", mod + 1), Form("inv mass %d", mod + 1), 100, 0., 0.5);
      mHist1D[kPi0M1 + mod]->GetXaxis()->SetTitle("m_{#gamma#gamma} (GeV/c^{2})");
      mHist1D[kPi0M1 + mod]->SetStats(0);
      mHist1D[kPi0M1 + mod]->SetMinimum(0);
      getObjectsManager()->startPublishing(mHist1D[kPi0M1 + mod]);
    } else {
      mHist1D[kPi0M1 + mod]->Reset();
    }
  }
}

void ClusterQcTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  reset();
}

void ClusterQcTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ClusterQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  auto clusters = ctx.inputs().get<gsl::span<o2::phos::Cluster>>("clusters");
  auto cluTR = ctx.inputs().get<gsl::span<o2::phos::TriggerRecord>>("clustertr");

  for (auto& tr : cluTR) {

    int firstCluInEvent = tr.getFirstEntry();
    int lastCluInEvent = firstCluInEvent + tr.getNumberOfObjects();

    for (int m = 4; m--;) {
      mBuffer[m].clear();
    }
    for (int i = firstCluInEvent; i < lastCluInEvent; i++) {
      const o2::phos::Cluster& clu = clusters[i];

      int mod = clu.module();
      //Fill occupancy and time-E histos
      float posX, posZ;
      clu.getLocalPosition(posX, posZ);
      short absId;
      mGeom->relPosToAbsId(mod, posX, posZ, absId);
      char relid[3];
      mGeom->absToRelNumbering(absId, relid);

      float e = clu.getEnergy();
      if (e > mOccCut) {
        mHist2D[kOccupancyM1 + mod - 1]->Fill(relid[1] - 0.5, relid[2] - 0.5);
      }
      mHist2D[kTimeEM1 + mod - 1]->Fill(e, clu.getTime());
      mHist1D[kSpectrumM1 + mod - 1]->Fill(e);

      if (!checkCluster(clu)) {
        continue;
      }
      // prepare TLorentsVector
      TVector3 vec3;
      mGeom->local2Global(mod, posX, posZ, vec3);
      double norm = vec3.Mag();
      TLorentzVector v(vec3.X() * e / norm, vec3.Y() * e / norm, vec3.Z() * e / norm, e);
      for (const TLorentzVector& v2 : mBuffer[mod - 1]) {
        TLorentzVector sum = v + v2;
        if (sum.Pt() > mPtMin) {
          mHist1D[kPi0M1 + mod - 1]->Fill(sum.M());
        }
      }
      mBuffer[mod - 1].emplace_back(v);
    }
  }

} //function monitor data

void ClusterQcTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ClusterQcTask::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ClusterQcTask::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  for (int i = kNhist1D; i--;) {
    if (mHist1D[i]) {
      mHist1D[i]->Reset();
    }
  }
  for (int i = kNhist2D; i--;) {
    if (mHist2D[i]) {
      mHist2D[i]->Reset();
    }
  }
}
bool ClusterQcTask::checkCluster(const o2::phos::Cluster& clu)
{
  //First check BadMap
  float posX, posZ;
  clu.getLocalPosition(posX, posZ);
  short absId;
  o2::phos::Geometry::relPosToAbsId(clu.module(), posX, posZ, absId);
  if (!mBadMap->isChannelGood(absId)) {
    return false;
  }

  return (clu.getEnergy() > 0.3 && clu.getMultiplicity() > 1);
}

} // namespace o2::quality_control_modules::phos
