///
/// \file   TaskInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_QCTASK_H_
#define QUALITYCONTROL_LIBS_CORE_QCTASK_H_

#include "DataFormat/DataBlock.h"
#include "QualityControl/ObjectsManager.h"

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
/// Purely abstract class defining the skeleton and the common interface of a QC task.
/// It is therefore the parent class of any QC task.
/// It is responsible for the instantiation, modification and destruction of the TObjects that are published.
///
/// It is part of the template method design pattern.
///
/// \author Barthelemy von Haller
class TaskInterface
{
  public:
    /// \brief Constructor
    /// Can't be used when dynamically loading the class with ROOT.
    /// @param name
    /// @param objectsManager
    TaskInterface(ObjectsManager *objectsManager);
    /// \brief Default constructor
    /// Remember to set an objectsManager.
    TaskInterface();
    /// Destructor
    virtual ~TaskInterface();
//    void Init(ObjectsManager *objectsManager);

    // Definition of the methods for the template method pattern
    virtual void initialize() = 0;
    virtual void startOfActivity(Activity &activity) = 0;
    virtual void startOfCycle() = 0;
    virtual void monitorDataBlock(DataBlock &block) = 0;
    virtual void endOfCycle() = 0;
    virtual void endOfActivity(Activity &activity) = 0;
    virtual void Reset() = 0;

    // Setters and getters
    void setObjectsManager(ObjectsManager *objectsManager);

  protected:
    ObjectsManager* getObjectsManager();

  private:
    ObjectsManager *mObjectsManager; // TODO should we rather have a global/singleton for the objectsManager ?

};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_QCTASK_H_
