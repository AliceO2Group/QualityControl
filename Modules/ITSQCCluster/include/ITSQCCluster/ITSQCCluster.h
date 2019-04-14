///
/// \file   ITSQCCluster.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#ifndef QC_MODULE_ITSQCCLUSTER_ITSQCCLUSTER_H
#define QC_MODULE_ITSQCCLUSTER_ITSQCCLUSTER_H

#include "QualityControl/TaskInterface.h"

class TH1F;

using namespace o2::quality_control::core;

namespace o2
{
namespace quality_control_modules
{
namespace itsqccluster
{

/// \brief Example Quality Control DPL Task
/// It is final because there is no reason to derive from it. Just remove it if needed.
/// \author Barthelemy von Haller
/// \author Piotr Konopka
class ITSQCCluster /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  ITSQCCluster();
  /// Destructor
  ~ITSQCCluster() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  TH1F* mHistogram;
};

} // namespace itsqccluster
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_ITSQCCLUSTER_ITSQCCLUSTER_H

