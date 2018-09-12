///
/// \file   SkeletonTask.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_SKELETON_SKELETONTASK_H
#define QC_MODULE_SKELETON_SKELETONTASK_H

#include "QualityControl/TaskInterface.h"

class TH1F;

using namespace o2::quality_control::core;

namespace o2 {
namespace quality_control_modules {
namespace skeleton {

/// \brief Example Quality Control Task
/// It is final because there is no reason to derive from it. Just remove it if needed.
/// \author Barthelemy von Haller
class SkeletonTask /*final*/: public TaskInterface // todo add back the "final" when doxygen is fixed
{
  public:
    /// \brief Constructor
    SkeletonTask();
    /// Destructor
    ~SkeletonTask() override;

    // Definition of the methods for the template method pattern
    void initialize() override;
    void startOfActivity(Activity &activity) override;
    void startOfCycle() override;
    void monitorDataBlock(DataSetReference block) override;
    void endOfCycle() override;
    void endOfActivity(Activity &activity) override;
    void reset() override;

  private:

    TH1F *mHistogram;
};

} // namespace skeleton
} // namespace quality_control_modules
} // namespace o2

#endif //QC_MODULE_SKELETON_SKELETONTASK_H
