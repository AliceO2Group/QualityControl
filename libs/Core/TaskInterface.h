///
/// \file   TaskInterface.h
///

#ifndef QUALITYCONTROL_LIBS_CORE_QCTASK_H_
#define QUALITYCONTROL_LIBS_CORE_QCTASK_H_

#include "DataBlock.h"
#include "Publisher.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

/// \brief Dummy class that should be removed when there is the official one.
/// \author   Barthelemy von Haller
class Activity
{
  public:
    /// Default constructor
    Activity(): mId(0), mType(0) {}
    Activity(int id, int type) : mId(id), mType(type) {}
    /// Destructor
    virtual ~Activity() {}
    int mId;
    int mType;
};

/// \brief  Skeleton of a QC task.
///
/// Purely abstract class defining the skeleton of a QC task.
/// It is needed for the template method design pattern.
/// It is responsible for the instantiation, modification and destruction of the TObjects that are published.
///
/// \author Barthelemy von Haller
class TaskInterface
{
  public:
    /// Constructor
    TaskInterface(std::string name, Publisher *publisher);
    /// Destructor
    virtual ~TaskInterface();

    // Definition of the methods for the template method pattern
    virtual void initialize() = 0;
    virtual void startOfActivity(Activity &activity) = 0;
    virtual void startOfCycle() = 0;
    virtual void monitorDataBlock(DataBlock &block) = 0;
    virtual void endOfCycle() = 0;
    virtual void endOfActivity(Activity &activity) = 0;
    virtual void Reset() = 0;

    // Setters and getters
    const std::string &getName() const;
    void setName(const std::string &name);
    void setPublisher(Publisher *publisher);

  protected:
    Publisher* getPublisher();

  private:
    std::string mName;
    Publisher *mPublisher;

};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_QCTASK_H_
