///
/// \file   EverIncreasingGraph.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CHECKER_EverIncreasingGraph_H_
#define QUALITYCONTROL_LIBS_CHECKER_EverIncreasingGraph_H_

#include <Common/DataBlock.h>

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/CheckInterface.h"

namespace AliceO2 {
namespace QualityControlModules {
namespace Daq {

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class EverIncreasingGraph : public AliceO2::QualityControl::Checker::CheckInterface
{
  public:
    /// Default constructor
    EverIncreasingGraph();
    /// Destructor
    ~EverIncreasingGraph() override;

    void configure(std::string name) override;
    Quality check(const MonitorObject *mo) override;
    void beautify(MonitorObject *mo, Quality checkResult = Quality::Null) override;
    std::string getAcceptedType() override;

  private:
    DataBlockId mLastId;

    ClassDefOverride(EverIncreasingGraph,1);
};

} // namespace Example
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CHECKER_EverIncreasingGraph_H_
