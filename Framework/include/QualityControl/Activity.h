///
/// \file   Activity.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_CORE_ACTIVITY_H
#define QUALITYCONTROL_CORE_ACTIVITY_H

namespace o2 {
namespace quality_control {
namespace core {

/// \brief Dummy class that should be removed when there is the official one.
/// This corresponds to a Run1/2 "run".
/// \author   Barthelemy von Haller
class Activity
{
  public:
  Activity() = default;
  Activity(int id, int type) : mId(id), mType(type)
  {}
  /// Copy constructor
  Activity (const Activity& other) = default;
  /// Move constructor
  Activity (Activity&& other) noexcept = default;
  /// Copy assignment operator
  Activity& operator= (const Activity& other) = default;
  /// Move assignment operator
  Activity& operator= (Activity&& other) noexcept = default;

  virtual ~Activity() = default;

  int mId{0};
  int mType{0};
};

} // namespace core
} // namespace QualityControl
} // namespace o2

#endif // QUALITYCONTROL_CORE_ACTIVITY_H
