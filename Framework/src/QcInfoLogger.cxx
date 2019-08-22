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

// clang-format off
namespace o2::quality_control::core
{
// clang-format on

  QcInfoLogger::QcInfoLogger()
  {
    infoContext context;
    context.setField(infoContext::FieldName::Facility, "QC");
    context.setField(infoContext::FieldName::System, "QC");
    *this << "QC infologger initialized" << infologger::endm;
    setFMQLogsToInfoLogger(this);  // theLog being our singleton of QcInfoLogger
  }

} // namespace o2::quality_control::core
