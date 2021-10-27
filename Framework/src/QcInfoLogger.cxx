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

void QcInfoLogger::setFacility(const std::string& facility)
{
  mContext->setField(infoContext::FieldName::Facility, facility);
  mContext->setField(infoContext::FieldName::System, "QC");
  instance->setContext(*mContext);
  ILOG(Debug, Support) << "Facility set to " << facility << ENDM;
}

void QcInfoLogger::setDetector(const std::string& detector)
{
  mContext->setField(infoContext::FieldName::Detector, detector);
  instance->setContext(*mContext);
  ILOG(Debug, Support) << "Detector set to " << detector << ENDM;
}

void QcInfoLogger::setRun(int run)
{
  if (run > 0) {
    mContext->setField(infoContext::FieldName::Run, std::to_string(run));
  }
  instance->setContext(*mContext);
  ILOG(Debug, Support) << "IL: Run set to " << run << ENDM;
}

void QcInfoLogger::setPartition(const std::string& partitionName)
{
  mContext->setField(infoContext::FieldName::Partition, partitionName);
  instance->setContext(*mContext);
  ILOG(Debug, Support) << "IL: Partition set to " << partitionName << ENDM;
}

void QcInfoLogger::init(const std::string& facility,
                        bool discardDebug,
                        int discardFromLevel,
                        AliceO2::InfoLogger::InfoLogger* dplInfoLogger,
                        AliceO2::InfoLogger::InfoLoggerContext* dplContext,
                        int run,
                        std::string partitionName)
{
  if(dplInfoLogger && dplContext) {
    // we ignore the small memory leak that might occur if we are replacing the default InfoLogger
    instance = dplInfoLogger;
    mContext = dplContext;
  }
  setFacility(facility);
  setRun(run);
  setPartition(partitionName);

  // Set the proper discard filters
  ILOG_INST.filterDiscardDebug(discardDebug);
  ILOG_INST.filterDiscardLevel(discardFromLevel);
  // we use cout because we might have just muted ourselves
  ILOG(Debug, Ops) << "QC infologger initialized" << ENDM;
  ILOG(Debug, Support) << "   Discard debug ? " << discardDebug << ENDM;
  ILOG(Debug, Support) << "   Discard from level ? " << discardFromLevel << ENDM;
}

void QcInfoLogger::init(const std::string& facility,
                        const boost::property_tree::ptree& config,
                        AliceO2::InfoLogger::InfoLogger* dplInfoLogger ,
                        AliceO2::InfoLogger::InfoLoggerContext* dplContext,
                        int run,
                        std::string partitionName)
{
  std::string discardDebugStr = config.get<std::string>("qc.config.infologger.filterDiscardDebug", "false");
  bool discardDebug = discardDebugStr == "true" ? 1 : 0;
  int discardLevel = config.get<int>("qc.config.infologger.filterDiscardLevel", 21 /* Discard Trace */);
  init(facility, discardDebug, discardLevel, dplInfoLogger, dplContext, run, partitionName);
}

} // namespace o2::quality_control::core
