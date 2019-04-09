///
/// \file   EverIncreasingGraph.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_DAQ_EVERINCREASINGRAPH_H
#define QC_MODULE_DAQ_EVERINCREASINGRAPH_H

#include <Common/DataBlock.h>

#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

namespace o2::quality_control_modules::daq
{

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
  Quality check(const MonitorObject* mo) override;
  void beautify(MonitorObject* mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

 private:
  DataBlockId mLastId;

  ClassDefOverride(EverIncreasingGraph, 1);
};

} // namespace o2::quality_control_modules::daq

#endif // QC_MODULE_DAQ_EVERINCREASINGRAPH_H
