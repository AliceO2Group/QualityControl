//#include <fairmq/FairMQDevice.h>
//
#include <InfoLogger/InfoLogger.hxx>
//
#include <InfoLogger/InfoLoggerFMQ.hxx>

using namespace AliceO2::InfoLogger;

#include <Framework/DataProcessorSpec.h>
#include <Framework/ConfigContext.h>
#include <Framework/WorkflowSpec.h>
#include <Framework/Task.h>
//#include "Framework/runDataProcessing.h"
#include "../include/QualityControl/QcInfoLogger.h"
//#include "../include/QualityControl/QcInfoLogger.h"
using namespace o2;
using namespace o2::framework;

//class SingletonInfoLogger// : public AliceO2::InfoLogger::InfoLogger
//{
//
// public:
//  static InfoLogger& GetInstance()
//  {
//    static InfoLogger* foo = []() {
//      std::cout << "once" << std::endl;
//      auto *info = new InfoLogger();
//      setFMQLogsToInfoLogger(info);
//      return info;
//    }();
//    return *foo;
//  }
//
// private:
//  SingletonInfoLogger()
//  {
//    LOG(INFO) << "asdf" ;
//  }
//  ~SingletonInfoLogger() = default;
//
//  // Disallow copying
//  SingletonInfoLogger& operator=(const SingletonInfoLogger&) = delete;
//  SingletonInfoLogger(const SingletonInfoLogger&) = delete;
//};

//InfoLogger qcInfoLogger;
//{
//
//}

//class ATask : public Task
//{
// public:
//  ATask(){}
//  void init(InitContext& ic) final
//  {
//    LOG(INFO) << "init";
//    setFMQLogsToInfoLogger(&infoLogger);
//    infoLogger << "QC infologger initialized with fmq" << InfoLogger::endm;
//    LOG(INFO) << "redirection done";
//  }
//  void run(ProcessingContext& pc) final
//  {
//    auto& result = pc.outputs().make<int>({ "dummy" }, 1);
//    //    LOG(INFO) << "run";
//  }
//
// private:
//  InfoLogger infoLogger;
//};

class ATask : public Task
{
 public:
  ATask() {
    infoContext context;
    context.setField(infoContext::FieldName::Facility, "QC");
    context.setField(infoContext::FieldName::System, "QC");
    ilog.setContext(context);
    setFMQLogsToInfoLogger(&ilog);
  }
  void init(InitContext& ic) final
  {
    ilog << "init in task, sent to infologger" << ENDM;
    LOG(INFO) << "init in task, sent to fairlogger";
  }
  void run(ProcessingContext& pc) final
  {
    if(i++ %10000 == 0) {
      ilog << "run " << i << ENDM;
      LOG(INFO) << "run (fairlogger) " << i;
    }
    auto& result = pc.outputs().make<int>({ "dummy" }, 1);
  }
  ~ATask(){
    unsetFMQLogsToInfoLogger();
  }

 private:
  size_t i = 0;
  InfoLogger ilog;
};

#include "Framework/runDataProcessing.h"

WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  return WorkflowSpec{
    DataProcessorSpec{
      "producer",
      Inputs{},
      {
        OutputSpec{ { "dummy" }, "TST", "TEST" },
      },
      adaptFromTask<ATask>() }
  };
}

//int main() {
//
//  InfoLogger infoLogger;
//
//  InfoLoggerContext context;
//
//  context.setField(InfoLoggerContext::FieldName::Facility, "QC");
//
//  context.setField(InfoLoggerContext::FieldName::System, "QC");
//
//  infoLogger.setContext(context);
//
//  setFMQLogsToInfoLogger(&infoLogger);
//
//  infoLogger << "QC infologger initialized with fmq" << InfoLogger::endm;
//
//
//
//  return 0;
//
//}

//class SingletonInfoLogger : public AliceO2::InfoLogger::InfoLogger // identical to QcInfoLogger
//
//{
//
// public:
//  static SingletonInfoLogger& GetInstance()
//
//  {
//    // Guaranteed to be destroyed. Instantiated on first use
//    static SingletonInfoLogger foo;
//    return foo;
//  }
//
// private:
//  SingletonInfoLogger()
//
//  {
//    setFMQLogsToInfoLogger(this);
//    *this << "Singleton infologger initialized" << infologger::endm;
//  }
//
//  ~SingletonInfoLogger(){
////    fair::Logger::RemoveCustomSink("infoLogger");
//  }
//  // Disallow copying
//  SingletonInfoLogger& operator=(const SingletonInfoLogger&) = delete;
//  SingletonInfoLogger(const SingletonInfoLogger&) = delete;
//};
//
//int main()
//{
//  SingletonInfoLogger::GetInstance() << "hello" << infologger::endm;
//  SingletonInfoLogger::GetInstance() << "main" << infologger::endm;
//  LOG(INFO) << "fair main";
//  return 0;
//}
