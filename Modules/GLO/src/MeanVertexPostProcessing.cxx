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
/// \author Chiara Zampolli, Andrea Ferrero
///

#include "GLO/MeanVertexPostProcessing.h"
#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsCalibration/MeanVertexObject.h"
#include "CCDB/CCDBTimeStampUtils.h"

// ROOT
#include <TGraph.h>
#include <TAxis.h>
#include <TMath.h>

#include <boost/property_tree/ptree.hpp>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::glo
{

class TrendGraph : public TGraph
{
 public:
  TrendGraph(std::string name, std::string title, std::string label)
    : TGraph(0),
      mAxisLabel(label)
  {
    SetMarkerStyle(kCircle);
    SetNameTitle(name.c_str(), title.c_str());
  }

  ~TrendGraph() override = default;

  void update(uint64_t time, float val)
  {
    AddPoint(time, val);
    Draw("APL");
    GetYaxis()->SetTitle(mAxisLabel.c_str());
    GetXaxis()->SetTimeDisplay(1);
    GetXaxis()->SetNdivisions(505);
    GetXaxis()->SetTimeOffset(0.0);
    GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
  }

 private:
  std::string mAxisLabel;
  std::unique_ptr<TGraph> mGraph;
};

//_________________________________________________________________________________________

void MeanVertexPostProcessing::configure(const boost::property_tree::ptree& config)
{
  if (const auto& customConfigs = config.get_child_optional("qc.postprocessing." + getID() + ".customization"); customConfigs.has_value()) {
    for (const auto& customConfig : customConfigs.value()) {
      if (const auto& customNames = customConfig.second.get_child_optional("CcdbURL"); customNames.has_value()) {
        ILOG(Info, Support) << "MeanVertexCalib post-processing: getting customized CCDB url" << ENDM;
        mCcdbUrl = customConfig.second.get<std::string>("CcdbURL");
      }
    }
  }
  ILOG(Info, Support) << "MeanVertexCalib post-processing: CCDB url will be set to: " << mCcdbUrl << ENDM;
  mCcdbApi.init(mCcdbUrl);

  // create the graphs
  mGraphX = std::make_shared<TrendGraph>("MeanVtxXTrending", "Mean Vertex X", "cm");
  mGraphY = std::make_shared<TrendGraph>("MeanVtxYTrending", "Mean Vertex Y", "cm");
  mGraphZ = std::make_shared<TrendGraph>("MeanVtxZTrending", "Mean Vertex Z", "cm");

  mGraphSigmaX = std::make_shared<TrendGraph>("MeanVtxSigmaXTrending", "Mean Vertex #sigma_{X}", "cm");
  mGraphSigmaY = std::make_shared<TrendGraph>("MeanVtxSigmaYTrending", "Mean Vertex #sigma_{Y}", "cm");
  mGraphSigmaZ = std::make_shared<TrendGraph>("MeanVtxSigmaZTrending", "Mean Vertex #sigma_{Z}", "cm");
}

void MeanVertexPostProcessing::initialize(Trigger, framework::ServiceRegistryRef)
{
  // publish the graphs
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
  std::map<std::string, std::string> md;
  auto* meanVtx = mCcdbApi.retrieveFromTFileAny<o2::dataformats::MeanVertexObject>("GLO/Calib/MeanVertex", md, t.timestamp);
  if (!meanVtx) {
    ILOG(Info, Support) << "MeanVertexCalib post-processing: null object received for " << t.timestamp << ENDM;
    return;
  }

  // get values
  auto x = meanVtx->getX();
  auto y = meanVtx->getY();
  auto z = meanVtx->getZ();
  auto sx = meanVtx->getSigmaX();
  auto sy = meanVtx->getSigmaY();
  auto sz = meanVtx->getSigmaZ();

  // get time stamp
  std::map<std::string, std::string> headers;
  headers = mCcdbApi.retrieveHeaders("GLO/Calib/MeanVertex", md, t.timestamp);
  const auto validFrom = headers.find("Valid-From");
  long startVal = std::stol(validFrom->second);
  ILOG(Debug, Support) << "MeanVertexCalib post-processing: startValidity = " << startVal << " X = " << x << " Y = " << y << " Z = " << z << ENDM;

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
}

} // namespace o2::quality_control_modules::glo
