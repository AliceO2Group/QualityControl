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
      mBadChannelMapEMCALHisto = new TH2D("badChannelMapEMCAL", "Number of Good/Dead/Bad Channels in EMCAL Only", 3, 0, 2, 12288, 0, 12288);
      mBadChannelMapEMCALHisto->GetXaxis()->SetTitle("channel status");
      mBadChannelMapEMCALHisto->GetYaxis()->SetTitle("Number of channels");
      mBadChannelMapEMCALHisto->SetStats(false);
      getObjectsManager()->startPublishing(mBadChannelMapEMCALHisto);

      // histogram for number of bad, dead, good channels in emcal only
      mBadChannelMapDCALHisto = new TH2D("badChannelMapDCAL", "Number of Good/Dead/Bad Channels in DCAL Only", 3, 0, 2, 5376, 0, 5376);
      mBadChannelMapDCALHisto->GetXaxis()->SetTitle("channel status");
      mBadChannelMapDCALHisto->GetYaxis()->SetTitle("Number of channels");
      mBadChannelMapDCALHisto->SetStats(false);
      getObjectsManager()->startPublishing(mBadChannelMapDCALHisto);

 
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
          // check to see if the cell is in the emcal or dcal and fill appropriate hist
          int cellID = geo->GetAbsCellIdFromCellIndexes(i, j);
          if (cellID < 12288) {
            mBadChannelMapEMCALHisto->Fill(hist_temp2->GetBinContent(i + 1, j + 1));
          }
          else{
            mBadChannelMapDCALHisto->Fill(hist_temp2->GetBinContent(i + 1, j + 1));
          }
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
      getObjectsManager()->stopPublishing(mBadChannelMapEMCALHisto);
      getObjectsManager()->stopPublishing(mBadChannelMapDCALHisto);
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
  if (mBadChannelMapEMCALHisto)
    mBadChannelMapEMCALHisto->Reset();
  if (mBadChannelMapDCALHisto)
    mBadChannelMapDCALHisto->Reset();
  if (mTimeCalibParamHisto)
    mTimeCalibParamHisto->Reset();
}
} // namespace o2::quality_control_modules::emcal
