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
/// \file
/// \author Cristina Terrevoli, Markus Fasel
///

#include "CCDB/CCDBTimeStampUtils.h"
#include "EMCALBase/Geometry.h"
#include "EMCALCalib/BadChannelMap.h"
#include "EMCALCalib/TimeCalibrationParams.h"
// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "EMCAL/CalibMonitoringTask.h"

// root includes
#include "TCanvas.h"
#include "TPaveText.h"
#include "TH1D.h"
#include "TH2D.h"

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::emcal
{

void CalibMonitoringTask::configure(const boost::property_tree::ptree& config)
{
  mCalibDB = std::make_unique<o2::emcal::CalibDB>(config.get<std::string>("qc.config.conditionDB.url").data());

  for (const auto& output : config.get_child("qc.postprocessing." + getID() + ".calibObjects")) {
    mCalibObjects.emplace_back(output.second.data());
  }
}

void CalibMonitoringTask::initialize(Trigger, framework::ServiceRegistryRef)
{
  QcInfoLogger::setDetector("EMC");
  ILOG(Debug, Devel) << "initialize CalibTask" << ENDM;
  // initialize histograms to be monitored as data member
  for (const auto& obj : mCalibObjects) {
    if (obj == "TimeCalibParams") {
      mTimeCalibParamHisto = new TH1D("timeCalibCoeff", "Time Calib Coeff", 17644, -0.5, 17643.5); //
      mTimeCalibParamHisto->GetXaxis()->SetTitle("Cell Id");
      mTimeCalibParamHisto->GetYaxis()->SetTitle("Time (ns)");
      mTimeCalibParamHisto->SetStats(false);
      getObjectsManager()->startPublishing(mTimeCalibParamHisto);

      mTimeCalibParamPosition = new TH2D("timeCalibPosition", "Time Calib Coeff in 2D", 96, -0.5, 95.5, 208, -0.5, 207.5);
      mTimeCalibParamPosition->GetXaxis()->SetTitle("column (#eta)");
      mTimeCalibParamPosition->GetYaxis()->SetTitle("row (#phi)");
      mTimeCalibParamPosition->SetStats(false);
      getObjectsManager()->startPublishing(mTimeCalibParamPosition);
    }
    if (obj == "BadChannelMap") {
      mBadChannelMapHisto = new TH2D("badChannelMap", "Pos. of Bad Channel", 96, -0.5, 95.5, 208, -0.5, 207.5);
      mBadChannelMapHisto->GetXaxis()->SetTitle("column (#eta)");
      mBadChannelMapHisto->GetYaxis()->SetTitle("row (#phi)");
      mBadChannelMapHisto->SetStats(false);
      getObjectsManager()->startPublishing(mBadChannelMapHisto);

      // histogram for number of bad, dead, good channels in emcal only
      mMaskStatsEMCALHisto = new TH1D("MaskStatsEMCALHisto", "Number of Good/Dead/Bad Channels in EMCAL Only",3, -0.5, 2.5);
      mMaskStatsEMCALHisto->GetXaxis()->SetTitle("channel status");
      mMaskStatsEMCALHisto->GetYaxis()->SetTitle("Number of channels");
      mMaskStatsEMCALHisto->SetStats(false);
      getObjectsManager()->startPublishing(mMaskStatsEMCALHisto);

      // histogram for number of bad, dead, good channels in emcal only
      mMaskStatsDCALHisto = new TH1D("MaskStatsDCALHisto", "Number of Good/Dead/Bad Channels in DCAL Only", 3, -0.5, 2.5);
      mMaskStatsDCALHisto->GetXaxis()->SetTitle("channel status");
      mMaskStatsDCALHisto->GetYaxis()->SetTitle("Number of channels");
      mMaskStatsDCALHisto->SetStats(false);
      getObjectsManager()->startPublishing(mMaskStatsDCALHisto);

      // histogram for number of bad, dead, good channels in all
      mMaskStatsAllHisto = new TH1D("MaskStatsAllHisto", "Number of Good/Dead/Bad Channels in EMCAL + DCAL", 3, -0.5, 2.5);
      mMaskStatsAllHisto->GetXaxis()->SetTitle("channel status");
      mMaskStatsAllHisto->GetYaxis()->SetTitle("Number of channels");
      mMaskStatsAllHisto->SetStats(false);
      getObjectsManager()->startPublishing(mMaskStatsAllHisto);
    }
  }
  o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
}

void CalibMonitoringTask::update(Trigger t, framework::ServiceRegistryRef)
{
  auto geo = o2::emcal::Geometry::GetInstance();
  std::map<std::string, std::string> metadata;
  reset();
  for (const auto& obj : mCalibObjects) {
    if (obj == "BadChannelMap") {
      mBadChannelMap = mCalibDB->readBadChannelMap(o2::ccdb::getCurrentTimestamp(), metadata);
      if (!mBadChannelMap)
        ILOG(Info, Support) << "No Bad Channel Map object " << ENDM;
      TH2* hist_temp2 = 0x0;
      hist_temp2 = mBadChannelMap->getHistogramRepresentation();
      for (Int_t i = 0; i < hist_temp2->GetNbinsX(); i++) {
        for (Int_t j = 0; j < hist_temp2->GetNbinsY(); j++) {
          mBadChannelMapHisto->SetBinContent(i + 1, j + 1, hist_temp2->GetBinContent(i + 1, j + 1));
        }
      }

      // loop over all cells and check channel status
      int minCellDCAL = 12288;
      for (int cellID = 0; cellID < 17664; cellID++) {
        auto cellStatus = mBadChannelMap->getChannelStatus(cellID);
        int statusbin = 0;
        if (cellStatus == o2::emcal::ChannelStatus_t::BAD) {
          statusbin = 1;
        } else if (cellStatus == o2::emcal::ChannelStatus_t::DEAD) {
           statusbin = 2;
        }
        mMaskStatsAllHisto->Fill(statusbin);
        if(cellID < minCellDCAL) {
          mMaskStatsEMCALHisto->Fill(statusbin);
        } else {
          mMaskStatsDCALHisto->Fill(statusbin);
        }
      }
    }

    if (obj == "TimeCalibParams") {
      mTimeCalib = mCalibDB->readTimeCalibParam(o2::ccdb::getCurrentTimestamp(), metadata);
      if (!mTimeCalib)
        ILOG(Info, Support) << " No Time Calib object " << ENDM;
      TH1* hist_temp = 0x0;
      hist_temp = mTimeCalib->getHistogramRepresentation(false); // we monitor for the moment only the high gain
      for (Int_t i = 0; i < hist_temp->GetNbinsX(); i++) {
        mTimeCalibParamHisto->SetBinContent(i, hist_temp->GetBinContent(i + 1));
        auto [row, column] = geo->GlobalRowColFromIndex(i);
        mTimeCalibParamPosition->SetBinContent(column + 1, row + 1, hist_temp->GetBinContent(i + 1));
      }
    }
  }
}

void CalibMonitoringTask::finalize(Trigger t, framework::ServiceRegistryRef)
{
  for (const auto& obj : mCalibObjects) {
    if (obj == "BadChannelMap") {
      getObjectsManager()->stopPublishing(mBadChannelMapHisto);
      getObjectsManager()->stopPublishing(mMaskStatsEMCALHisto);
      getObjectsManager()->stopPublishing(mMaskStatsDCALHisto);
      getObjectsManager()->stopPublishing(mMaskStatsAllHisto);
    }
    if (obj == "TimeCalibParams") {
      getObjectsManager()->stopPublishing(mTimeCalibParamHisto);
      getObjectsManager()->stopPublishing(mTimeCalibParamPosition);
    }
  }
}
void CalibMonitoringTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Support) << "Resetting the histogram" << ENDM;
  if (mBadChannelMapHisto)
    mBadChannelMapHisto->Reset();
  if (mMaskStatsEMCALHisto)
    mMaskStatsEMCALHisto->Reset();
  if (mMaskStatsDCALHisto)
    mMaskStatsDCALHisto->Reset();
  if (mMaskStatsAllHisto)
    mMaskStatsAllHisto->Reset();
  if (mTimeCalibParamHisto)
    mTimeCalibParamHisto->Reset();
}
} // namespace o2::quality_control_modules::emcal
