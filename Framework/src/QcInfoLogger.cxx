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
/// \file   QcInfoLogger.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/QcInfoLogger.h"
#include <boost/property_tree/ptree.hpp>

namespace o2::quality_control::core
{

QcInfoLogger::QcInfoLogger()
{
  infoContext context;
  context.setField(infoContext::FieldName::Facility, "QC");
  context.setField(infoContext::FieldName::System, "QC");
  this->setContext(context);
  *this << "QC infologger initialized" << ENDM;
}

void QcInfoLogger::setFacility(const std::string& facility)
{
  infoContext context;
  context.setField(infoContext::FieldName::Facility, facility);
  context.setField(infoContext::FieldName::System, "QC");
  this->setContext(context);
  *this << LogDebugDevel << "Facility set to old " << facility << ENDM;
}

void QcInfoLogger::init(const std::string& facility, bool discardDebug, int discardFromLevel, AliceO2::InfoLogger::InfoLoggerContext* dplContext)
{
  // Set Facility and System
  infoContext context;
  context.setField(infoContext::FieldName::Facility, facility);
  dplContext->setField(infoContext::FieldName::Facility, facility);
  context.setField(infoContext::FieldName::System, "QC");
  dplContext->setField(infoContext::FieldName::System, "QC");
  this->setContext(context);
  *this << LogDebugDevel << "Facility set to " << facility << ENDM;

  // Set the proper discard filters
  ILOG_INST.filterDiscardDebug(discardDebug);
  ILOG_INST.filterDiscardLevel(discardFromLevel);
  // we use cout because we might have just muted ourselves
  std::cout << "Discard debug ? " << discardDebug << std::endl;
  std::cout << "Discard from level " << discardFromLevel << std::endl;
}

void QcInfoLogger::init(const std::string& facility, const boost::property_tree::ptree& config, AliceO2::InfoLogger::InfoLoggerContext* dplContext)
{
  std::string discardDebugStr = config.get<std::string>("qc.config.infologger.filterDiscardDebug", "false");
  bool discardDebug = discardDebugStr == "true" ? 1 : 0;
  int discardLevel = config.get<int>("qc.config.infologger.filterDiscardLevel", 21 /* Discard Trace */);
  init(facility, discardDebug, discardLevel, dplContext);
}

} // namespace o2::quality_control::core
