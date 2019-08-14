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
/// @file    QcInfoLogger.h
/// @author  Barthelemy von Haller
///

#ifndef QC_CORE_QCINFOLOGGER_H
#define QC_CORE_QCINFOLOGGER_H

#include <InfoLogger/InfoLogger.hxx>
#include "QualityControl/TaskInterface.h"

typedef AliceO2::InfoLogger::InfoLogger infologger; // not to have to type the full stuff each time -> log::endm
typedef AliceO2::InfoLogger::InfoLoggerContext infoContext;

namespace o2::quality_control::core
{

/// \brief  Singleton class that any class in the QC can use to log.
///
/// The aim of this class is to avoid every class in the package to define
/// and configure its own instance of InfoLogger.
/// Independent InfoLogger instances can still be created when and if needed.
/// Usage :   QcInfoLogger::GetInstance() << "blabla" << infologger::endm;
///           ILOG << "info message" << ENDM;
///
/// \author Barthelemy von Haller
class QcInfoLogger : public AliceO2::InfoLogger::InfoLogger
{

 public:
  static QcInfoLogger& GetInstance()
  {
    // Guaranteed to be destroyed. Instantiated on first use
    static QcInfoLogger foo;
    return foo;
  }

 private:
  QcInfoLogger()
  {
    infoContext context;
    context.setField(infoContext::FieldName::Facility, "QC");
    context.setField(infoContext::FieldName::System, "QC");
    *this << "QC infologger initialized" << infologger::endm;
  }

  ~QcInfoLogger() override = default;

  // Disallow copying
  QcInfoLogger& operator=(const QcInfoLogger&) = delete;
  QcInfoLogger(const QcInfoLogger&) = delete;
};

} // namespace o2::quality_control::core

#define ILOGD(severity) o2::quality_control::core::QcInfoLogger::GetInstance() << AliceO2::InfoLogger::InfoLogger::Severity::severity
#define ILOG o2::quality_control::core::QcInfoLogger::GetInstance()
#define ILOGE o2::quality_control::core::QcInfoLogger::GetInstance() << AliceO2::InfoLogger::InfoLogger::Error
#define ILOGF o2::quality_control::core::QcInfoLogger::GetInstance() << AliceO2::InfoLogger::InfoLogger::Fatal
#define ILOGW o2::quality_control::core::QcInfoLogger::GetInstance() << AliceO2::InfoLogger::InfoLogger::Warning
#define ENDM AliceO2::InfoLogger::InfoLogger::endm;

#endif // QC_CORE_QCINFOLOGGER_H
