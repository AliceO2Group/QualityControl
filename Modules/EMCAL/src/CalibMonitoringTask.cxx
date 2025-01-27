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
#include "EMCALBase/Mapper.h"
#include "EMCALReconstruction/Channel.h"
#include "EMCALCalib/BadChannelMap.h"
#include "EMCALCalib/TimeCalibrationParams.h"
#include "EMCALCalib/FeeDCS.h"
// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "EMCAL/CalibMonitoringTask.h"
#include "EMCAL/DrawGridlines.h"

// root includes
#include "TCanvas.h"
#include "TPaveText.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TLine.h"
#include <boost/property_tree/ptree.hpp>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::emcal
{

int CalibMonitoringTask::GetTRUIndexFromSTUIndex(Int_t id, Int_t detector)
{
  Int_t kEMCAL = 0;
  Int_t kDCAL = 1;

  if ((id > 31 && detector == kEMCAL) || (id > 13 && detector == kDCAL) || id < 0) {
    return -1; // Error Condition
  }

  if (detector == kEMCAL) {
    return id;
  } else if (detector == kDCAL) {
    return 32 + ((int)(id / 4) * 6) + ((id % 4 < 2) ? (id % 4) : (id % 4 + 2));
  }
  return -1;
}

int CalibMonitoringTask::GetChannelForMaskRun2(int mask, int bitnumber, bool onethirdsm)
{
  if (onethirdsm)
    return mask * 16 + bitnumber;
  const int kChannelMap[6][16] = { { 8, 9, 10, 11, 20, 21, 22, 23, 32, 33, 34, 35, 44, 45, 46, 47 },     // Channels in mask0
                                   { 56, 57, 58, 59, 68, 69, 70, 71, 80, 81, 82, 83, 92, 93, 94, 95 },   // Channels in mask1
                                   { 4, 5, 6, 7, 16, 17, 18, 19, 28, 29, 30, 31, 40, 41, 42, 43 },       // Channels in mask2
                                   { 52, 53, 54, 55, 64, 65, 66, 67, 76, 77, 78, 79, 88, 89, 90, 91 },   // Channels in mask3
                                   { 0, 1, 2, 3, 12, 13, 14, 15, 24, 25, 26, 27, 36, 37, 38, 39 },       // Channels in mask4
                                   { 48, 49, 50, 51, 60, 61, 62, 63, 72, 73, 74, 75, 84, 85, 86, 87 } }; // Channels in mask5
  return kChannelMap[mask][bitnumber];
}

std::vector<int> CalibMonitoringTask::GetAbsFastORIndexFromMask()
{
  std::vector<int> maskedfastors;
  int itru = 0;
  for (Int_t i = 0; i < 46; i++) {
    int localtru = itru % 32, detector = itru >= 32 ? 1 : 0,
        globaltru = GetTRUIndexFromSTUIndex(localtru, detector);
    bool onethirdsm = ((globaltru >= 30 && globaltru < 32) || (globaltru >= 50 && globaltru < 52));
    for (int ipos = 0; ipos < 6; ipos++) {
      auto regmask = mFeeDCS->getTRUDCS(i).getMaskReg(ipos);
      std::bitset<16> bitsregmask(regmask);
      for (int ibit = 0; ibit < 16; ibit++) {
        if (bitsregmask.test(ibit)) {
          auto channel = GetChannelForMaskRun2(ipos, ibit, onethirdsm);
          int absfastor = mTriggerMapping->getAbsFastORIndexFromIndexInTRU(globaltru, channel);
          maskedfastors.push_back(absfastor);
        }
      }
    }
    itru++;
  }
  return maskedfastors;
}

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
      if (!mTimeCalibParamHisto) {
        mTimeCalibParamHisto = new TH1D("timeCalibCoeff", "Time Calib Coeff", 17644, -0.5, 17643.5); //
        mTimeCalibParamHisto->GetXaxis()->SetTitle("Cell Id");
        mTimeCalibParamHisto->GetYaxis()->SetTitle("Time (ns)");
        mTimeCalibParamHisto->SetStats(false);
      }
      getObjectsManager()->startPublishing(mTimeCalibParamHisto);

      if (!mTimeCalibParamPosition) {
        mTimeCalibParamPosition = new TH2D("timeCalibPosition", "Time Calib Coeff in 2D", 96, -0.5, 95.5, 208, -0.5, 207.5);
        mTimeCalibParamPosition->GetXaxis()->SetTitle("column (#eta)");
        mTimeCalibParamPosition->GetYaxis()->SetTitle("row (#phi)");
        mTimeCalibParamPosition->SetStats(false);
      }
      getObjectsManager()->startPublishing(mTimeCalibParamPosition);
    }
    if (obj == "BadChannelMap") {
      if (!mBadChannelMapHisto) {
        mBadChannelMapHisto = new TH2D("badChannelMap", "Pos. of Bad Channel", 96, -0.5, 95.5, 208, -0.5, 207.5);
        mBadChannelMapHisto->GetXaxis()->SetTitle("column (#eta)");
        mBadChannelMapHisto->GetYaxis()->SetTitle("row (#phi)");
        mBadChannelMapHisto->SetStats(false);
      }
      getObjectsManager()->startPublishing(mBadChannelMapHisto);

      // histogram for number of bad, dead, good channels in emcal only
      if (!mMaskStatsEMCALHisto) {
        mMaskStatsEMCALHisto = new TH1D("MaskStatsEMCALHisto", "Number of Good/Dead/Bad Channels in EMCAL Only", 3, -0.5, 2.5);
        mMaskStatsEMCALHisto->GetXaxis()->SetTitle("channel status");
        mMaskStatsEMCALHisto->GetYaxis()->SetTitle("Number of channels");
        mMaskStatsEMCALHisto->GetXaxis()->SetBinLabel(1, "Good cells");
        mMaskStatsEMCALHisto->GetXaxis()->SetBinLabel(2, "Bad cells");
        mMaskStatsEMCALHisto->GetXaxis()->SetBinLabel(3, "Dead cells");
        mMaskStatsEMCALHisto->SetStats(false);
      }
      getObjectsManager()->startPublishing(mMaskStatsEMCALHisto);

      // histogram for number of bad, dead, good channels in emcal only
      if (!mMaskStatsDCALHisto) {
        mMaskStatsDCALHisto = new TH1D("MaskStatsDCALHisto", "Number of Good/Dead/Bad Channels in DCAL Only", 3, -0.5, 2.5);
        mMaskStatsDCALHisto->GetXaxis()->SetTitle("channel status");
        mMaskStatsDCALHisto->GetYaxis()->SetTitle("Number of channels");
        mMaskStatsDCALHisto->GetXaxis()->SetBinLabel(1, "Good cells");
        mMaskStatsDCALHisto->GetXaxis()->SetBinLabel(2, "Bad cells");
        mMaskStatsDCALHisto->GetXaxis()->SetBinLabel(3, "Dead cells");
        mMaskStatsDCALHisto->SetStats(false);
      }
      getObjectsManager()->startPublishing(mMaskStatsDCALHisto);

      // histogram for number of bad, dead, good channels in all
      if (!mMaskStatsAllHisto) {
        mMaskStatsAllHisto = new TH1D("MaskStatsAllHisto", "Number of Good/Dead/Bad Channels in EMCAL + DCAL", 3, -0.5, 2.5);
        mMaskStatsAllHisto->GetXaxis()->SetTitle("channel status");
        mMaskStatsAllHisto->GetYaxis()->SetTitle("Number of channels");
        mMaskStatsAllHisto->GetXaxis()->SetBinLabel(1, "Good cells");
        mMaskStatsAllHisto->GetXaxis()->SetBinLabel(2, "Bad cells");
        mMaskStatsAllHisto->GetXaxis()->SetBinLabel(3, "Dead cells");
        mMaskStatsAllHisto->SetStats(false);
      }
      getObjectsManager()->startPublishing(mMaskStatsAllHisto);

      // histogram number of bad, good and dead channels per supermodule
      if (!mMaskStatsSupermoduleHisto) {
        mMaskStatsSupermoduleHisto = new TH2D("MaskStatsSupermoduleHisto", "Number of Good/Dead/Bad Channels per supermodule", 3, -0.5, 2.5, 20, -0.5, 19.5);
        mMaskStatsSupermoduleHisto->GetXaxis()->SetTitle("channel status");
        mMaskStatsSupermoduleHisto->GetYaxis()->SetTitle("Supermodule ID");
        mMaskStatsSupermoduleHisto->GetXaxis()->SetBinLabel(1, "Good cells");
        mMaskStatsSupermoduleHisto->GetXaxis()->SetBinLabel(2, "Bad cells");
        mMaskStatsSupermoduleHisto->GetXaxis()->SetBinLabel(3, "Dead cells");
        mMaskStatsSupermoduleHisto->SetStats(false);
      }
      getObjectsManager()->startPublishing(mMaskStatsSupermoduleHisto);

      if (!mNumberOfBadChannelsFEC) {
        mNumberOfBadChannelsFEC = new TH2D("NumberBadChannelsFEC", "Number of bad channels per FEC", 40, -0.5, 39.5, 20., -0.5, 19.5);
        mNumberOfBadChannelsFEC->GetXaxis()->SetTitle("FEC ID");
        mNumberOfBadChannelsFEC->GetYaxis()->SetTitle("Supermodule ID");
        mNumberOfBadChannelsFEC->SetStats(false);
      }
      getObjectsManager()->startPublishing(mNumberOfBadChannelsFEC);

      if (!mNumberOfDeadChannelsFEC) {
        mNumberOfDeadChannelsFEC = new TH2D("NumberDeadChannelsFEC", "Number of dead channels per FEC", 40, -0.5, 39.5, 20., -0.5, 19.5);
        mNumberOfDeadChannelsFEC->GetXaxis()->SetTitle("FEC ID");
        mNumberOfDeadChannelsFEC->GetYaxis()->SetTitle("Supermodule ID");
        mNumberOfDeadChannelsFEC->SetStats(false);
      }
      getObjectsManager()->startPublishing(mNumberOfDeadChannelsFEC);

      if (!mNumberOfNonGoodChannelsFEC) {
        mNumberOfNonGoodChannelsFEC = new TH2D("NumberNonGoodChannelsFEC", "Number of dead+bad channels per FEC", 40, -0.5, 39.5, 20., -0.5, 19.5);
        mNumberOfNonGoodChannelsFEC->GetXaxis()->SetTitle("FEC ID");
        mNumberOfNonGoodChannelsFEC->GetYaxis()->SetTitle("Supermodule ID");
        mNumberOfNonGoodChannelsFEC->SetStats(false);
      }
      getObjectsManager()->startPublishing(mNumberOfNonGoodChannelsFEC);
    }
    if (obj == "FeeDCS") {
      if (!mSRUFirmwareVersion) {
        mSRUFirmwareVersion = new TH1D("SRUFirmwareVersion", "SRUFirmwareVersion of Supermodule", 20., -0.5, 19.5);
        mSRUFirmwareVersion->GetXaxis()->SetTitle("Supermodule ID");
        mSRUFirmwareVersion->GetYaxis()->SetTitle("SRUFirmwareVersion");
        mSRUFirmwareVersion->SetStats(false);
      }
      getObjectsManager()->startPublishing(mSRUFirmwareVersion);

      if (!mActiveDDLs) {
        mActiveDDLs = new TH1D("ActiveDDLs", "Active Status of DDLs", 46., -0.5, 45.5);
        mActiveDDLs->GetXaxis()->SetTitle("DDL ID");
        mActiveDDLs->GetYaxis()->SetTitle("Active Status");
        mActiveDDLs->SetStats(false);
      }
      getObjectsManager()->startPublishing(mActiveDDLs);

      if (!mTRUThresholds) {
        mTRUThresholds = new TH1D("TRUThresholds", "L0 threshold of TRUs", 46., -0.5, 45.5);
        mTRUThresholds->GetXaxis()->SetTitle("TRU ID");
        mTRUThresholds->GetYaxis()->SetTitle("L0 threshold");
        mTRUThresholds->SetStats(false);
      }
      getObjectsManager()->startPublishing(mTRUThresholds);

      if (!mL0Algorithm) {
        mL0Algorithm = new TH1D("L0Algorithm", "L0 algorithm of TRUs", 46., -0.5, 45.5);
        mL0Algorithm->GetXaxis()->SetTitle("TRU ID");
        mL0Algorithm->GetYaxis()->SetTitle("L0 algorithm");
        mL0Algorithm->SetStats(false);
      }
      getObjectsManager()->startPublishing(mL0Algorithm);

      if (!mRollbackSTU) {
        mRollbackSTU = new TH1D("RollbackSTU", "Rollback Buffer of TRUs", 46., -0.5, 45.5);
        mRollbackSTU->GetXaxis()->SetTitle("TRU ID");
        mRollbackSTU->GetYaxis()->SetTitle("Rollback Buffer");
        mRollbackSTU->SetStats(false);
      }
      getObjectsManager()->startPublishing(mRollbackSTU);

      if (!mTRUMaskPositionHisto) {
        mTRUMaskPositionHisto = new TH2D("TRUMaskPositionHisto", "TRU Mask Position Histogram", 48., -0.5, 47.5, 104, -0.5, 103.5);
        mTRUMaskPositionHisto->GetXaxis()->SetTitle("FastOR Abs Eta");
        mTRUMaskPositionHisto->GetYaxis()->SetTitle("FastOR Abs Phi");
        mTRUMaskPositionHisto->SetStats(false);
      }
      getObjectsManager()->startPublishing(mTRUMaskPositionHisto);
    }
  }
  o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
  mMapper = std::make_unique<o2::emcal::MappingHandler>();
}

