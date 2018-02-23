// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <memory>

#include <fairmq/FairMQDevice.h>

#include "Framework/DataSampling.h"
#include "Framework/runDataProcessing.h"
#include <Configuration/ConfigurationInterface.h>
#include <Configuration/ConfigurationFactory.h>
#include <Common/DataBlock.h>
#include "DataSampling/SamplerFactory.h"
#include "QualityControl/TaskDevice.h"
#include "QualityControl/TaskConfig.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/TaskFactory.h"
#include "QualityControl/QcInfoLogger.h"

using namespace std;
using namespace AliceO2;
using namespace AliceO2::Configuration;
using namespace AliceO2::Monitoring;
using namespace o2::framework;
using namespace o2::quality_control::core;
using namespace std::chrono;

DataSetReference convertO2DataModelToDataSetReference(o2::framework::DataRef& input)
{
  const auto* header = o2::header::get<o2::header::DataHeader>(input.header);

  DataSetReference mDataSet = make_shared<DataSet>();

  // header
  static DataBlockId id = 1;

  auto block = make_shared<SelfReleasingBlockContainer>();
  block->getData()->header.blockType = DataBlockType::H_BASE; // = *static_cast<DataBlockHeaderBase *>(msg->GetData());
  block->getData()->header.headerSize = sizeof(DataBlockHeaderBase);
  block->getData()->header.dataSize = (uint32_t) header->payloadSize;
  block->getData()->header.id = id++;
  block->getData()->header.linkId = 0;

  block->getData()->data = new char[header->payloadSize];
  memcpy(block->getData()->data, const_cast<char*>(input.payload), header->payloadSize);

  mDataSet->push_back(block);

  return mDataSet;
}


