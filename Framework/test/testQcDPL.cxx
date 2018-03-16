// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include <memory>

#include "Framework/DataSampling.h"
#include "Framework/runDataProcessing.h"
#include "QualityControl/TaskDataProcessor.h"

using namespace std;
using namespace AliceO2;
using namespace o2::framework;
using namespace o2::quality_control::core;
using namespace std::chrono;

//todo:
// rename it to taskDPL.cxx, move it to src, say it is not usable because there are no arguments passed to exe

void defineDataProcessing(vector<DataProcessorSpec>& specs)
{
  const string qcTaskName = "daqTask";
  const std::string qcConfigurationSource = std::string("file://") + getenv("QUALITYCONTROL_ROOT") + "/etc/qcTaskDplConfig.ini";

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



  LOG(INFO) << "Using config file '" << qcConfigurationSource << "'";
  o2::framework::DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);
}