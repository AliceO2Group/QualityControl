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
/// \file   BCTask.cxx
/// \author My Name
///

#include <TCanvas.h>
#include <TH1.h>

#include <boost/algorithm/string/case_conv.hpp>

#include "CommonConstants/LHCConstants.h"
#include "CommonConstants/Triggers.h"
#include "DataFormatsCTP/Configuration.h"
#include "DataFormatsCTP/Digits.h"
#include "DataFormatsEMCAL/TriggerRecord.h"

#include "QualityControl/QcInfoLogger.h"
#include "EMCAL/BCTask.h"
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>
#include <Framework/TimingInfo.h>

namespace o2::quality_control_modules::emcal
{

BCTask::~BCTask()
{
  delete mBCReadout;
  delete mBCEMCAny;
  delete mBCMinBias;
  delete mBCL0EMCAL;
  delete mBCL0DCAL;
}

void BCTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  constexpr unsigned int LHC_MAX_BC = o2::constants::lhc::LHCMaxBunches;
  mBCReadout = new TH1F("BCEMCALReadout", "BC distribution EMCAL readout", LHC_MAX_BC, -0.5, LHC_MAX_BC - 0.5);
  getObjectsManager()->startPublishing(mBCReadout);
  mBCEMCAny = new TH1F("BCCTPEMCALAny", "BC distribution CTP EMCAL any triggered", LHC_MAX_BC, -0.5, LHC_MAX_BC - 0.5);
  getObjectsManager()->startPublishing(mBCEMCAny);
  mBCMinBias = new TH1F("BCCTPEMCALMinBias", "BC distribution CTP EMCAL min. bias triggered", LHC_MAX_BC, -0.5, LHC_MAX_BC - 0.5);
  getObjectsManager()->startPublishing(mBCMinBias);
  mBCL0EMCAL = new TH1F("BCCTPEMCALL0", "BC distribution CTP EMCAL LO-triggered", LHC_MAX_BC, -0.5, LHC_MAX_BC - 0.5);
  getObjectsManager()->startPublishing(mBCL0EMCAL);
  mBCL0DCAL = new TH1F("BCCTPDCALL0", "BC distribution CTP DCAL L0-triggered", LHC_MAX_BC, -0.5, LHC_MAX_BC - 0.5);
  getObjectsManager()->startPublishing(mBCL0DCAL);
}

void BCTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  mCurrentRun = activity.mId;
  std::fill(mTriggerClassIndices.begin(), mTriggerClassIndices.end(), 0);
  reset();
}

void BCTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void BCTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  bool hasCTPConfig = true;
  if (!hasClassMasksLoaded()) {
    if (!loadTriggerClasses(ctx.services().get<o2::framework::TimingInfo>().creation)) {
      ILOG(Error, Support) << "No trigger classes loaded, processing CTP digits not possible" << ENDM;
      hasCTPConfig = false;
    }
  }
  auto emctriggers = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>("emcal-triggers");
  for (const auto& emctrg : emctriggers) {
    if (emctrg.getTriggerBits() & o2::trigger::PhT) {
      mBCReadout->Fill(emctrg.getBCData().bc);
    }
  }

  if (hasCTPConfig) {
    auto ctpdigits = ctx.inputs().get<gsl::span<o2::ctp::CTPDigit>>("ctp-digits");
    for (const auto& ctpdigit : ctpdigits) {
      auto bc = ctpdigit.intRecord.bc;
      uint64_t classMaskCTP = ctpdigit.CTPClassMask.to_ulong();
      bool emcalANY = false;
      if (classMaskCTP & mTriggerClassIndices[EMCMinBias]) {
        mBCMinBias->Fill(bc);
        emcalANY = true;
      }
      if (classMaskCTP & mTriggerClassIndices[EMCL0]) {
        mBCL0EMCAL->Fill(bc);
        emcalANY = true;
      }
      if (classMaskCTP & mTriggerClassIndices[DMCL0]) {
        mBCL0DCAL->Fill(bc);
        emcalANY = true;
      }
      if (emcalANY) {
        mBCEMCAny->Fill(bc);
      }
    }
  }
}

void BCTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void BCTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void BCTask::reset()
{
  // clean all the monitor objects here
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mBCReadout->Reset();
  mBCEMCAny->Reset();
  mBCMinBias->Reset();
  mBCL0EMCAL->Reset();
  mBCL0DCAL->Reset();
}

bool BCTask::hasClassMasksLoaded() const
{
  bool result = false;
  // require at least 1 trigger class to be different from 0 (usually at least the min. bias trigger)
  for (auto trg : mTriggerClassIndices) {
    if (trg > 0) {
      result = true;
      break;
    }
  }
  return result;
}

bool BCTask::loadTriggerClasses(uint64_t timestamp)
{
  auto tokenize = [](const std::string_view trgclass, char delimiter) -> std::vector<std::string> {
    std::vector<std::string> tokens;
    std::stringstream tokenizer(trgclass.data());
    std::string buf;
    while (std::getline(tokenizer, buf, delimiter)) {
      tokens.emplace_back(buf);
    }

    return tokens;
  };
  std::map<std::string, std::string> metadata;
  metadata["runNumber"] = std::to_string(mCurrentRun);
  auto ctpconfig = this->retrieveConditionAny<o2::ctp::CTPConfiguration>("CTP/Config/Config", metadata, timestamp);
  if (ctpconfig) {
    for (auto& cls : ctpconfig->getCTPClasses()) {
      auto trgclsname = boost::algorithm::to_upper_copy(cls.name);
      if (trgclsname.find("-EMC") == std::string::npos) {
        // Not an EMCAL trigger class
        continue;
      }
      auto tokens = tokenize(trgclsname, '-');
      if (tokens[1] != "B") {
        // Not bucket in both beams
        continue;
      }
      LOG(debug) << "Found trigger class: " << trgclsname;
      if (tokens[0] == "C0TVX") {
        mTriggerClassIndices[EMCMinBias] = cls.classMask;
        ILOG(Info, Support) << "Trigger class index EMC min bias: " << mTriggerClassIndices[EMCMinBias] << " (Class name: " << cls.name << ")" << ENDM;
      }
      if (tokens[0] == "CTVXEMC") {
        mTriggerClassIndices[EMCL0] = cls.classMask;
        ILOG(Info, Support) << "Trigger class index EMC L0:       " << mTriggerClassIndices[EMCL0] << " (Class name: " << cls.name << ")" << ENDM;
      }
      if (tokens[0] == "CTVXDMC") {
        mTriggerClassIndices[DMCL0] = cls.classMask;
        ILOG(Info, Support) << "Trigger class index DMC L0:       " << mTriggerClassIndices[DMCL0] << " (Class name: " << cls.name << ")" << ENDM;
      }
    }
    return true;
  } else {
    ILOG(Error, Support) << "Failed loading CTP configuration for run " << mCurrentRun << ", timestamp " << timestamp << ENDM;
    return false;
  }
}

} // namespace o2::quality_control_modules::emcal
