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
#include "TPCBase/Painter.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/RawDigits.h"
#include "TPC/Utility.h"

namespace o2::quality_control_modules::tpc
{

RawDigits::RawDigits() : TaskInterface()
{
  mWrapperVector.emplace_back(&mRawDigitQC.getNClusters());
  mWrapperVector.emplace_back(&mRawDigitQC.getQMax());
  mWrapperVector.emplace_back(&mRawDigitQC.getTimeBin());
}

void RawDigits::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize TPC RawDigits QC task" << AliceO2::InfoLogger::InfoLogger::endm;

  mRawReader.createReader("");

  addAndPublish(getObjectsManager(), mNRawDigitsCanvasVec, { "c_Sides_N_RawDigits", "c_ROCs_N_RawDigits_1D", "c_ROCs_N_RawDigits_2D" });
  addAndPublish(getObjectsManager(), mQMaxCanvasVec, { "c_Sides_Q_Max", "c_ROCs_Q_Max_1D", "c_ROCs_Q_Max_2D" });
  addAndPublish(getObjectsManager(), mTimeBinCanvasVec, { "c_Sides_Time_Bin", "c_ROCs_Time_Bin_1D", "c_ROCs_Time_Bin_2D" });

  for (auto& wrapper : mWrapperVector) {
    getObjectsManager()->startPublishing(&wrapper);
    getObjectsManager()->addMetadata(wrapper.getObj()->getName().data(), "custom", "87");
  }

  mRawReader.setLinkZSCallback([this](int cru, int rowInSector, int padInRow, int timeBin, float adcValue) -> bool {
    mRawDigitQC.fillADCValue(cru, rowInSector, padInRow, timeBin, adcValue);
    return true;
  });
}

void RawDigits::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void RawDigits::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void RawDigits::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto& reader = mRawReader.getReaders()[0];
  o2::tpc::calib_processing_helper::processRawData(ctx.inputs(), reader, false);

  mRawDigitQC.analyse();

  auto vecPtrNRawDigits = toVector(mNRawDigitsCanvasVec);
  o2::tpc::painter::makeSummaryCanvases(mRawDigitQC.getNClusters(), 300, 0, 0, true, &vecPtrNRawDigits);

  auto vecPtrQMax = toVector(mQMaxCanvasVec);
  o2::tpc::painter::makeSummaryCanvases(mRawDigitQC.getQMax(), 300, 0, 0, true, &vecPtrQMax);

  auto vecPtrTimeBin = toVector(mTimeBinCanvasVec);
  o2::tpc::painter::makeSummaryCanvases(mRawDigitQC.getTimeBin(), 300, 0, 0, true, &vecPtrTimeBin);
}

void RawDigits::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void RawDigits::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void RawDigits::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
}

} // namespace o2::quality_control_modules::tpc
