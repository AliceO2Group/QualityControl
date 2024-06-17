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
/// \file   PID.cxx
/// \author Jens Wiechula
///

// root includes
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TF1.h>
#include <TProfile.h>

// O2 includes
#include "Framework/ProcessingContext.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "TPCQC/Helpers.h"
#include <Framework/InputRecord.h>

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/PID.h"
#include "Common/Utils.h"

namespace o2::quality_control_modules::tpc
{

PID::PID() : TaskInterface() {}

PID::~PID()
{
}

void PID::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TPC PID QC task" << ENDM;
  // elementary cuts for PID from json file
  const int cutMinNCluster = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "cutMinNCluster");
  const float cutAbsTgl = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutAbsTgl");
  const float cutMindEdxTot = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutMindEdxTot");
  const float cutMaxdEdxTot = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutMaxdEdxTot");
  const float cutMinpTPC = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutMinpTPC");
  const float cutMaxpTPC = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutMaxpTPC");
  const float cutMinpTPCMIPs = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutMinpTPCMIPs");
  const float cutMaxpTPCMIPs = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "cutMaxpTPCMIPs");
  const int createCanvas = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "createCanvas");
  const bool runAsyncAndTurnOffSomeHistos = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "turnOffHistosForAsync");

  // set track cuts defaults are (AbsEta = 1.0, nCluster = 60, MindEdxTot  = 20)
  mQCPID.setPIDCuts(cutMinNCluster, cutAbsTgl, cutMindEdxTot, cutMaxdEdxTot, cutMinpTPC, cutMaxpTPC, cutMinpTPCMIPs, cutMaxpTPCMIPs, runAsyncAndTurnOffSomeHistos);
  mQCPID.setCreateCanvas(createCanvas);
  mQCPID.initializeHistograms();
  //mSeparationPowerCanvas = mQCPID.getSeparationPowerCanvas();
  //getObjectsManager()->startPublishing(mSeparationPowerCanvas);
  // pass map of vectors of histograms to be beautified!

  o2::tpc::qc::helpers::setStyleHistogramsInMap(mQCPID.getMapOfHisto());
  for (auto const& pair : mQCPID.getMapOfHisto()) {
    for (auto& hist : pair.second) {
      getObjectsManager()->startPublishing(hist.get());
    }
  }
  for (auto const& pair : mQCPID.getMapOfCanvas()) {
    for (auto& canv : pair.second) {
      getObjectsManager()->startPublishing(canv.get());
    }
  }
}

void PID::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  mQCPID.resetHistograms();
}

void PID::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void PID::monitorData(o2::framework::ProcessingContext& ctx)
{
  using TrackType = std::vector<o2::tpc::TrackTPC>;
  auto tracks = ctx.inputs().get<TrackType>("inputTracks");
  // ILOG(Info, Support) << "monitorData: " << tracks.size() << ENDM;

  for (auto const& track : tracks) {
    mQCPID.processTrack(track, tracks.size());
  }
}

void PID::endOfCycle()
{
  // ===| Fitting Histogram for separation Power |============================================================
  std::unique_ptr<TF1> fitFunc = std::make_unique<TF1>("fitFunc", "[0]*exp(-0.5*((x-[1])/[2])^2) + [3]*exp(-0.5*((x-[4])/[5])^2)", 0, 100);
  
  const int nPars = 8; //6 fit parameters + 2 external (seperation power + chi²/ndf)

  for (auto const& pair : mQCPID.getMapOfHisto()) {
    for (auto& hist : pair.second) {
      //      if (std::static_cast<string>(pair.first).compare("hdEdxMaxMIP") == 0) {
      if (pair.first.compare("hdEdxMaxMIP") == 0) {
        mTrendingParameters.clear();
        // Define fitting function: sum of two Gaussians with an offset
        // Set initial parameters for the fit
        fitFunc->SetParameter(0, 3000); // Amplitude of the first Gaussian
        fitFunc->SetParameter(1, 50);   // Mean of the first Gaussian
        fitFunc->SetParLimits(1, 45, 55);
        fitFunc->SetParameter(2, 2);   // Sigma of the first Gaussian
        fitFunc->SetParameter(3, 100); // Amplitude of the second Gaussian
        fitFunc->SetParameter(4, 75);  // Mean of the second Gaussian
        fitFunc->SetParLimits(4, 60, 90);
        fitFunc->SetParameter(5, 10); // Sigma of the second Gaussian

        // Fit the histogram with the fitting function
        hist->Fit(fitFunc.get(), "QRN");
        for (int iPar = 0; iPar < nPars - 2; iPar++) {
          mTrendingParameters.emplace_back(fitFunc->GetParameter(iPar)); //save fit parameters
        }
        // Retrieve parameters of the fitted function

        const double sepPow = (mTrendingParameters[4] - mTrendingParameters[1]) / (mTrendingParameters[2] / 2. + mTrendingParameters[5] / 2.); //separation power
        mTrendingParameters.emplace_back(sepPow);
        mTrendingParameters.emplace_back(fitFunc->GetChisquare() / fitFunc->GetNDF()); //chi²/ndf of fit
      }
    }
  }

  pSeparationPower = new TProfile("pSeparationPower", "pSeparationPower", nPars, 0., (float)nPars);
  getObjectsManager()->startPublishing(pSeparationPower);
  const TString binLabels[nPars] = { "Amplitude Pi", "Mean Pi", "Sigma Pi", "Amplitude El", "Mean El", "Sigma El", "Separation Power", "chiSquare/ndf" };

  for (int iPar = 0; iPar < nPars; iPar++) {
    pSeparationPower->GetXaxis()->SetBinLabel(iPar + 1, binLabels[iPar]);
    pSeparationPower->Fill((float)iPar + 0.5, mTrendingParameters[iPar]);
  }
  //mSeparationPowerCanvas->cd();
  //pSeparationPower->Draw();
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void PID::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void PID::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mQCPID.resetHistograms();
}

} // namespace o2::quality_control_modules::tpc
