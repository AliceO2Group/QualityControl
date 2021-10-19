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
/// \file   CalibQcTask.cxx
/// \author Dmitri Peresunko
/// \author Dmitry Blau
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "QualityControl/QcInfoLogger.h"
#include "PHOS/CalibQcTask.h"
#include "Framework/InputRecord.h"
#include "PHOSBase/Geometry.h"
#include "PHOSBase/Mapping.h"

//using namespace o2::phos;

namespace o2::quality_control_modules::phos
{

CalibQcTask::~CalibQcTask()
{
  for (int i = NHIST2D; i--;) {
    if (mHist2D[i]) {
      mHist2D[i]->Delete();
      mHist2D[i] = nullptr;
    }
  }
}
void CalibQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  using infoCONTEXT = AliceO2::InfoLogger::InfoLoggerContext;
  infoCONTEXT context;
  context.setField(infoCONTEXT::FieldName::Facility, "QC");
  context.setField(infoCONTEXT::FieldName::System, "QC");
  context.setField(infoCONTEXT::FieldName::Detector, "PHS");
  QcInfoLogger::GetInstance().setContext(context);
  ILOG(Info, Support) << "initialize CalibQcTask" << AliceO2::InfoLogger::InfoLogger::endm;

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("pedestal"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Working in pedestal mode " << AliceO2::InfoLogger::InfoLogger::endm;
    if (param->second.find("on") != std::string::npos) {
      mMode = 1;
    }
  }
  if (auto param = mCustomParameters.find("LED"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Working in LED mode " << AliceO2::InfoLogger::InfoLogger::endm;
    if (param->second.find("on") != std::string::npos) {
      mMode = 2;
    }
  }
  if (auto param = mCustomParameters.find("BadMap"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Working in BadMap mode " << AliceO2::InfoLogger::InfoLogger::endm;
    if (param->second.find("on") != std::string::npos) {
      mMode = 0;
    }
  }

  //Prepare histograms
  if (mMode == 1) { //Pedestals
    for (Int_t mod = 0; mod < 4; mod++) {
      if (!mHist2D[kChangeHGM1 + mod]) {
        mHist2D[kChangeHGM1 + mod] = new TH2F(Form("HGPedestalChange%d", mod + 1), Form("Change of HG pedestals in mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
        mHist2D[kChangeHGM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
        mHist2D[kChangeHGM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
        mHist2D[kChangeHGM1 + mod]->GetXaxis()->SetTitle("x, cells");
        mHist2D[kChangeHGM1 + mod]->GetYaxis()->SetTitle("z, cells");
        mHist2D[kChangeHGM1 + mod]->SetStats(0);
        mHist2D[kChangeHGM1 + mod]->SetMinimum(0);
        // mHist2D[kChangeHGM1+mod]->SetMaximum(100) ;
        getObjectsManager()->startPublishing(mHist2D[kChangeHGM1 + mod]);
      } else {
        mHist2D[kChangeHGM1 + mod]->Reset();
      }
      if (!mHist2D[kChangeLGM1 + mod]) {
        mHist2D[kChangeLGM1 + mod] = new TH2F(Form("LGPedestalChange%d", mod + 1), Form("Change of LG pedestals in mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
        mHist2D[kChangeLGM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
        mHist2D[kChangeLGM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
        mHist2D[kChangeLGM1 + mod]->GetXaxis()->SetTitle("x, cells");
        mHist2D[kChangeLGM1 + mod]->GetYaxis()->SetTitle("z, cells");
        mHist2D[kChangeLGM1 + mod]->SetStats(0);
        mHist2D[kChangeLGM1 + mod]->SetMinimum(0);
        // mHist2D[kChangeLGM1+mod]->SetMaximum(100) ;
        getObjectsManager()->startPublishing(mHist2D[kChangeLGM1 + mod]);
      } else {
        mHist2D[kChangeLGM1 + mod]->Reset();
      }
    }
  }                 //Pedestals
  if (mMode == 2) { //LED
    for (Int_t mod = 0; mod < 4; mod++) {
      if (!mHist2D[kChangeHGM1 + mod]) {
        mHist2D[kChangeHGM1 + mod] = new TH2F(Form("HGLGRatioChange%d", mod + 1), Form("Change of HG/LG ratio in mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
        mHist2D[kChangeHGM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
        mHist2D[kChangeHGM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
        mHist2D[kChangeHGM1 + mod]->GetXaxis()->SetTitle("x, cells");
        mHist2D[kChangeHGM1 + mod]->GetYaxis()->SetTitle("z, cells");
        mHist2D[kChangeHGM1 + mod]->SetStats(0);
        mHist2D[kChangeHGM1 + mod]->SetMinimum(0);
        mHist2D[kChangeHGM1 + mod]->SetMaximum(20);
        getObjectsManager()->startPublishing(mHist2D[kChangeHGM1 + mod]);
      } else {
        mHist2D[kChangeHGM1 + mod]->Reset();
      }
    }
  }                 //LED
  if (mMode == 0) { //BadMap
    for (Int_t mod = 0; mod < 4; mod++) {
      if (!mHist2D[kChangeHGM1 + mod]) {
        mHist2D[kChangeHGM1 + mod] = new TH2F(Form("BadMapChange%d", mod + 1), Form("Change of bad map in mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
        mHist2D[kChangeHGM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
        mHist2D[kChangeHGM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
        mHist2D[kChangeHGM1 + mod]->GetXaxis()->SetTitle("x, cells");
        mHist2D[kChangeHGM1 + mod]->GetYaxis()->SetTitle("z, cells");
        mHist2D[kChangeHGM1 + mod]->SetStats(0);
        mHist2D[kChangeHGM1 + mod]->SetMinimum(-2);
        mHist2D[kChangeHGM1 + mod]->SetMaximum(2);
        getObjectsManager()->startPublishing(mHist2D[kChangeHGM1 + mod]);
      } else {
        mHist2D[kChangeHGM1 + mod]->Reset();
      }
    }
  } //BadMap
}

void CalibQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  reset();
}

void CalibQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void CalibQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  //std::array<short, 2*o2::phos::Mapping::NCHANNELS>
  auto diff = ctx.inputs().get<gsl::span<short>>("calibdiff");
  bool fillLG = diff.size() > o2::phos::Mapping::NCHANNELS + 1;

  char relid[3];
  for (short absId = o2::phos::Mapping::NCHANNELS; absId > 1792; absId--) {

    o2::phos::Geometry::absToRelNumbering(absId, relid);
    mHist2D[kChangeHGM1 + relid[0] - 1]->SetBinContent(relid[1], relid[2], diff[absId]);
    if (fillLG && mMode == 1) { //Only in pedestals prepare both histos
      mHist2D[kChangeLGM1 + relid[0] - 1]->SetBinContent(relid[1], relid[2], diff[absId + o2::phos::Mapping::NCHANNELS]);
    }
  }

} //function monitor data

void CalibQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void CalibQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void CalibQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  for (int i = NHIST2D; i--;) {
    if (mHist2D[i]) {
      mHist2D[i]->Reset();
    }
  }
}
} // namespace o2::quality_control_modules::phos
