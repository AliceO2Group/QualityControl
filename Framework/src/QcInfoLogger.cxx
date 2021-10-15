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

QcInfoLogger::QcInfoLogger()
{
  mContext = std::make_shared<AliceO2::InfoLogger::InfoLoggerContext>();
  mContext->setField(infoContext::FieldName::Facility, "QC");
  mContext->setField(infoContext::FieldName::System, "QC");
  this->setContext(*mContext);
}

void QcInfoLogger::setFacility(const std::string& facility)
{
  mContext->setField(infoContext::FieldName::Facility, facility);
  mContext->setField(infoContext::FieldName::System, "QC");
  if (mDplContext) {
    mDplContext->setField(infoContext::FieldName::System, "QC");
    mDplContext->setField(infoContext::FieldName::Facility, facility);
  }
  this->setContext(*mContext);
  ILOG(Debug, Support) << "Facility set to " << facility << ENDM;
}

void QcInfoLogger::setDetector(const std::string& detector)
{
  mContext->setField(infoContext::FieldName::Detector, detector);
  if (mDplContext) {
    mDplContext->setField(infoContext::FieldName::Detector, detector);
  }
  this->setContext(*mContext);
  ILOG(Debug, Support) << "Detector set to " << detector << ENDM;
}

void QcInfoLogger::setRunPartition(int run, std::string& partitionName)
{
  if(run > 0) {
    mContext->setField(infoContext::FieldName::Run, std::to_string(run));
    if (mDplContext) {
      mDplContext->setField(infoContext::FieldName::Run, std::to_string(run));
    }
  }
  mContext->setField(infoContext::FieldName::Partition, partitionName);
  if (mDplContext) {
    mDplContext->setField(infoContext::FieldName::Partition, partitionName);
  }
  this->setContext(*mContext);
  ILOG(Debug, Support) << "IL: Run set to " << run << ENDM;
  ILOG(Debug, Support) << "IL: Partition set to " << partitionName << ENDM;
}

void QcInfoLogger::init(const std::string& facility, bool discardDebug, int discardFromLevel,
                        AliceO2::InfoLogger::InfoLoggerContext* dplContext,
                        int run,
                        std::string partitionName)
{
  mDplContext = dplContext;
  setFacility(facility);
  setRunPartition(run, partitionName);

  // Set the proper discard filters
  ILOG_INST.filterDiscardDebug(discardDebug);
  ILOG_INST.filterDiscardLevel(discardFromLevel);
  // we use cout because we might have just muted ourselves
  ILOG(Debug, Ops) << "QC infologger initialized" << ENDM;
  ILOG(Debug, Support) << "   Discard debug ? " << discardDebug << ENDM;
  ILOG(Debug, Support) << "   Discard from level ? " << discardFromLevel << ENDM;
}

void QcInfoLogger::init(const std::string& facility, const boost::property_tree::ptree& config,
                        AliceO2::InfoLogger::InfoLoggerContext* dplContext,
                        int run,
                        std::string partitionName)
{
  std::string discardDebugStr = config.get<std::string>("qc.config.infologger.filterDiscardDebug", "false");
  bool discardDebug = discardDebugStr == "true" ? 1 : 0;
  int discardLevel = config.get<int>("qc.config.infologger.filterDiscardLevel", 21 /* Discard Trace */);
  init(facility, discardDebug, discardLevel, dplContext);
}

} // namespace o2::quality_control::core
