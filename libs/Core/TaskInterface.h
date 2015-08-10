///
/// \file   TaskInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_QCTASK_H_
#define QUALITYCONTROL_LIBS_CORE_QCTASK_H_

#include "DataBlock.h"
#include "Publisher.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

///
/// Dummy class that should be removed when there is the official one
/// \author   Barthelemy von Haller
class Activity
{
  public:
    /// Default constructor
    Activity() {}
    Activity(int id, int type) : mId(id), mType(type) {}
    /// Destructor
    virtual ~Activity() {}
    int mId;
    int mType;
};

/// \brief   Here you put a short description of the class
///
/// \author   Barthelemy von Haller
class TaskInterface
{
  public:
    /// Default constructor
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

} /* namespace Core */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_LIBS_CORE_QCTASK_H_ */
