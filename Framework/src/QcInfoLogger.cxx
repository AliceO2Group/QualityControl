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
#include <InfoLogger/InfoLoggerFMQ.hxx>
//#

namespace o2::quality_control::core
{

QcInfoLogger::QcInfoLogger()
{
  infoContext context;
  context.setField(infoContext::FieldName::Facility, "QC");
  context.setField(infoContext::FieldName::System, "QC");
  this->setContext(context);
  //  setFMQLogsToInfoLogger(this); // disabled, see https://github.com/AliceO2Group/QualityControl/pull/222
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

void QcInfoLogger::init(const std::string& facility, bool discardDebug, int discardFromLevel)
{
  // facility
  infoContext context;
  context.setField(infoContext::FieldName::Facility, facility);
  context.setField(infoContext::FieldName::System, "QC");
  this->setContext(context);
  *this << LogDebugDevel << "Facility set to new " << facility << ENDM;

  // discard
  ILOG_INST.filterDiscardDebug(discardDebug);
  ILOG_INST.filterDiscardLevel(discardFromLevel);
  // we use cout because we might have just muted ourselves
  std::cout << "Discard debug ? " << discardDebug << std::endl;
  std::cout << "Discard from level " << discardFromLevel << std::endl;
}

} // namespace o2::quality_control::core
