// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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
  QcInfoLogger::GetInstance() << "initialize CalibQcTask" << AliceO2::InfoLogger::InfoLogger::endm;

  // // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  // if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
  //   QcInfoLogger::GetInstance() << "Custom parameter - myOwnKey : " << param->second << AliceO2::InfoLogger::InfoLogger::endm;
  // }

  //TODO: configure reading bad map from CCDB
  mBadMap.reset(new o2::phos::BadChannelsMap());

  //Prepare histograms
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
}

void CalibQcTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  reset();
}

void CalibQcTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void CalibQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  //std::array<short, 2*o2::phos::Mapping::NCHANNELS>
  auto diff = ctx.inputs().get<gsl::span<short>>("CALIBDIFF"); 
  bool fillLG = diff.size()>o2::phos::Mapping::NCHANNELS;

  char relid[3];
  for (short absId = o2::phos::Mapping::NCHANNELS; absId > 1792; absId--) {
   
    o2::phos::Geometry::absToRelNumbering(absId, relid);
printf("absId=%d, relid=[%d,%d,%d]\n",absId, relid[0], relid[1], relid[2]);    
    mHist2D[kChangeHGM1 + relid[0]]->SetBinContent(relid[1],relid[2],diff[absId]) ;
    if(fillLG){
      mHist2D[kChangeLGM1 + relid[0]]->SetBinContent(relid[1],relid[2],diff[absId+o2::phos::Mapping::NCHANNELS]) ;
    }
  }

} //function monitor data

void CalibQcTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void CalibQcTask::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void CalibQcTask::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  for (int i = NHIST2D; i--;) {
    if (mHist2D[i]) {
      mHist2D[i]->Reset();
    }
  }
}
} // namespace o2::quality_control_modules::phos
