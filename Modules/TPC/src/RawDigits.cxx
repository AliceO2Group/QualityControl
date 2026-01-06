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
/// \file   RawDigits.cxx
/// \author Thomas Klemenz
///

// O2 includes
#include "Framework/ProcessingContext.h"
#include "DataFormatsTPC/ClusterNative.h"
#if __has_include("TPCBase/Painter.h")
#include "TPCBase/Painter.h"
#else
#include "TPCBaseRecSim/Painter.h"
#endif

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/RawDigits.h"
#include "TPC/Utility.h"

namespace o2::quality_control_modules::tpc
{

RawDigits::RawDigits() : TaskInterface()
{
}

void RawDigits::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TPC RawDigits QC task" << ENDM;

  mRawDigitQC.setName("RawDigitData");

  const auto last = mCustomParameters.end();
  const auto itMergeable = mCustomParameters.find("mergeableOutput");
  std::string mergeable;

  if (itMergeable == last) {
    LOGP(warning, "missing parameter 'mergeableOutput'");
    LOGP(warning, "Please add 'mergeableOutput': '<value>' to the 'taskParameters'.");
  } else {
    mergeable = itMergeable->second;
  }

  if (mergeable == "true") {
    mIsMergeable = true;
    ILOG(Info, Support) << "Using mergeable output for RawDigits Task." << ENDM;
  } else if (mergeable == "false") {
    mIsMergeable = false;
    ILOG(Info, Support) << "Using non-mergeable output for RawDigits Task." << ENDM;
  } else {
    mIsMergeable = false;
    LOGP(warning, "No valid value for 'mergeableOutput'. Set it as 'true' or 'false'. Falling back to non-mergeable output.");
  }

  mRawReader.createReader("");

  if (mIsMergeable) {
    getObjectsManager()->startPublishing(&mRawDigitQC);
  } else {
    mWrapperVector.emplace_back(&mRawDigitQC.getClusters().getNClusters());
    mWrapperVector.emplace_back(&mRawDigitQC.getClusters().getQMax());
    mWrapperVector.emplace_back(&mRawDigitQC.getClusters().getTimeBin());

    addAndPublish(getObjectsManager(), mNRawDigitsCanvasVec, { "c_Sides_N_RawDigits", "c_ROCs_N_RawDigits_1D", "c_ROCs_N_RawDigits_2D" });
    addAndPublish(getObjectsManager(), mQMaxCanvasVec, { "c_Sides_Q_Max", "c_ROCs_Q_Max_1D", "c_ROCs_Q_Max_2D" });
    addAndPublish(getObjectsManager(), mTimeBinCanvasVec, { "c_Sides_Time_Bin", "c_ROCs_Time_Bin_1D", "c_ROCs_Time_Bin_2D" });

    for (auto& wrapper : mWrapperVector) {
      getObjectsManager()->startPublishing<true>(&wrapper);
    }
  }

  mRawReader.setLinkZSCallback([this](int cru, int rowInSector, int padInRow, int timeBin, float adcValue) -> bool {
    mRawDigitQC.getClusters().fillADCValue(cru, rowInSector, padInRow, timeBin, adcValue);
    return true;
  });
}

void RawDigits::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;

  mRawDigitQC.getClusters().reset();

  if (!mIsMergeable) {
    clearCanvases(mNRawDigitsCanvasVec);
    clearCanvases(mQMaxCanvasVec);
    clearCanvases(mTimeBinCanvasVec);
  }
}

void RawDigits::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void RawDigits::monitorData(o2::framework::ProcessingContext& ctx)
{
  mRawDigitQC.getClusters().denormalize();

  auto& reader = mRawReader.getReaders()[0];
  o2::tpc::calib_processing_helper::processRawData(ctx.inputs(), reader, false);

  if (!mIsMergeable) {
    mRawDigitQC.getClusters().normalize();

    fillCanvases(mRawDigitQC.getClusters().getNClusters(), mNRawDigitsCanvasVec, mCustomParameters, "NRawDigits");
    fillCanvases(mRawDigitQC.getClusters().getQMax(), mQMaxCanvasVec, mCustomParameters, "Qmax");
    fillCanvases(mRawDigitQC.getClusters().getTimeBin(), mTimeBinCanvasVec, mCustomParameters, "TimeBin");
  }
}

void RawDigits::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;

  if (mIsMergeable) {
    mRawDigitQC.getClusters().normalize();
  }
}

void RawDigits::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void RawDigits::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the data" << ENDM;

  mRawDigitQC.getClusters().reset();

  if (!mIsMergeable) {
    clearCanvases(mNRawDigitsCanvasVec);
    clearCanvases(mQMaxCanvasVec);
    clearCanvases(mTimeBinCanvasVec);
  }
}

} // namespace o2::quality_control_modules::tpc
