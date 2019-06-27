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

#include "TaskInterface.h"
#include <InfoLogger/InfoLogger.hxx>
#include <iostream>

typedef AliceO2::InfoLogger::InfoLogger infologger; // not to have to type the full stuff each time -> log::endm

namespace o2::quality_control::core
{

/// \brief  Singleton class that any class in the QC can use to log.
///
/// The aim of this class is to avoid every class in the package to define
/// and configure its own instance of InfoLogger.
/// Independent InfoLogger instances can still be created when and if needed.
/// Usage :   QcInfoLogger::GetInstance() << "blabla" << infologger::endm;
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
    // TODO configure the QC infologger, e.g. proper facility
    *this << "QC infologger initialized" << infologger::endm;
  }

  ~QcInfoLogger() override {}

  // Disallow copying
  QcInfoLogger& operator=(const QcInfoLogger&) = delete;
  QcInfoLogger(const QcInfoLogger&) = delete;
};

} // namespace o2::quality_control::core

#endif // QC_CORE_QCINFOLOGGER_H
