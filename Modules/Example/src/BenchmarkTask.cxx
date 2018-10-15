///
/// \file   BenchmarkTask.cxx
/// \author Barthelemy von Haller
///

#include "Example/BenchmarkTask.h"
#include "QualityControl/QcInfoLogger.h"
#include <TCanvas.h>
#include <TH1.h>
#include <thread>

using namespace std;
using namespace o2::configuration;

namespace o2
{
namespace quality_control_modules
{
namespace example
{

BenchmarkTask::BenchmarkTask() : TaskInterface() {}

BenchmarkTask::~BenchmarkTask() {}

void BenchmarkTask::initialize(o2::framework::InitContext& ctx)
{
  QcInfoLogger::GetInstance() << "initialize benchmarktask \"" << getName() << "\""
                              << AliceO2::InfoLogger::InfoLogger::endm;

  mConfigFile = ConfigurationFactory::getConfiguration("file:./example.ini");
  string prefix = "/qc/tasks_config/" + getName();
  string taskDefinitionName = mConfigFile->get<std::string>(prefix + ".taskDefinition").value();
  mNumberHistos = mConfigFile->get<int>(taskDefinitionName + ".numberHistos").value();
  mNumberChecks = mConfigFile->get<int>(taskDefinitionName + ".numberChecks").value();
  mTypeOfChecks = mConfigFile->get<std::string>(taskDefinitionName + ".typeOfChecks").value();
  mModuleOfChecks = mConfigFile->get<std::string>(taskDefinitionName + ".moduleOfChecks").value();

  mHistos.reserve(mNumberHistos);

  // Create and publish the histos
  for (size_t i = 0; i < mNumberHistos; i++) {
    assert(mHistos.empty()); // because otherwise the code below makes no sense.
    stringstream name;
    name << "histogram_" << getName() << "_" << i;
    mHistos.push_back(new TH1F(name.str().c_str(), name.str().c_str(), 1000, -5, 5));
    getObjectsManager()->startPublishing(mHistos[i], name.str());

    // Add the checks
    for (size_t j = 0; j < mNumberChecks; j++) {
      getObjectsManager()->addCheck(name.str(), "fakeCheck_" + std::to_string(j), mTypeOfChecks, mModuleOfChecks);
    }
  }
}

void BenchmarkTask::startOfActivity(Activity& activity)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void BenchmarkTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void BenchmarkTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(100) /*100ms*/);
}

void BenchmarkTask::endOfCycle()
{
  for (auto histo : mHistos) {
    histo->Reset();
    histo->FillRandom("gaus", 1000);
  }
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void BenchmarkTask::endOfActivity(Activity& activity)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void BenchmarkTask::reset() { QcInfoLogger::GetInstance() << "Reset" << AliceO2::InfoLogger::InfoLogger::endm; }

} // namespace example
} // namespace quality_control_modules
} // namespace o2
