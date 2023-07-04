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
/// \file   MeanVertexPostProcessing.cxx
/// \author Chiara Zampolli
///

#include "DataFormatsCalibration/MeanVertexObject.h"
#include "CCDB/CCDBTimeStampUtils.h"

#include "GLO/MeanVertexPostProcessing.h"
#include "QualityControl/QcInfoLogger.h"

#include <TMath.h>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::glo
{

TrendGraph::TrendGraph(std::string name, std::string title, std::string label, float rangeMin, float rangeMax)
  : TCanvas(name.c_str(), title.c_str()),
    mAxisLabel(label),
    mRangeMin(rangeMin),
    mRangeMax(rangeMax)
{
  mGraphHist = std::make_unique<TGraph>(0);

  mGraph = std::make_unique<TGraph>(0);
  mGraph->SetMarkerStyle(kCircle);
  mGraph->SetTitle(fmt::format("{};time;{}", title, label).c_str());
  mLines[0] = std::make_unique<TLine>(0, mRangeMin, 1, mRangeMin);
  mLines[1] = std::make_unique<TLine>(0, mRangeMax, 1, mRangeMax);
  mLines[0]->SetLineStyle(kDashed);
  mLines[1]->SetLineStyle(kDashed);
}

void TrendGraph::update(uint64_t time, float val)
{
  mGraph->AddPoint(time, val);
  mGraphHist->AddPoint(time, 0);

  Clear();
  cd();

  // Draw underlying histogram and format axes
  mGraphHist->Draw("A");
  auto hAxis = mGraphHist->GetHistogram();
  hAxis->SetTitle(GetTitle());
  hAxis->GetYaxis()->SetTitle(mAxisLabel.c_str());
  hAxis->GetXaxis()->SetTimeDisplay(1);
  hAxis->GetXaxis()->SetNdivisions(505);
  hAxis->GetXaxis()->SetTimeOffset(0.0);
  hAxis->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");

  // Adjust vertical axis range
  Double_t min = mRangeMin;
  Double_t max = mRangeMax;
  min = TMath::Min(min, TMath::MinElement(mGraph->GetN(), mGraph->GetY()));
  max = TMath::Max(max, TMath::MaxElement(mGraph->GetN(), mGraph->GetY()));
  auto delta = max - min;
  min -= 0.1 * delta;
  max += 0.1 * delta;

  // Update plot
  hAxis->SetMinimum(min);
  hAxis->SetMaximum(max);
  hAxis->Draw("AXIS");

  // Draw graph
  mGraph->Draw("PL,SAME");

  // Draw reference lines for acceptable ranges
  mLines[0]->SetX1(hAxis->GetXaxis()->GetXmin());
  mLines[1]->SetX1(hAxis->GetXaxis()->GetXmin());
  mLines[0]->SetX2(hAxis->GetXaxis()->GetXmax());
  mLines[1]->SetX2(hAxis->GetXaxis()->GetXmax());
  mLines[0]->Draw();
  mLines[1]->Draw();
}

//_________________________________________________________________________________________

MeanVertexPostProcessing::~MeanVertexPostProcessing()
{
  delete mX;
  delete mY;
  delete mZ;
  delete mStartValidity;
}

void MeanVertexPostProcessing::configure(const boost::property_tree::ptree& config)
{
  if (const auto& customConfigs = config.get_child_optional("qc.postprocessing." + getID() + ".customization"); customConfigs.has_value()) {
    for (const auto& customConfig : customConfigs.value()) {
      if (const auto& customNames = customConfig.second.get_child_optional("CcdbURL"); customNames.has_value()) {
        ILOG(Info, Support) << "MeanVertexCalib post-processing: getting customized CCDB url" << ENDM;
        mCcdbUrl = customConfig.second.get<std::string>("CcdbURL");
      }
      if (const auto& customNames = customConfig.second.get_child_optional("RangeX"); customNames.has_value()) {
        mRangeX = customConfig.second.get<float>("RangeX");
      }
      if (const auto& customNames = customConfig.second.get_child_optional("RangeY"); customNames.has_value()) {
        mRangeY = customConfig.second.get<float>("RangeY");
      }
      if (const auto& customNames = customConfig.second.get_child_optional("RangeZ"); customNames.has_value()) {
        mRangeZ = customConfig.second.get<float>("RangeZ");
      }
      if (const auto& customNames = customConfig.second.get_child_optional("RangeSigmaX"); customNames.has_value()) {
        mRangeSigmaX = customConfig.second.get<float>("RangeSigmaX");
      }
      if (const auto& customNames = customConfig.second.get_child_optional("RangeSigmaY"); customNames.has_value()) {
        mRangeSigmaY = customConfig.second.get<float>("RangeSigmaY");
      }
      if (const auto& customNames = customConfig.second.get_child_optional("RangeSigmaZ"); customNames.has_value()) {
        mRangeSigmaZ = customConfig.second.get<float>("RangeSigmaZ");
      }
      if (const auto& customNames = customConfig.second.get_child_optional("ResetHistos"); customNames.has_value()) {
        mResetHistos = customConfig.second.get<bool>("ResetHistos");
      }
    }
  }
  ILOG(Info, Support) << "MeanVertexCalib post-processing: CCDB url will be set to: " << mCcdbUrl << ENDM;
  mCcdbApi.init(mCcdbUrl);
}

void MeanVertexPostProcessing::initialize(Trigger, framework::ServiceRegistryRef)
{
  // QcInfoLogger::setDetector("GLO");
  mX = new TH1F("mMeanVtxX", "Mean Vertex X", 20, -100, 100);
  mY = new TH1F("mMeanVtxY", "Mean Vertex Y", 20, -100, 100);
  mZ = new TH1F("mMeanVtxZ", "Mean Vertex Z", 20, -100, 100);
  const long currentTime = o2::ccdb::getCurrentTimestamp();
  mStartValidity = new TH1F("mStartValidity", "Start Validity of Mean Vertex object", 600, currentTime - 1000 * 600, currentTime - 1000 + 60 * 1000 * 10 * 600); // 10 hours, with bins of 1 minutes, starting 10 minutes in the past
  getObjectsManager()->startPublishing(mX);
  getObjectsManager()->startPublishing(mY);
  getObjectsManager()->startPublishing(mZ);
  getObjectsManager()->startPublishing(mStartValidity);
  mGraphX = std::make_unique<TrendGraph>("MeanVtxXTrending", "Mean Vertex X", "cm", -mRangeX, mRangeX);
  mGraphY = std::make_unique<TrendGraph>("MeanVtxYTrending", "Mean Vertex Y", "cm", -mRangeY, mRangeY);
  mGraphZ = std::make_unique<TrendGraph>("MeanVtxZTrending", "Mean Vertex Z", "cm", -mRangeZ, mRangeZ);

  mGraphSigmaX = std::make_unique<TrendGraph>("MeanVtxSigmaXTrending", "Mean Vertex #sigma_{X}", "cm", 0, mRangeSigmaX);
  mGraphSigmaY = std::make_unique<TrendGraph>("MeanVtxSigmaYTrending", "Mean Vertex #sigma_{Y}", "cm", 0, mRangeSigmaY);
  mGraphSigmaZ = std::make_unique<TrendGraph>("MeanVtxSigmaZTrending", "Mean Vertex #sigma_{Z}", "cm", 0, mRangeSigmaZ);

  getObjectsManager()->startPublishing(mGraphX.get());
  getObjectsManager()->startPublishing(mGraphY.get());
  getObjectsManager()->startPublishing(mGraphZ.get());

  getObjectsManager()->startPublishing(mGraphSigmaX.get());
  getObjectsManager()->startPublishing(mGraphSigmaY.get());
  getObjectsManager()->startPublishing(mGraphSigmaZ.get());

  ILOG(Info, Support) << "MeanVertexCalib post-processing: Initialization done";
}

void MeanVertexPostProcessing::update(Trigger t, framework::ServiceRegistryRef)
{
  ILOG(Info, Support) << "Trigger type is: " << t.triggerType << ", the timestamp is " << t.timestamp << ENDM;
  std::map<std::string, std::string> md, headers;
  auto* meanVtx = mCcdbApi.retrieveFromTFileAny<o2::dataformats::MeanVertexObject>("GLO/Calib/MeanVertex", md, t.timestamp);
  if (!meanVtx) {
    ILOG(Info, Support) << "MeanVertexCalib post-processing: null object received for " << t.timestamp << ENDM;
    return;
  }

  if (mResetHistos) {
    mX->Reset();
    mY->Reset();
    mZ->Reset();
    mStartValidity->Reset();
  }

  // get values
  auto x = meanVtx->getX();
  auto y = meanVtx->getY();
  auto z = meanVtx->getZ();
  auto sx = meanVtx->getSigmaX();
  auto sy = meanVtx->getSigmaY();
  auto sz = meanVtx->getSigmaZ();

  // get time stamp
  headers = mCcdbApi.retrieveHeaders("GLO/Calib/MeanVertex", md, t.timestamp);
  const auto validFrom = headers.find("Valid-From");
  long startVal = std::stol(validFrom->second);
  ILOG(Info, Support) << "MeanVertexCalib post-processing: startValidity = " << startVal << " X = " << x << " Y = " << y << " Z = " << z << ENDM;
  mX->Fill(x);
  mY->Fill(y);
  mZ->Fill(z);
  mStartValidity->Fill(startVal);

  // ROOT expects time in seconds
  startVal /= 1000;

  mGraphX->update(startVal, x);
  mGraphY->update(startVal, y);
  mGraphZ->update(startVal, z);
  mGraphSigmaX->update(startVal, sx);
  mGraphSigmaY->update(startVal, sy);
  mGraphSigmaZ->update(startVal, sz);
}

void MeanVertexPostProcessing::finalize(Trigger, framework::ServiceRegistryRef)
{
  // Only if you don't want it to be published after finalisation.
  getObjectsManager()->stopPublishing(mX);
  getObjectsManager()->stopPublishing(mY);
  getObjectsManager()->stopPublishing(mZ);
  getObjectsManager()->stopPublishing(mStartValidity);
  getObjectsManager()->stopPublishing(mGraphX.get());
  getObjectsManager()->stopPublishing(mGraphY.get());
  getObjectsManager()->stopPublishing(mGraphZ.get());

  getObjectsManager()->stopPublishing(mGraphSigmaX.get());
  getObjectsManager()->stopPublishing(mGraphSigmaY.get());
  getObjectsManager()->stopPublishing(mGraphSigmaZ.get());
}

} // namespace o2::quality_control_modules::glo
