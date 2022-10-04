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
/// @file    QcInfoLogger.h
/// @author  Barthelemy von Haller
///

#ifndef QC_CORE_QCINFOLOGGER_H
#define QC_CORE_QCINFOLOGGER_H

#include <InfoLogger/InfoLogger.hxx>
#include <InfoLogger/InfoLoggerMacros.hxx>
#include <boost/property_tree/ptree_fwd.hpp>

typedef AliceO2::InfoLogger::InfoLogger infologger; // not to have to type the full stuff each time
typedef AliceO2::InfoLogger::InfoLoggerContext infoContext;

namespace o2::quality_control::core
{

struct DiscardFileParameters {
  bool debug = false;
  int fromLevel = 21 /* Discard Trace */;
  std::string discardFile;
  unsigned long rotateMaxBytes = 0;
  unsigned int rotateMaxFiles = 0;
};

/// \brief  Singleton class that any class in the QC can use to log.
///
/// The aim of this class is to avoid every class in the package to define
/// and configure its own instance of InfoLogger.
/// Independent InfoLogger instances can still be created when and if needed.
/// Usage :   QcInfoLogger::GetInstance() << "blabla" << infologger::endm;
///           ILOG(Info) << "info message with implicit level Support" << ENDM; // short version
///           ILOGI << "info message" << ENDM;      // shorter
///           ILOG_INST << InfoLogger::InfoLoggerMessageOption{ InfoLogger::Fatal, 1, 1, "asdf", 3 }
///                     << "fatal message with extra fields" << ENDM; // complex version
///           ILOG(Info, Ops) << "Test message with severity Info and level Ops, see InfoLoggerMacros.hxx" << ENDM;
///
/// \author Barthelemy von Haller
class QcInfoLogger
{
 public:
  static AliceO2::InfoLogger::InfoLogger& GetInfoLogger()
  {
    return *instance;
  }

  // disable non-static
  QcInfoLogger() = delete;
  ~QcInfoLogger() = delete;
  QcInfoLogger& operator=(const QcInfoLogger&) = delete;
  QcInfoLogger(const QcInfoLogger&) = delete;

  static void setFacility(const std::string& facility);
  static void setDetector(const std::string& detector);
  static void setRun(int run);
  static void setPartition(const std::string& partitionName);
  static void init(const std::string& facility,
                   const DiscardFileParameters& discardFileParameters,
                   AliceO2::InfoLogger::InfoLogger* dplInfoLogger = nullptr,
                   AliceO2::InfoLogger::InfoLoggerContext* dplContext = nullptr,
                   int run = -1,
                   const std::string& partitionName = "");
  static void init(const std::string& facility,
                   const boost::property_tree::ptree& config,
                   AliceO2::InfoLogger::InfoLogger* dplInfoLogger = nullptr,
                   AliceO2::InfoLogger::InfoLoggerContext* dplContext = nullptr,
                   int run = -1,
                   const std::string& partitionName = "");

  // build a default infologger
  static class _init
  {
   public:
    _init()
    {
      instance = new AliceO2::InfoLogger::InfoLogger();
      mContext = new AliceO2::InfoLogger::InfoLoggerContext();
      mContext->setField(infoContext::FieldName::Facility, "QC");
      mContext->setField(infoContext::FieldName::System, "QC");
      instance->setContext(*mContext);
    }
  } _initializer;

 private:
  // raw pointers because we don't want to take ownership of dpl pointers and destroy them.
  // if we keep the default infologger it will any ways be valid till the end of the process.
  static AliceO2::InfoLogger::InfoLogger* instance;
  static AliceO2::InfoLogger::InfoLoggerContext* mContext;
};

} // namespace o2::quality_control::core

// Define shortcuts to our instance using macros.
#define ILOG_INST o2::quality_control::core::QcInfoLogger::GetInfoLogger()
#define ILOGI ILOG_INST << AliceO2::InfoLogger::InfoLogger::Info
#define ILOGW ILOG_INST << AliceO2::InfoLogger::InfoLogger::Warning
#define ILOGE ILOG_INST << AliceO2::InfoLogger::InfoLogger::Error
#define ILOGF ILOG_INST << AliceO2::InfoLogger::InfoLogger::Fatal
#define ENDM AliceO2::InfoLogger::InfoLogger::endm;

#define NUM_ARGS_(_1, _2, _3, _4, _5, _6, _7, _8, TOTAL, ...) TOTAL
#define NUM_ARGS(...) NUM_ARGS_(__VA_ARGS__, 6, 5, 4, 3, 2, 1, 0)
#define CONCATENATE(X, Y) X##Y
#define CONCATE(MACRO, NUMBER) CONCATENATE(MACRO, NUMBER)
#define VA_MACRO(MACRO, ...)            \
  CONCATE(MACRO, NUM_ARGS(__VA_ARGS__)) \
  (__VA_ARGS__)

#define ILOG(...) VA_MACRO(ILOG, void, void, __VA_ARGS__)
// TODO understand why the zero argument does not work.
// the code is derived from https://stackoverflow.com/questions/16683146/can-macros-be-overloaded-by-number-of-arguments
#define ILOG0(s, t) \
  ILOG_INST << AliceO2::InfoLogger::InfoLogger::InfoLoggerMessageOption { AliceO2::InfoLogger::InfoLogger::Severity::Info, AliceO2::InfoLogger::InfoLogger::Level::Support, AliceO2::InfoLogger::InfoLogger::undefinedMessageOption.errorCode, __FILE__, __LINE__ }
#define ILOG1(s, t, severity) \
  ILOG_INST << AliceO2::InfoLogger::InfoLogger::InfoLoggerMessageOption { AliceO2::InfoLogger::InfoLogger::Severity::severity, AliceO2::InfoLogger::InfoLogger::Level::Support, AliceO2::InfoLogger::InfoLogger::undefinedMessageOption.errorCode, __FILE__, __LINE__ }
#define ILOG2(s, t, severity, level) \
  ILOG_INST << AliceO2::InfoLogger::InfoLogger::InfoLoggerMessageOption { AliceO2::InfoLogger::InfoLogger::Severity::severity, AliceO2::InfoLogger::InfoLogger::Level::level, AliceO2::InfoLogger::InfoLogger::undefinedMessageOption.errorCode, __FILE__, __LINE__ }

#endif // QC_CORE_QCINFOLOGGER_H
