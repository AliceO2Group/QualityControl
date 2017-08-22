///
/// \file   TaskInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_QCTASK_H_
#define QUALITYCONTROL_LIBS_CORE_QCTASK_H_

#include <memory>

#include "QualityControl/ObjectsManager.h"
#include <DataFormat/DataSet.h>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

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
    explicit TaskInterface(ObjectsManager *objectsManager);

    /// \brief Default constructor
    TaskInterface();

    /// \brief Destructor
    virtual ~TaskInterface() noexcept = default;
    /// Copy constructor
    TaskInterface (const TaskInterface& other) = default;
    /// Move constructor
    TaskInterface (TaskInterface&& other) noexcept = default;
    /// Copy assignment operator
    TaskInterface& operator= (const TaskInterface& other) = default;
    /// Move assignment operator
    TaskInterface& operator= (TaskInterface&& other) noexcept = default;


    // Definition of the methods for the template method pattern
    virtual void initialize() = 0;
    virtual void startOfActivity(Activity &activity) = 0;
    virtual void startOfCycle() = 0;
    virtual void monitorDataBlock(DataSetReference block) = 0;
    virtual void endOfCycle() = 0;
    virtual void endOfActivity(Activity &activity) = 0;
    virtual void reset() = 0;

    // Setters and getters
    void setObjectsManager(std::shared_ptr<ObjectsManager> objectsManager);
    void setName(const std::string &name);
    const std::string &getName() const;

  protected:
    std::shared_ptr<ObjectsManager> getObjectsManager();

  private:
    std::shared_ptr<ObjectsManager> mObjectsManager; // TODO should we rather have a global/singleton for the objectsManager ?
    std::string mName;
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_QCTASK_H_
