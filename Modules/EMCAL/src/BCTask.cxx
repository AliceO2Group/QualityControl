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

#include "CCDB/BasicCCDBManager.h"
#include "CommonConstants/LHCConstants.h"
#include "CommonConstants/Triggers.h"
#include "DataFormatsCTP/Configuration.h"
#include "DataFormatsCTP/Digits.h"
#include "DataFormatsEMCAL/Constants.h"
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
  delete mBCIncomplete;
  delete mBCEMCAny;
  delete mBCMinBias;
  delete mBCL0EMCAL;
  delete mBCL0DCAL;
}

void BCTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  parseTriggerSelection();
  constexpr unsigned int LHC_MAX_BC = o2::constants::lhc::LHCMaxBunches;
  mBCReadout = new TH1F("BCEMCALReadout", "BC distribution EMCAL readout", LHC_MAX_BC, -0.5, LHC_MAX_BC - 0.5);
  getObjectsManager()->startPublishing(mBCReadout);
  mBCIncomplete = new TH1F("BCIncomplete", "BC distribution EMCAL incomplete triggers", LHC_MAX_BC, -0.5, LHC_MAX_BC - 0.5);
  getObjectsManager()->startPublishing(mBCIncomplete);
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
  auto ctpconfig = ctx.inputs().get<o2::ctp::CTPConfiguration*>("ctp-config");
  auto emctriggers = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>("emcal-triggers");
  for (const auto& emctrg : emctriggers) {
    if (emctrg.getTriggerBits() & o2::trigger::PhT) {
      mBCReadout->Fill(emctrg.getBCData().bc);
      if (emctrg.getTriggerBits() & o2::emcal::triggerbits::Inc) {
        mBCIncomplete->Fill(emctrg.getBCData().bc);
      }
    }
  }

  if (hasClassMasksLoaded()) {
    auto ctpdigits = ctx.inputs().get<gsl::span<o2::ctp::CTPDigit>>("ctp-digits");
    for (const auto& ctpdigit : ctpdigits) {
      auto bc = ctpdigit.intRecord.bc;
      uint64_t classMaskCTP = ctpdigit.CTPClassMask.to_ulong();
      bool emcalANY = false;
      for (auto clsMaskEMC : mAllEMCALClasses) {
        if (classMaskCTP & clsMaskEMC) {
          emcalANY = true;
          break;
        }
      }
      if (emcalANY) {
        mBCEMCAny->Fill(bc);
      }
      // Distinguish between a couple of triggers
      if (classMaskCTP & mTriggerClassIndices[EMCMinBias]) {
        mBCMinBias->Fill(bc);
      }
      if (classMaskCTP & mTriggerClassIndices[EMCL0]) {
        mBCL0EMCAL->Fill(bc);
      }
      if (classMaskCTP & mTriggerClassIndices[DMCL0]) {
        mBCL0DCAL->Fill(bc);
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

void BCTask::finaliseCCDB(o2::framework::ConcreteDataMatcher& matcher, void* obj)
{
  if (matcher == o2::framework::ConcreteDataMatcher("CTP", "CONFIG", 0)) {
    auto triggerconfig = reinterpret_cast<const o2::ctp::CTPConfiguration*>(obj);
    if (triggerconfig) {
      ILOG(Info, Support) << "Loading EMCAL trigger classes for new trigger configuration: " << ENDM;
      loadTriggerClasses(triggerconfig);
    }
  }
}

void BCTask::reset()
{
  // clean all the monitor objects here
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mBCReadout->Reset();
  mBCIncomplete->Reset();
  mBCEMCAny->Reset();
  mBCMinBias->Reset();
  mBCL0EMCAL->Reset();
  mBCL0DCAL->Reset();
}

void BCTask::parseTriggerSelection()
{
  // Default trigger alias
  if (auto param = mCustomParameters.find("AliasMB"); param != mCustomParameters.end()) {
    mTriggerAliases["MB"] = param->second;
  } else {
    mTriggerAliases["MB"] = "C0TVX";
  }
  if (auto param = mCustomParameters.find("Alias0EMC"); param != mCustomParameters.end()) {
    mTriggerAliases["0EMC"] = param->second;
  } else {
    mTriggerAliases["0EMC"] = "CTVXEMC";
  }
  if (auto param = mCustomParameters.find("Alias0DMC"); param != mCustomParameters.end()) {
    mTriggerAliases["0DMC"] = param->second;
  } else {
    mTriggerAliases["0DMC"] = "CTVXDMC";
  }

  if (auto param = mCustomParameters.find("BeamMode"); param != mCustomParameters.end()) {
    mBeamMode = getBeamPresenceMode(param->second);
  }
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

std::string BCTask::getBeamPresenceModeToken(BeamPresenceMode_t beammode) const
{
  std::string token;
  switch (beammode) {
    case BeamPresenceMode_t::BOTH:
      token = "B";
      break;
    case BeamPresenceMode_t::ASIDE:
      token = "A";
      break;
    case BeamPresenceMode_t::CSIDE:
      token = "C";
      break;
    case BeamPresenceMode_t::EMPTY:
      token = "E";
      break;
    case BeamPresenceMode_t::NONE:
      token = "NONE";
      break;
    default:
      break;
  }
  return token;
}

BCTask::BeamPresenceMode_t BCTask::getBeamPresenceMode(const std::string_view beamname) const
{
  if (beamname == "ASIDE") {
    return BeamPresenceMode_t::ASIDE;
  }
  if (beamname == "CSIDE") {
    return BeamPresenceMode_t::CSIDE;
  }
  if (beamname == "BOTH") {
    return BeamPresenceMode_t::BOTH;
  }
  if (beamname == "EMPTY") {
    return BeamPresenceMode_t::EMPTY;
  }
  if (beamname == "NONE") {
    return BeamPresenceMode_t::NONE;
  }
  return BeamPresenceMode_t::ANY;
}

void BCTask::loadTriggerClasses(const o2::ctp::CTPConfiguration* ctpconfig)
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
  mAllEMCALClasses.clear();

  for (auto& cls : ctpconfig->getCTPClasses()) {
    auto trgclsname = boost::algorithm::to_upper_copy(cls.name);
    auto tokens = tokenize(trgclsname, '-');
    auto triggercluster = tokens[3];
    if (triggercluster.find("EMC") == std::string::npos) {
      // Not an EMCAL trigger class
      continue;
    }
    if (mBeamMode != BeamPresenceMode_t::ANY) {
      if (tokens[1] != getBeamPresenceModeToken(mBeamMode)) {
        // beam mode not matching
        continue;
      }
    }
    ILOG(Info, Support) << "EMCAL trigger cluster: Found trigger class: " << trgclsname << ENDM;
    mAllEMCALClasses.insert(cls.classMask);
    if (tokens[0] == mTriggerAliases.find("MB")->second) {
      mTriggerClassIndices[EMCMinBias] = cls.classMask;
      ILOG(Info, Support) << "Trigger class index EMC min bias: " << mTriggerClassIndices[EMCMinBias] << " (Class name: " << cls.name << ")" << ENDM;
    }
    if (tokens[0] == mTriggerAliases.find("0EMC")->second) {
      mTriggerClassIndices[EMCL0] = cls.classMask;
      ILOG(Info, Support) << "Trigger class index EMC L0:       " << mTriggerClassIndices[EMCL0] << " (Class name: " << cls.name << ")" << ENDM;
    }
    if (tokens[0] == mTriggerAliases.find("0DMC")->second) {
      mTriggerClassIndices[DMCL0] = cls.classMask;
      ILOG(Info, Support) << "Trigger class index DMC L0:       " << mTriggerClassIndices[DMCL0] << " (Class name: " << cls.name << ")" << ENDM;
    }
  }
}

} // namespace o2::quality_control_modules::emcal
