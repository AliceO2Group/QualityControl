///
/// \file   BenchmarkTask.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_EXAMPLE_BENCHMARKTASK_H
#define QC_MODULE_EXAMPLE_BENCHMARKTASK_H

#include "QualityControl/TaskInterface.h"

#include <vector>
#include "Configuration/Configuration.h"

class TH1F;

using namespace AliceO2::QualityControl::Core;

namespace AliceO2 {
namespace QualityControlModules {
namespace Example {

/// \brief Quality Control Task for benchmarking
/// It publishes a number of TH1F (configurable, see example.ini in module QualityControl).
/// The histos are reset and refilled (1000 random) at EOC. They have 1000 bins.
/// The monitoring of data blocks is empty (sleep 100 ms).
/// \author Barthelemy von Haller
class BenchmarkTask: public TaskInterface
{
  public:
    /// \brief Constructor
    BenchmarkTask();
    /// Destructor
    ~BenchmarkTask() override;

    // Definition of the methods for the template method pattern
    void initialize() override;
    void startOfActivity(Activity &activity) override;
    void startOfCycle() override;
    void monitorDataBlock(DataSetReference block) override;
    void endOfCycle() override;
    void endOfActivity(Activity &activity) override;
    void reset() override;

  private:

    std::vector<TH1F*> mHistos;
    AliceO2::Configuration::ConfigFile mConfigFile;
    size_t mNumberHistos;
    size_t mNumberChecks;
    std::string mTypeOfChecks;
    std::string mModuleOfChecks;

//    ClassDef(BenchmarkTask,1);
};

}
}
}

#endif //QC_MODULE_EXAMPLE_BENCHMARKTASK_H
