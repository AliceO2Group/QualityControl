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
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRef.h>
#include <Framework/DataRefUtils.h>
#include <DataFormatsPHOS/Pedestals.h>
#include <DataFormatsPHOS/CalibParams.h>

// using namespace o2::phos;

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
  ILOG(Info, Support) << "==============initialize CalibQcTask==============" << ENDM;
  ILOG(Debug, Devel) << "initialize CalibQcTask" << ENDM;
  using infoCONTEXT = AliceO2::InfoLogger::InfoLoggerContext;
  infoCONTEXT context;
  context.setField(infoCONTEXT::FieldName::Facility, "QC");
  context.setField(infoCONTEXT::FieldName::System, "QC");
  context.setField(infoCONTEXT::FieldName::Detector, "PHS");
  QcInfoLogger::GetInfoLogger().setContext(context);

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("pedestal"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Working in pedestal mode " << ENDM;
    if (param->second.find("on") != std::string::npos) {
      mMode = 1;
    }
  }
  if (auto param = mCustomParameters.find("LED"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Working in LED mode " << ENDM;
    if (param->second.find("on") != std::string::npos) {
      mMode = 2;
    }
  }
  if (auto param = mCustomParameters.find("BadMap"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Working in BadMap mode " << ENDM;
    if (param->second.find("on") != std::string::npos) {
      mMode = 0;
    }
  }

  if (auto param = mCustomParameters.find("L1phase"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Working in L1phase mode" << ENDM;
    if (param->second.find("on") != std::string::npos) {
      mMode = 3;
    }
  }

  ILOG(Info, Support) << "==============Prepare Histos===============" << ENDM;
  // Prepare histograms
  if (mMode == 1) { // Pedestals
    for (Int_t mod = 0; mod < 4; mod++) {
      if (!mHist2D[kChangeHGM1 + mod]) {
        // pedestal values
        mHist2D[kValueHGM1 + mod] = new TH2F(Form("HGPedestalValue%d", mod + 1), Form("Values of HG pedestals in mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
        mHist2D[kValueHGM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
        mHist2D[kValueHGM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
        mHist2D[kValueHGM1 + mod]->GetXaxis()->SetTitle("x, cells");
        mHist2D[kValueHGM1 + mod]->GetYaxis()->SetTitle("z, cells");
        mHist2D[kValueHGM1 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[kValueHGM1 + mod]);
        // pedestal RMS
        mHist2D[kRMSHGM1 + mod] = new TH2F(Form("HGPedestalRMS%d", mod + 1), Form("RMSs of HG pedestals in mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
        mHist2D[kRMSHGM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
        mHist2D[kRMSHGM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
        mHist2D[kRMSHGM1 + mod]->GetXaxis()->SetTitle("x, cells");
        mHist2D[kRMSHGM1 + mod]->GetYaxis()->SetTitle("z, cells");
        mHist2D[kRMSHGM1 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[kRMSHGM1 + mod]);

        // pedestal change
        mHist2D[kChangeHGM1 + mod] = new TH2F(Form("HGPedestalChange%d", mod + 1), Form("Change of HG pedestals in mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
        mHist2D[kChangeHGM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
        mHist2D[kChangeHGM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
        mHist2D[kChangeHGM1 + mod]->GetXaxis()->SetTitle("x, cells");
        mHist2D[kChangeHGM1 + mod]->GetYaxis()->SetTitle("z, cells");
        mHist2D[kChangeHGM1 + mod]->SetStats(0);
        mHist2D[kChangeHGM1 + mod]->SetMinimum(-5);
        mHist2D[kChangeHGM1 + mod]->SetMaximum(5);
        getObjectsManager()->startPublishing(mHist2D[kChangeHGM1 + mod]);

      } else {
        mHist2D[kValueHGM1 + mod]->Reset();
        mHist2D[kRMSHGM1 + mod]->Reset();
        mHist2D[kChangeHGM1 + mod]->Reset();
      }
      if (!mHist2D[kChangeLGM1 + mod]) {
        // pedestal values
        mHist2D[kValueLGM1 + mod] = new TH2F(Form("LGPedestalValue%d", mod + 1), Form("Values of LG pedestals in mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
        mHist2D[kValueLGM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
        mHist2D[kValueLGM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
        mHist2D[kValueLGM1 + mod]->GetXaxis()->SetTitle("x, cells");
        mHist2D[kValueLGM1 + mod]->GetYaxis()->SetTitle("z, cells");
        mHist2D[kValueLGM1 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[kValueLGM1 + mod]);
        // pedestal RMS
        mHist2D[kRMSLGM1 + mod] = new TH2F(Form("LGPedestalRMS%d", mod + 1), Form("RMSs of LG pedestals in mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
        mHist2D[kRMSLGM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
        mHist2D[kRMSLGM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
        mHist2D[kRMSLGM1 + mod]->GetXaxis()->SetTitle("x, cells");
        mHist2D[kRMSLGM1 + mod]->GetYaxis()->SetTitle("z, cells");
        mHist2D[kRMSLGM1 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[kRMSLGM1 + mod]);

        // pedestal change
        mHist2D[kChangeLGM1 + mod] = new TH2F(Form("LGPedestalChange%d", mod + 1), Form("Change of LG pedestals in mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
        mHist2D[kChangeLGM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
        mHist2D[kChangeLGM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
        mHist2D[kChangeLGM1 + mod]->GetXaxis()->SetTitle("x, cells");
        mHist2D[kChangeLGM1 + mod]->GetYaxis()->SetTitle("z, cells");
        mHist2D[kChangeLGM1 + mod]->SetStats(0);
        mHist2D[kChangeLGM1 + mod]->SetMinimum(-5);
        mHist2D[kChangeLGM1 + mod]->SetMaximum(5);
        getObjectsManager()->startPublishing(mHist2D[kChangeLGM1 + mod]);

      } else {
        mHist2D[kValueLGM1 + mod]->Reset();
        mHist2D[kRMSLGM1 + mod]->Reset();
        mHist2D[kChangeLGM1 + mod]->Reset();
      }
    }
  }                 // Pedestals
  if (mMode == 2) { // LED
    for (Int_t mod = 0; mod < 4; mod++) {
      if (!mHist2D[kChangeHGM1 + mod]) {
        // HGLG ratio value
        mHist2D[kValueHGM1 + mod] = new TH2F(Form("HGLGRatioValue%d", mod + 1), Form("Value of HG/LG ratio in mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
        mHist2D[kValueHGM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
        mHist2D[kValueHGM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
        mHist2D[kValueHGM1 + mod]->GetXaxis()->SetTitle("x, cells");
        mHist2D[kValueHGM1 + mod]->GetYaxis()->SetTitle("z, cells");
        mHist2D[kValueHGM1 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[kValueHGM1 + mod]);
        // HGLG ratio change
        mHist2D[kChangeHGM1 + mod] = new TH2F(Form("HGLGRatioChange%d", mod + 1), Form("Change of HG/LG ratio in mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
        mHist2D[kChangeHGM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
        mHist2D[kChangeHGM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
        mHist2D[kChangeHGM1 + mod]->GetXaxis()->SetTitle("x, cells");
        mHist2D[kChangeHGM1 + mod]->GetYaxis()->SetTitle("z, cells");
        mHist2D[kChangeHGM1 + mod]->SetStats(0);
        getObjectsManager()->startPublishing(mHist2D[kChangeHGM1 + mod]);

      } else {
        mHist2D[kValueHGM1 + mod]->Reset();
        mHist2D[kChangeHGM1 + mod]->Reset();
      }
    }
  }                 // LED
  if (mMode == 0) { // BadMap
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
  }                 // BadMap
  if (mMode == 3) { // L1phase
    if (!mHist2D[kL1phase]) {
      mHist2D[kL1phase] = new TH2F("L1phase", "Time vs DDL", 14, 0., 14., 100, -200.e-9, 200.e-9);
      mHist2D[kL1phase]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2D[kL1phase]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2D[kL1phase]->GetXaxis()->SetTitle("DDL");
      mHist2D[kL1phase]->GetYaxis()->SetTitle("t (s)");
      mHist2D[kL1phase]->SetDrawOption("colz");
      mHist2D[kL1phase]->SetStats(0);
      getObjectsManager()->startPublishing(mHist2D[kL1phase]);
    } else {
      mHist2D[kL1phase]->Reset();
    }
  } // BadMap
  ILOG(Info, Support) << " CalibQcTask histos ready " << ENDM;
}

void CalibQcTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  reset();
}

void CalibQcTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void CalibQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  bool hasCalibDiff = false, hasCalibPars = false, hasPedestals = false, hasL1phase = false;
  for (auto&& input : o2::framework::InputRecordWalker(ctx.inputs())) {
    // get message header
    if (input.header != nullptr && input.payload != nullptr) {
      auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(input);
      // get payload of a specific input, which is a char array.
      // const char* payload = input.payload;
      const auto* header = o2::framework::DataRefUtils::getHeader<header::DataHeader*>(input);
      if (payloadSize) {
        if ((strcmp(header->dataOrigin.str, "PHS") == 0) && (strcmp(header->dataDescription.str, "CALIBDIFF") == 0)) {
          // LOG(info) << "monitorData() : I found calibdiff in inputs";
          hasCalibDiff = true;
        }
        if ((strcmp(header->dataOrigin.str, "CLP") == 0) && (strcmp(header->dataDescription.str, "PHOS_HGLGratio") == 0)) {
          // LOG(info) << "monitorData() : I found PHOS_HGLGratio in inputs";
          hasCalibPars = true;
        }
        if ((strcmp(header->dataOrigin.str, "CLP") == 0) && (strcmp(header->dataDescription.str, "PHOS_Pedestal") == 0)) {
          // LOG(info) << "monitorData() : I found PHOS_Pedestal in inputs";
          hasPedestals = true;
        }
        if ((strcmp(header->dataOrigin.str, "CLP") == 0) && (strcmp(header->dataDescription.str, "PHOS_L1phase") == 0)) {
          // LOG(info) << "monitorData() : I found PHOS_L1phase in inputs";
          hasL1phase = true;
        }
      }
    }
  }

  // std::array<short, 2*o2::phos::Mapping::NCHANNELS>
  if (mMode == 0) { // Bad map
    if (hasCalibDiff) {
      auto diff = ctx.inputs().get<gsl::span<short>>("calibdiff");
      char relid[3];
      for (short absId = o2::phos::Mapping::NCHANNELS; absId > 1792; absId--) {
        o2::phos::Geometry::absToRelNumbering(absId, relid);
        mHist2D[kChangeHGM1 + relid[0] - 1]->SetBinContent(relid[1], relid[2], diff[absId]);
      }
    }
  } else if (mMode == 1) { // Pedestal
    if (hasCalibDiff) {
      auto diff = ctx.inputs().get<gsl::span<short>>("calibdiff");
      char relid[3];
      for (short absId = o2::phos::Mapping::NCHANNELS; absId > 1792; absId--) {
        o2::phos::Geometry::absToRelNumbering(absId, relid);
        mHist2D[kChangeHGM1 + relid[0] - 1]->SetBinContent(relid[1], relid[2], diff[absId]);
        mHist2D[kChangeLGM1 + relid[0] - 1]->SetBinContent(relid[1], relid[2], diff[absId + o2::phos::Mapping::NCHANNELS]);
      }
    }
    if (hasPedestals) {
      auto peds = o2::framework::DataRefUtils::as<o2::framework::CCDBSerialized<o2::phos::Pedestals>>(ctx.inputs().get<o2::framework::DataRef>("peds"));
      char relid[3];
      for (short absId = o2::phos::Mapping::NCHANNELS; absId > 1792; absId--) {
        o2::phos::Geometry::absToRelNumbering(absId, relid);
        mHist2D[kValueHGM1 + relid[0] - 1]->SetBinContent(relid[1], relid[2], peds->getHGPedestal(absId));
        mHist2D[kValueLGM1 + relid[0] - 1]->SetBinContent(relid[1], relid[2], peds->getLGPedestal(absId));
        mHist2D[kRMSHGM1 + relid[0] - 1]->SetBinContent(relid[1], relid[2], peds->getHGRMS(absId));
        mHist2D[kRMSLGM1 + relid[0] - 1]->SetBinContent(relid[1], relid[2], peds->getLGRMS(absId));
      }
    }
  } else if (mMode == 2) { // LED
    if (hasCalibDiff) {
      auto diff = ctx.inputs().get<gsl::span<float>>("calibdiff");
      char relid[3];
      for (short absId = o2::phos::Mapping::NCHANNELS; absId > 1792; absId--) {
        o2::phos::Geometry::absToRelNumbering(absId, relid);
        mHist2D[kChangeHGM1 + relid[0] - 1]->SetBinContent(relid[1], relid[2], diff[absId]);
      }
    }
    if (hasCalibPars) {
      auto calibParams = o2::framework::DataRefUtils::as<o2::framework::CCDBSerialized<o2::phos::CalibParams>>(ctx.inputs().get<o2::framework::DataRef>("calibpars"));
      char relid[3];
      for (short absId = o2::phos::Mapping::NCHANNELS; absId > 1792; absId--) {
        o2::phos::Geometry::absToRelNumbering(absId, relid);
        mHist2D[kValueHGM1 + relid[0] - 1]->SetBinContent(relid[1], relid[2], calibParams->getHGLGRatio(absId));
      }
    }

  } else if (mMode == 3) { // L1phase
    if (hasL1phase) {
      auto vec = ctx.inputs().get<gsl::span<unsigned int>>("l1phase");
      for (short it = 0; it < vec.size(); it++) {
        mHist2D[kL1phase]->SetBinContent(it / 100 + 1, it % 100 + 1, float(vec[it]));
      }
    }
  }
}

void CalibQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void CalibQcTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void CalibQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  for (int i = NHIST2D; i--;) {
    if (mHist2D[i]) {
      mHist2D[i]->Reset();
    }
  }
}
} // namespace o2::quality_control_modules::phos