void CalibMonitoringTask::update(Trigger t, framework::ServiceRegistryRef)
{
  auto geo = o2::emcal::Geometry::GetInstance();
  std::map<std::string, std::string> metadata;
  reset();
  for (const auto& obj : mCalibObjects) {
    if (obj == "BadChannelMap") {
      mBadChannelMap = mCalibDB->readBadChannelMap(t.timestamp, metadata);
      if (!mBadChannelMap) {
        ILOG(Info, Support) << "No Bad Channel Map object " << ENDM;
        continue;
      }
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
        auto [supermodule, module, modphi, modeta] = geo->GetCellIndex(cellID);
        auto [ddl, onlinerow, onlinecol] = geo->getOnlineID(cellID);
        auto hwaddress = mMapper->getMappingForDDL(ddl).getHardwareAddress(onlinerow, onlinecol, o2::emcal::ChannelType_t::HIGH_GAIN);
        auto fecID = mMapper->getFEEForChannelInDDL(ddl, o2::emcal::Channel::getFecIndexFromHwAddress(hwaddress), o2::emcal::Channel::getBranchIndexFromHwAddress(hwaddress));
        int statusbin = 0;
        if (cellStatus == o2::emcal::BadChannelMap::MaskType_t::BAD_CELL) {
          statusbin = 1;
          mNumberOfBadChannelsFEC->Fill(fecID, supermodule);
          mNumberOfNonGoodChannelsFEC->Fill(fecID, supermodule);
        } else if (cellStatus == o2::emcal::BadChannelMap::MaskType_t::DEAD_CELL) {
          statusbin = 2;
          mNumberOfDeadChannelsFEC->Fill(fecID, supermodule);
          mNumberOfNonGoodChannelsFEC->Fill(fecID, supermodule);
        }
        mMaskStatsAllHisto->Fill(statusbin);
        mMaskStatsSupermoduleHisto->Fill(statusbin, supermodule);
        if (cellID < minCellDCAL) {
          mMaskStatsEMCALHisto->Fill(statusbin);
        } else {
          mMaskStatsDCALHisto->Fill(statusbin);
        }
      }
    }

    if (obj == "TimeCalibParams") {
      mTimeCalib = mCalibDB->readTimeCalibParam(t.timestamp, metadata);
      if (!mTimeCalib) {
        ILOG(Info, Support) << " No Time Calib object " << ENDM;
        continue;
      }
      TH1* hist_temp = 0x0;
      hist_temp = mTimeCalib->getHistogramRepresentation(false); // we monitor for the moment only the high gain
      for (Int_t i = 0; i < hist_temp->GetNbinsX(); i++) {
        mTimeCalibParamHisto->SetBinContent(i, hist_temp->GetBinContent(i + 1));
        auto [row, column] = geo->GlobalRowColFromIndex(i);
        mTimeCalibParamPosition->SetBinContent(column + 1, row + 1, hist_temp->GetBinContent(i + 1));
      }
    }
    if (obj == "FeeDCS") {
      mFeeDCS = mCalibDB->readFeeDCSData(t.timestamp, metadata);
      if (!mFeeDCS) {
        ILOG(Info, Support) << " No FEE DCS object " << ENDM;
        continue;
      }
      for (Int_t i = 0; i < mSRUFirmwareVersion->GetNbinsX(); i++) {
        mSRUFirmwareVersion->SetBinContent(i + 1, mFeeDCS->getSRUFWversion(i));
      }
      for (Int_t i = 0; i < mActiveDDLs->GetNbinsX(); i++) {
        mActiveDDLs->SetBinContent(i + 1, mFeeDCS->isDDLactive(i));
      }
      for (Int_t i = 0; i < mTRUThresholds->GetNbinsX(); i++) {
        mTRUThresholds->SetBinContent(i + 1, static_cast<double>(mFeeDCS->getTRUDCS(i).getGTHRL0()));
      }
      for (Int_t i = 0; i < mL0Algorithm->GetNbinsX(); i++) {
        mL0Algorithm->SetBinContent(i + 1, mFeeDCS->getTRUDCS(i).getL0SEL());
      }

      for (Int_t i = 0; i < mRollbackSTU->GetNbinsX(); i++) {
        mRollbackSTU->SetBinContent(i + 1, mFeeDCS->getTRUDCS(i).getRLBKSTU());
      }

      auto fastORs = GetAbsFastORIndexFromMask();

      for (auto fastORID : fastORs) {
        auto [eta, phi] = mTriggerMapping->getPositionInEMCALFromAbsFastORIndex(fastORID);
        mTRUMaskPositionHisto->SetBinContent(eta + 1, phi + 1, 1.);
      }

      o2::quality_control_modules::emcal::DrawGridlines::DrawFastORGrid(mTRUMaskPositionHisto);
      o2::quality_control_modules::emcal::DrawGridlines::DrawTRUGrid(mTRUMaskPositionHisto);
      o2::quality_control_modules::emcal::DrawGridlines::DrawSMGridInTriggerGeo(mTRUMaskPositionHisto);
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
      getObjectsManager()->stopPublishing(mMaskStatsSupermoduleHisto);
      getObjectsManager()->stopPublishing(mNumberOfBadChannelsFEC);
      getObjectsManager()->stopPublishing(mNumberOfDeadChannelsFEC);
      getObjectsManager()->stopPublishing(mNumberOfNonGoodChannelsFEC);
    }
    if (obj == "TimeCalibParams") {
      getObjectsManager()->stopPublishing(mTimeCalibParamHisto);
      getObjectsManager()->stopPublishing(mTimeCalibParamPosition);
    }
    if (obj == "FeeDCS") {
      getObjectsManager()->stopPublishing(mSRUFirmwareVersion);
      getObjectsManager()->stopPublishing(mActiveDDLs);
      getObjectsManager()->stopPublishing(mTRUThresholds);
      getObjectsManager()->stopPublishing(mL0Algorithm);
      getObjectsManager()->stopPublishing(mRollbackSTU);
      getObjectsManager()->stopPublishing(mTRUMaskPositionHisto);
    }
  }
}
void CalibMonitoringTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Support) << "Resetting the histogram" << ENDM;
  if (mBadChannelMapHisto) {
    mBadChannelMapHisto->Reset();
  }
  if (mMaskStatsEMCALHisto) {
    mMaskStatsEMCALHisto->Reset();
  }
  if (mMaskStatsDCALHisto) {
    mMaskStatsDCALHisto->Reset();
  }
  if (mMaskStatsAllHisto) {
    mMaskStatsAllHisto->Reset();
  }
  if (mMaskStatsSupermoduleHisto) {
    mMaskStatsSupermoduleHisto->Reset();
  }
  if (mNumberOfBadChannelsFEC) {
    mNumberOfBadChannelsFEC->Reset();
  }
  if (mNumberOfDeadChannelsFEC) {
    mNumberOfDeadChannelsFEC->Reset();
  }
  if (mNumberOfNonGoodChannelsFEC) {
    mNumberOfNonGoodChannelsFEC->Reset();
  }
  if (mTimeCalibParamHisto) {
    mTimeCalibParamHisto->Reset();
  }
  if (mSRUFirmwareVersion) {
    mSRUFirmwareVersion->Reset();
  }
  if (mActiveDDLs) {
    mActiveDDLs->Reset();
  }
  if (mTRUThresholds) {
    mTRUThresholds->Reset();
  }
  if (mL0Algorithm) {
    mL0Algorithm->Reset();
  }
  if (mRollbackSTU) {
    mRollbackSTU->Reset();
  }
  if (mTRUMaskPositionHisto) {
    mTRUMaskPositionHisto->Reset();
  }
}
} // namespace o2::quality_control_modules::emcal
