///
/// \file   EverIncreasingGraph.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_DAQ_EVERINCREASINGRAPH_H
#define QC_MODULE_DAQ_EVERINCREASINGRAPH_H

#include <Common/DataBlock.h>

#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/CheckInterface.h"

namespace o2 {
namespace quality_control_modules {
namespace daq {

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class EverIncreasingGraph : public o2::quality_control::checker::CheckInterface
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

} // namespace daq
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_DAQ_EVERINCREASINGRAPH_H