void defineDataProcessing(vector<DataProcessorSpec>& specs)
// clion introspections go bananas in this function
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
#pragma clang diagnostic push
#pragma ide diagnostic ignored "CannotResolve"
{
  const string qcTaskName = "daqTask";
  const string qcConfigurationSource = "file:///home/pkonopka/alice/QualityControl/Framework/example-default.ini";

  DataProcessorSpec qcTask{
    "daqTask",
    Inputs{
      {"dataITS", "ITS", "RAWDATA_S", 0, InputSpec::Timeframe}
    },
    Outputs{
      {"ITS", "HISTO", 0, OutputSpec::Timeframe}
    },
    AlgorithmSpec{
      (AlgorithmSpec::InitCallback) [qcTaskName, qcConfigurationSource](InitContext& initContext) {
//        FairMQDevice* device = new o2::quality_control::core::TaskDevice("daqTask", "home/pkonopka/alice/O2/QualityControl/Framework/example-default.ini");
//        device->

//        sleep(10);

        auto device = initContext.services().get<RawDeviceService>().device();

        string mTaskName = qcTaskName;
        TaskConfig mTaskConfig;
        shared_ptr<AliceO2::Configuration::ConfigurationInterface> mConfigFile;
        shared_ptr<AliceO2::Monitoring::Collector> mCollector;
//        shared_ptr<AliceO2::DataSampling::SamplerInterface> mSampler;
        o2::quality_control::core::TaskInterface *mTask;
        shared_ptr<ObjectsManager> mObjectsManager;

        // stats
        int mTotalNumberObjectsPublished;
        AliceO2::Common::Timer timerTotalDurationActivity;
        ba::accumulator_set<double, ba::features<ba::tag::mean, ba::tag::variance>> pcpus;
        ba::accumulator_set<double, ba::features<ba::tag::mean, ba::tag::variance>> pmems;

        // --- TaskDevice::TaskDevice(std::string taskName, std::string configurationSource)

        // setup configuration
        mConfigFile = ConfigurationFactory::getConfiguration(qcConfigurationSource);
        // ---- populateConfig(mTaskName);
        {
          string taskDefinitionName = mConfigFile->get<string>(mTaskName + "/taskDefinition").value();

          mTaskConfig.taskName = mTaskName;
          mTaskConfig.moduleName = mConfigFile->get<string>(taskDefinitionName + "/moduleName").value();
          mTaskConfig.className = mConfigFile->get<string>(taskDefinitionName + "/className").value();
          mTaskConfig.cycleDurationSeconds = mConfigFile->get<int>(
            taskDefinitionName + "/cycleDurationSeconds").value_or(10);
          mTaskConfig.maxNumberCycles = mConfigFile->get<int>(taskDefinitionName + "/maxNumberCycles").value_or(-1);
        }

        // setup monitoring
        mCollector = MonitoringFactory::Get("infologger://");

        // setup publisher
        mObjectsManager = make_shared<ObjectsManager>(mTaskConfig);

        // setup task
        TaskFactory f;
        mTask = f.create(mTaskConfig, mObjectsManager);  // TODO could we use unique_ptr ?

        // setup datasampling
        string dataSamplingImplementation = mConfigFile->get<string>("DataSampling/implementation").value();
        QcInfoLogger::GetInstance() << "DataSampling implementation is '" << dataSamplingImplementation << "'"
                                    << InfoLogger::InfoLogger::endm;
//        mSampler = AliceO2::DataSampling::SamplerFactory::create(dataSamplingImplementation);

        // --- void TaskDevice::InitTask()
        {
          QcInfoLogger::GetInstance() << "initialize TaskDevice" << AliceO2::InfoLogger::InfoLogger::endm;

          // init user's task
          mTask->initialize();
        }



        // --- void TaskDevice::startOfActivity()
        {
          timerTotalDurationActivity.reset();
          Activity activity(mConfigFile->get<int>("Activity/number").value(),
                            mConfigFile->get<int>("Activity/type").value());
          mTask->startOfActivity(activity);
        }

        // --- void TaskDevice::Run()
        AliceO2::Common::Timer timer;
        timer.reset(10000000); // 10 s.
        int lastNumberObjects = 0;
        int cycle = 0;

        enum State { Idle, Cycle };
        State state = Idle;

        int numberBlocks = 0;
        auto start = system_clock::now();
        auto end = start + seconds(mTaskConfig.cycleDurationSeconds);

        return (AlgorithmSpec::ProcessCallback)
        [device,
            mTaskName,
            mTaskConfig,
            mConfigFile = std::move(mConfigFile),
            mCollector = std::move(mCollector),
//            mSampler = std::move(mSampler),
            mTask,
            mObjectsManager,// = std::move(mObjectsManager),
            mTotalNumberObjectsPublished,
            timerTotalDurationActivity,
            pcpus,
            pmems,
            timer,
            lastNumberObjects,
            cycle,
            state,
            numberBlocks,
            start,
            end]
          (ProcessingContext &processingContext) mutable {



          if (state == Idle) {
            if (/*CheckCurrentState(RUNNING) &&*/ (mTaskConfig.maxNumberCycles < 0 ||
                                                   cycle < mTaskConfig.maxNumberCycles)) {

              QcInfoLogger::GetInstance() << "cycle " << cycle << AliceO2::InfoLogger::InfoLogger::endm;
              // --- monitorCycle();
              {
                AliceO2::Common::Timer timer;
                mTask->startOfCycle();
                start = system_clock::now();
                end = start + seconds(mTaskConfig.cycleDurationSeconds);
                numberBlocks = 0;
              }
              state = Cycle;
            }
          } else // state == Cycle
          {
//              while (system_clock::now() < end) {
            //DataSetReference dataSetReference = mSampler->getData(100);
            DataRef input = *processingContext.inputs().begin();
            DataSetReference dataSetReference = convertO2DataModelToDataSetReference(input);
            if (dataSetReference) {
              mTask->monitorDataBlock(dataSetReference);
              //mSampler->releaseData(); // invalids the block !!!
              numberBlocks++;
            }
//              }
            if (system_clock::now() > end) {

              mTask->endOfCycle();

              // publication
              unsigned long numberObjectsPublished = //unsigned long TaskDevice::publish()
                ({
                  unsigned int sentMessages = 0;

                  for (auto& pair : *mObjectsManager) {
                    auto* mo = pair.second;
                    auto* message = new TMessage(kMESS_OBJECT); // will be deleted by fairmq using our custom method
                    message->WriteObjectAny(mo, mo->IsA());
                    FairMQMessagePtr msg(device->NewMessage(message->Buffer(),
                                                            message->BufferSize(),
                                                            [](void* data, void* object) { delete (TMessage*) object; },
                                                            message));
                    QcInfoLogger::GetInstance() << "Sending \"" << mo->getName() << "\""
                                                << AliceO2::InfoLogger::InfoLogger::endm;
                    device->Send(msg, "data-out");
                    sentMessages++;
                  }

                  sentMessages;
                });

              // monitoring metrics
              double durationCycle = timer.getTime();
              timer.reset();
              double durationPublication = timer.getTime();
              mCollector->send(numberBlocks, "QC_task_Numberofblocks_in_cycle");
              mCollector->send(durationCycle, "QC_task_Module_cycle_duration");
              mCollector->send(durationPublication, "QC_task_Publication_duration");
              mCollector->send((int) numberObjectsPublished,
                               "QC_task_Number_objects_published_in_cycle"); // cast due to Monitoring accepting only int
              double rate = numberObjectsPublished / (durationCycle + durationPublication);
              mCollector->send(rate, "QC_task_Rate_objects_published_per_second");
              mTotalNumberObjectsPublished += numberObjectsPublished;
              //std::vector<std::string> pidStatus = mMonitor->getPIDStatus(::getpid());
              //pcpus(std::stod(pidStatus[3]));
              //pmems(std::stod(pidStatus[4]));
              double whole_run_rate = mTotalNumberObjectsPublished / timerTotalDurationActivity.getTime();
              mCollector->send(mTotalNumberObjectsPublished, "QC_task_Total_objects_published_whole_run");
              mCollector->send(timerTotalDurationActivity.getTime(), "QC_task_Total_duration_activity_whole_run");
              mCollector->send(whole_run_rate, "QC_task_Rate_objects_published_per_second_whole_run");
//  mCollector->send(std::stod(pidStatus[3]), "QC_task_Mean_pcpu_whole_run");
              mCollector->send(ba::mean(pmems), "QC_task_Mean_pmem_whole_run");

              cycle++;

              // if 10 s we publish stats
              if (timer.isTimeout()) {
                double current = timer.getTime();
                int objectsPublished = (mTotalNumberObjectsPublished - lastNumberObjects);
                lastNumberObjects = mTotalNumberObjectsPublished;
                mCollector->send(objectsPublished / current, "QC_task_Rate_objects_published_per_10_seconds");
                timer.increment();
              }

              // in the future the end of an activity/run will come from the control
              // endOfActivity();

              state = Idle;
            }
          }
        };
      }
    },
    {
      ConfigParamSpec{
        "channel-config", VariantType::String,
        "name=data-out,type=pub,method=bind,address=tcp://*:5556,rateLogging=0", {"Out-of-band channel config"}}
    }
  };

  specs.push_back(qcTask);

  string configFilePath = "file:///home/pkonopka/alice/QualityControl/Framework/dataSamplingConfig.ini";

  LOG(INFO) << "Using config file '" << configFilePath << "'";

  o2::framework::DataSampling::GenerateInfrastructure(specs, configFilePath);
}
#pragma clang diagnostic pop
#pragma clang diagnostic pop