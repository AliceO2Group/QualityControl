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
#include "Framework/RawDeviceService.h"
#include "Framework/runDataProcessing.h"
#include <Configuration/ConfigurationInterface.h>
#include <Configuration/ConfigurationFactory.h>
#include <Common/DataBlock.h>
//#include "DataSampling/SamplerFactory.h"
#include "QualityControl/TaskDevice.h"
#include "QualityControl/TaskConfig.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/TaskFactory.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/TaskDataProcessor.h"

using namespace std;
using namespace AliceO2;
using namespace AliceO2::Configuration;
using namespace AliceO2::Monitoring;
using namespace o2::framework;
using namespace o2::quality_control::core;
using namespace std::chrono;

//todo:
// rename it to taskDPL.cxx, move it to src, say it is not usable because there are no arguments passed to exe
// finish task in separate thread if possible

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

        auto qcTask = std::make_shared<TaskDataProcessor>(qcTaskName, qcConfigurationSource);
        qcTask->initCallback(initContext);

        return (AlgorithmSpec::ProcessCallback) [qcTask = std::move(qcTask)] (ProcessingContext &processingContext) mutable {
          qcTask->processCallback(processingContext);
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