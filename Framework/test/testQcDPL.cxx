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

  TaskDataProcessorFactory qcFactory;
  specs.push_back(qcFactory.create(qcTaskName, qcConfigurationSource));

  DataProcessorSpec checker{
    "checker",
    Inputs{
      {"aaa", "ITS", "HIST_DAQTASK", 0, InputSpec::QA}
    },
    Outputs{},
    AlgorithmSpec{
      (AlgorithmSpec::InitCallback) [](InitContext& initContext) {

        return (AlgorithmSpec::ProcessCallback) [] (ProcessingContext &processingContext) mutable {
          LOG(INFO) << "checker invoked";
        };
      }
    }
  };
  specs.push_back(checker);

  LOG(INFO) << "Using config file '" << qcConfigurationSource << "'";
  o2::framework::DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);
}