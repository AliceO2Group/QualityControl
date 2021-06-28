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
/// \file   CalibrationTask.cxx
/// \author Milosz Filus
///

#include "TCanvas.h"
#include "TH1.h"
#include "TGraph.h"

#include "QualityControl/QcInfoLogger.h"
#include "FT0/CalibrationTask.h"
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include <Framework/InputRecord.h>
#include "DetectorsCalibration/Utils.h"
#include "CCDB/BasicCCDBManager.h"
#include "FITCalibration/FITCalibrationApi.h"

namespace o2::quality_control_modules::ft0
{

CalibrationTask::~CalibrationTask()
{
}

void CalibrationTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize CalibrationTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  mNotCalibratedChannelTimeHistogram = std::make_unique<TH1F>("Not_calibrated_time", "Not_calibrated_time", 2 * CHANNEL_TIME_HISTOGRAM_RANGE, -CHANNEL_TIME_HISTOGRAM_RANGE, CHANNEL_TIME_HISTOGRAM_RANGE);
  mCalibratedChannelTimeHistogram = std::make_unique<TH1F>("Calibrated_time", "Calibrated_time", 2 * CHANNEL_TIME_HISTOGRAM_RANGE, -CHANNEL_TIME_HISTOGRAM_RANGE, CHANNEL_TIME_HISTOGRAM_RANGE);
  mCalibratedTimePerChannelHistogram = std::make_unique<TH2F>("Calibrated_time_per_channel", "Calibrated_time_per_channel", o2::ft0::Nchannels_FT0, 0, o2::ft0::Nchannels_FT0, 2 * CHANNEL_TIME_HISTOGRAM_RANGE, -CHANNEL_TIME_HISTOGRAM_RANGE, CHANNEL_TIME_HISTOGRAM_RANGE);
  mNotCalibratedTimePerChannelHistogram = std::make_unique<TH2F>("Not_calibrated_time_per_channel", "Not_calibrated_time_per_channel", o2::ft0::Nchannels_FT0, 0, o2::ft0::Nchannels_FT0, 2 * CHANNEL_TIME_HISTOGRAM_RANGE, -CHANNEL_TIME_HISTOGRAM_RANGE, CHANNEL_TIME_HISTOGRAM_RANGE);
  mChannelTimeCalibrationObjectGraph = std::make_unique<TGraph>(o2::ft0::Nchannels_FT0);
  mChannelTimeCalibrationObjectGraph->SetName("Channel_time_calibration_object");
  mChannelTimeCalibrationObjectGraph->SetTitle("Channel_time_calibration_object");
  mChannelTimeCalibrationObjectGraph->SetMarkerStyle(20);
  mChannelTimeCalibrationObjectGraph->SetLineColor(kWhite);
  mChannelTimeCalibrationObjectGraph->SetFillColor(kBlack);

  getObjectsManager()->startPublishing(mNotCalibratedChannelTimeHistogram.get());
  getObjectsManager()->startPublishing(mCalibratedChannelTimeHistogram.get());
  getObjectsManager()->startPublishing(mCalibratedTimePerChannelHistogram.get());
  getObjectsManager()->startPublishing(mNotCalibratedTimePerChannelHistogram.get());
  getObjectsManager()->startPublishing(mChannelTimeCalibrationObjectGraph.get());
  ccdb::BasicCCDBManager::instance().setURL(mCustomParameters.at(CCDB_PARAM_KEY));
}

void CalibrationTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity" << activity.mId << ENDM;
  mNotCalibratedChannelTimeHistogram->Reset();
  mCalibratedChannelTimeHistogram->Reset();
  mCalibratedTimePerChannelHistogram->Reset();
  mNotCalibratedTimePerChannelHistogram->Reset();
}

void CalibrationTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
  mNotCalibratedChannelTimeHistogram->Reset();
  mCalibratedChannelTimeHistogram->Reset();
  mCalibratedTimePerChannelHistogram->Reset();
  mNotCalibratedTimePerChannelHistogram->Reset();
  mCurrentChannelTimeCalibrationObject = ccdb::BasicCCDBManager::instance().get<o2::ft0::FT0ChannelTimeCalibrationObject>(o2::fit::FITCalibrationApi::getObjectPath<o2::ft0::FT0ChannelTimeCalibrationObject>());
  for (std::size_t chID = 0; chID < o2::ft0::Nchannels_FT0; ++chID) {
    if (mCurrentChannelTimeCalibrationObject) {
      mChannelTimeCalibrationObjectGraph->SetPoint(chID, chID, mCurrentChannelTimeCalibrationObject->mTimeOffsets[chID]);
    } else {
      mChannelTimeCalibrationObjectGraph->SetPoint(chID, chID, 0);
    }
  }
}

void CalibrationTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  auto channels = ctx.inputs().get<gsl::span<o2::ft0::ChannelData>>("channels");

  for (auto& channel : channels) {
    if (mCurrentChannelTimeCalibrationObject) {
      mCalibratedChannelTimeHistogram->Fill(channel.CFDTime - mCurrentChannelTimeCalibrationObject->mTimeOffsets[channel.getChannelID()]);
      mCalibratedTimePerChannelHistogram->Fill(channel.getChannelID(), channel.CFDTime - mCurrentChannelTimeCalibrationObject->mTimeOffsets[channel.getChannelID()]);
    } else {
      mCalibratedChannelTimeHistogram->Fill(channel.CFDTime);
      mCalibratedTimePerChannelHistogram->Fill(channel.getChannelID(), channel.CFDTime);
    }
    mNotCalibratedChannelTimeHistogram->Fill(channel.CFDTime);
    mNotCalibratedTimePerChannelHistogram->Fill(channel.getChannelID(), channel.CFDTime);
  }
}

void CalibrationTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void CalibrationTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void CalibrationTask::reset()
{
  // clean all the monitor objects here
  mNotCalibratedChannelTimeHistogram->Reset();
  mCalibratedChannelTimeHistogram->Reset();
  mCalibratedTimePerChannelHistogram->Reset();
  mNotCalibratedTimePerChannelHistogram->Reset();
}

} // namespace o2::quality_control_modules::ft0
