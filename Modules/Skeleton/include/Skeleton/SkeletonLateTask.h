//
// Created by pkonopka on 24/06/25.
//

#ifndef SKELETONLATETASK_H
#define SKELETONLATETASK_H

#include "QualityControl/LateTaskInterface.h"
#include <memory>

class TGraph;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::skeleton {
/// \brief Example Quality Control Task
/// \author My Name
class SkeletonLateTask final : public LateTaskInterface
{
  public:
  /// \brief Constructor
  SkeletonLateTask() = default;
  /// Destructor
  ~SkeletonLateTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void process(o2::framework::ProcessingContext& ctx) override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

  private:
  std::shared_ptr<TGraph> mGraph = nullptr;
};

}

#endif //SKELETONLATETASK_H
