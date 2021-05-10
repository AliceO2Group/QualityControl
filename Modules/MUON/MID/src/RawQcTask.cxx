// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   RawQcTask.cxx
/// \author Bogdan Vulpescu
/// \author Xavier Lopez
/// \author Diego Stocco
/// \author Guillaume Taillepied
///

#include <TCanvas.h>
#include <TH1.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "QualityControl/QcInfoLogger.h"
#include "MID/RawQcTask.h"
#include <Framework/InputRecord.h>

#include "Framework/DataRefUtils.h"
#include "DataFormatsMID/ColumnData.h"
#include "DataFormatsMID/ROBoard.h"
#include "DataFormatsMID/ROFRecord.h"
#include "DPLUtils/DPLRawParser.h"
#include "MID/RawDataChecker.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/Decoder.h"
#include "MIDRaw/ElectronicsDelay.h"
#include "MIDRaw/FEEIdConfig.h"

namespace o2::quality_control_modules::mid
{

RawQcTask::~RawQcTask()
{
  if (mRawDataChecker)
    delete mRawDataChecker;
}

void RawQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize RawQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // Retrieve task parameters from the config file
  if (auto param = mCustomParameters.find("feeId-config-file"); param != mCustomParameters.end()) {
    ILOG(Info) << "Custom parameter - FEE Id config file: " << param->second << ENDM;
    mFeeIdConfigFilename = param->second;
  }

  if (auto param = mCustomParameters.find("crate-masks-file"); param != mCustomParameters.end()) {
    ILOG(Info) << "Custom parameter - Crate masks file: " << param->second << ENDM;
    mCrateMasksFilename = param->second;
  }

  if (auto param = mCustomParameters.find("electronics-delays-file"); param != mCustomParameters.end()) {
    ILOG(Info) << "Custom parameter - Electronics delays file: " << param->second << ENDM;
    mElectronicsDelaysFilename = param->second;
  }

  if (auto param = mCustomParameters.find("per-gbt"); param != mCustomParameters.end()) {
    ILOG(Info) << "Custom parameter - Reading per GBT link: " << param->second << ENDM;
    std::istringstream(param->second) >> std::boolalpha >> mPerGBT;
  }

  if (auto param = mCustomParameters.find("per-feeId"); param != mCustomParameters.end()) {
    ILOG(Info) << "Custom parameter - Reading per FEE Id: " << param->second << ENDM;
    std::istringstream(param->second) >> std::boolalpha >> mPerFeeId;
  }

  if (auto param = mCustomParameters.find("output-file"); param != mCustomParameters.end()) {
    ILOG(Info) << "Custom parameter - Output file: " << param->second << ENDM;
    mOutFilename = param->second;
  }

  // Decoder configuration inputs
  if (!mFeeIdConfigFilename.empty()) {
    mFeeIdConfig = o2::mid::FEEIdConfig(mFeeIdConfigFilename.c_str());
  }

  if (!mElectronicsDelaysFilename.empty()) {
    mElectronicsDelay = o2::mid::readElectronicsDelay(mElectronicsDelaysFilename.c_str());
  }

  if (!mCrateMasksFilename.empty()) {
    mCrateMasks = o2::mid::CrateMasks(mCrateMasksFilename.c_str());
  }
  mChecker.init(mCrateMasks);

  // Histograms to be published
  mRawDataChecker = new TH1F("mRawDataChecker", "Raw Data Checker", 2, 0, 2);
  mRawDataChecker->GetXaxis()->SetBinLabel(1, "Processed");
  mRawDataChecker->GetXaxis()->SetBinLabel(2, "Faulty");

  getObjectsManager()->startPublishing(mRawDataChecker);
}

void RawQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
  mDetElemID->Reset();
}

void RawQcTask::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void RawQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  ILOG(Info) << "startOfDataMonitoring" << ENDM;

  /* // FIXME
  // Old version if called with runMID.cxx through o2-qc-run-mid
  // To be included back

  auto digits = ctx.inputs().get("digits");

  // Get strip patterns and loop over them
  gsl::span<const o2::mid::ColumnData> patterns = o2::framework::DataRefUtils::as<const o2::mid::ColumnData>(msg);

  // Loop on stripPatterns
  for (auto& col : patterns) {
    int deIndex = col.deId;
    mDetElemID->Fill(deIndex);
  }
  */

  o2::framework::DPLRawParser parser(ctx.inputs());

  std::vector<o2::mid::ROFRecord> dummy;

  if (!mDecoder) {
    auto const* rdhPtr = reinterpret_cast<const o2::header::RDHAny*>(parser.begin().raw());
    mDecoder = createDecoder(*rdhPtr, true, mElectronicsDelay, mCrateMasks, mFeeIdConfig);
    ILOG(Info) << "Created decoder" << ENDM;
  }

  mDecoder->clear();

  int count = 0;
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    auto const* rdhPtr = reinterpret_cast<const o2::header::RDHAny*>(it.raw());
    gsl::span<const uint8_t> payload(it.data(), it.size());
    mDecoder->process(payload, *rdhPtr);
    ++count;
  }

  mChecker.clear();
  if (!mChecker.process(mDecoder->getData(), mDecoder->getROFRecords(), dummy)) {
    //ILOG(Info) << mChecker.getDebugMessage() << ENDM;
    mRawDataChecker->Fill("Faulty", mChecker.getNEventsFaulty());
  }

  ILOG(Info) << "Number of busy raised: " << mChecker.getNBusyRaised() << ENDM;
  ILOG(Info) << "Fraction of faulty events: " << mChecker.getNEventsFaulty() << " / " << mChecker.getNEventsProcessed() << ENDM;
  ILOG(Info) << "Counts: " << count << ENDM;

  mRawDataChecker->Fill("Processed", mChecker.getNEventsProcessed());
}

void RawQcTask::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
}

void RawQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void RawQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info) << "Resetting the histogram" << ENDM;
  mRawDataChecker->Reset();
}

} // namespace o2::quality_control_modules::mid
