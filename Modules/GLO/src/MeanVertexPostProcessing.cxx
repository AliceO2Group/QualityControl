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

#include <TH1F.h>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::glo
{

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
  mStartValidity = new TH1F("mStartValidity", "Start Validity of Mean Vertex object", 600, currentTime - 1000, currentTime - 1000 + 60 * 1000 * 10 * 600); // 10 hours, with bins of 1 minutes
  getObjectsManager()->startPublishing(mX);
  getObjectsManager()->startPublishing(mY);
  getObjectsManager()->startPublishing(mZ);
  getObjectsManager()->startPublishing(mStartValidity);
  ILOG(Info, Support) << "MeanVertexCalib post-processing: Initialization done";
}

void MeanVertexPostProcessing::update(Trigger t, framework::ServiceRegistryRef)
{
  ILOG(Info, Support) << "Trigger type is: " << t.triggerType << ", the timestamp is " << t.timestamp << ENDM;
  std::map<std::string, std::string> md, headers;
  long currentTime = o2::ccdb::getCurrentTimestamp();
  auto* meanVtx = mCcdbApi.retrieveFromTFileAny<o2::dataformats::MeanVertexObject>("GLO/Calib/MeanVertex", md, currentTime);
  auto x = meanVtx->getX();
  auto y = meanVtx->getY();
  auto z = meanVtx->getZ();
  headers = mCcdbApi.retrieveHeaders("GLO/Calib/MeanVertex", md, currentTime);
  const auto validFrom = headers.find("Valid-From");
  long startVal = std::stol(validFrom->second);
  ILOG(Info, Support) << "MeanVertexCalib post-processing: startValidity = " << startVal << " X = " << x << " Y = " << y << " Z = " << z << ENDM;
  mX->Fill(x);
  mY->Fill(y);
  mZ->Fill(z);
  mStartValidity->Fill(startVal);
}

void MeanVertexPostProcessing::finalize(Trigger, framework::ServiceRegistryRef)
{
  // Only if you don't want it to be published after finalisation.
  getObjectsManager()->stopPublishing(mX);
  getObjectsManager()->stopPublishing(mY);
  getObjectsManager()->stopPublishing(mZ);
  getObjectsManager()->stopPublishing(mStartValidity);
}

} // namespace o2::quality_control_modules::glo
