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
/// \file   QcInfoLogger.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/QcInfoLogger.h"
#include <boost/property_tree/ptree.hpp>

namespace o2::quality_control::core
{

AliceO2::InfoLogger::InfoLogger* QcInfoLogger::instance;
AliceO2::InfoLogger::InfoLoggerContext* QcInfoLogger::mContext;
QcInfoLogger::_init QcInfoLogger::_initializer;
bool QcInfoLogger::disabled = false;

void QcInfoLogger::setFacility(const std::string& facility)
{
  mContext->setField(infoContext::FieldName::Facility, facility);
  mContext->setField(infoContext::FieldName::System, "QC");
  instance->setContext(*mContext);
  ILOG(Debug, Devel) << "IL: Facility set to " << facility << ENDM;
}

void QcInfoLogger::setDetector(const std::string& detector)
{
  mContext->setField(infoContext::FieldName::Detector, detector);
  instance->setContext(*mContext);
  ILOG(Debug, Devel) << "IL: Detector set to " << detector << ENDM;
}

void QcInfoLogger::setRun(int run)
{
  if (run > 0) {
    mContext->setField(infoContext::FieldName::Run, std::to_string(run));
  }
  instance->setContext(*mContext);
  ILOG(Debug, Devel) << "IL: Run set to " << run << ENDM;
}

void QcInfoLogger::setPartition(const std::string& partitionName)
{
  if (partitionName.empty()) {
    ILOG(Debug, Devel) << "IL: Partition empty, we don't set it" << ENDM;
    return;
  }
  mContext->setField(infoContext::FieldName::Partition, partitionName);
  instance->setContext(*mContext);
  ILOG(Debug, Devel) << "IL: Partition set to " << partitionName << ENDM;
}

void QcInfoLogger::disable()
{
  QcInfoLogger::disabled = true;
  ILOG_INST.filterDiscardDebug(true);
  ILOG_INST.filterDiscardLevel(1);
}

using namespace std;

void QcInfoLogger::init(const std::string& facility,
                        const DiscardParameters& discardParameters,
                        AliceO2::InfoLogger::InfoLogger* dplInfoLogger,
                        AliceO2::InfoLogger::InfoLoggerContext* dplContext,
                        int run,
                        const std::string& partitionName)
{
  if (dplInfoLogger && dplContext) {
    // we ignore the small memory leak that might occur if we are replacing the default InfoLogger
    instance = dplInfoLogger;
    mContext = dplContext;
  }

  // Set the proper discard filters
  ILOG_INST.filterDiscardDebug(discardParameters.debug);
  ILOG_INST.filterDiscardLevel(discardParameters.fromLevel);
  if (disabled) {
    ILOG_INST.filterDiscardDebug(true);
    ILOG_INST.filterDiscardLevel(1);
  }
  if (!discardParameters.file.empty()) {
    ILOG_INST.filterDiscardSetFile(discardParameters.file.c_str(), discardParameters.rotateMaxBytes, discardParameters.rotateMaxFiles, 0, !discardParameters.debugInDiscardFile /*Do not store Debug messages in file*/);
  }
  ILOG(Debug, Support) << "QC infologger initialized : " << discardParameters.debug << " ; " << discardParameters.fromLevel << ENDM;
  ILOG(Debug, Devel) << "   Discard debug ? " << discardParameters.debug << " / Discard from level ? " << discardParameters.fromLevel << " / Discard to file ? " << (!discardParameters.file.empty() ? discardParameters.file : "No") << " / Discard max bytes and files ? " << discardParameters.rotateMaxBytes << " = " << discardParameters.rotateMaxFiles << " / Put discarded debug messages in file ? " << discardParameters.debugInDiscardFile << ENDM;

  setFacility(facility);
  setRun(run);
  setPartition(partitionName);
}

void QcInfoLogger::init(const std::string& facility,
                        const boost::property_tree::ptree& config,
                        AliceO2::InfoLogger::InfoLogger* dplInfoLogger,
                        AliceO2::InfoLogger::InfoLoggerContext* dplContext,
                        int run,
                        const std::string& partitionName)
{
  DiscardParameters discardParameters;
  auto discardDebugStr = config.get<std::string>("qc.config.infologger.filterDiscardDebug", "true");
  discardParameters.debug = discardDebugStr == "true";
  discardParameters.fromLevel = config.get<int>("qc.config.infologger.filterDiscardLevel", 21 /* Discard Trace */);
  discardParameters.file = config.get<std::string>("qc.config.infologger.filterDiscardFile", "");
  discardParameters.rotateMaxBytes = config.get<u_long>("infologger.filterRotateMaxBytes", 0);
  discardParameters.rotateMaxFiles = config.get<u_int>("infologger.filterRotateMaxFiles", 0);
  auto debugInDiscardFile = config.get<std::string>("qc.config.infologger.debugInDiscardFile", "false");
  discardParameters.debugInDiscardFile = debugInDiscardFile == "true";
  init(facility, discardParameters, dplInfoLogger, dplContext, run, partitionName);
}

} // namespace o2::quality_control::core
