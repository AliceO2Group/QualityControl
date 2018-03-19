///
/// \file   TaskInterface.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_QCTASK_H_
#define QUALITYCONTROL_LIBS_CORE_QCTASK_H_

#include <memory>

#include "QualityControl/ObjectsManager.h"
#include "QualityControl/Activity.h"
#include <Common/DataSet.h>

namespace o2 {
namespace quality_control {
namespace core {

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
    TaskInterface& operator= (TaskInterface&& other)/* noexcept */ = default; // error with gcc if noexcept


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

} // namespace core
} // namespace QualityControl
} // namespace o2

#endif // QUALITYCONTROL_LIBS_CORE_QCTASK_H_
