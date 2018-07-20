///
/// \file   MonitorObject.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_MONITOROBJECT_H_
#define QUALITYCONTROL_LIBS_CORE_MONITOROBJECT_H_

// std
#include <map>
#include <iostream>
// ROOT
#include <TObject.h>
// QC
#include "QualityControl/Quality.h"

namespace o2 {
namespace quality_control {
namespace core {

/// \brief Container for the definition of a check
struct CheckDefinition {
    CheckDefinition():result(Quality::Null){}
    std::string name;
    std::string className;
    std::string libraryName;
    Quality     result;
};

/// \brief  This class keeps the metadata about one published object.
///
/// \author Barthelemy von Haller
class MonitorObject : public TObject
{
  public:
    /// Default constructor
    MonitorObject();
    MonitorObject(const std::string &name, TObject *object, const std::string &taskName);

    /// Destructor
    ~MonitorObject() override;

    /// Copy constructor
    MonitorObject (const MonitorObject& other) = default;
    /// Move constructor
    MonitorObject (MonitorObject&& other) /*noexcept*/ = default;
    /// Copy assignment operator
    MonitorObject& operator= (const MonitorObject& other) = default;
    /// Move assignment operator
    MonitorObject& operator= (MonitorObject&& other) /*noexcept*/ = default;

    const std::string &getName() const
    {
      return mName;
    }

    /// \brief Overwrite the TObject's method just to avoid confusion.
    ///        One should rather use getName().
    const char * 	GetName () const override
    {
      return getName().c_str();
    }

    void setName(const std::string &name)
    {
      mName = name;
    }

    const std::string &getTaskName() const
    {
      return mTaskName;
    }

    void setTaskName(const std::string &taskName)
    {
      mTaskName = taskName;
    }

    /**
     * \brief Get the quality of this object.
     *
     * The method returns the lowest quality met amongst all the checks listed in \ref mChecks.
     * If there are no checks, the method returns \ref Quality::Null.
     *
     * @return the quality of the object
     */
    const Quality getQuality() const;

    TObject *getObject() const
    {
      return mObject;
    }

    void setObject(TObject *object)
    {
      mObject = object;
    }

    std::map<std::string, CheckDefinition> getChecks() const
    {
      return mChecks;
    }

    bool isIsOwner() const
    {
      return mIsOwner;
    }

    void setIsOwner(bool isOwner)
    {
      mIsOwner = isOwner;
    }

    /// \brief Add a check to be executed on this object when computing the quality.
    /// If a check with the same name already exists it will be replaced by this check.
    /// Several checks can be added for the same check class name, but with different names (and
    /// they will get different configuration).
    /// \author Barthelemy von Haller
    /// \param name Arbitrary name to identify this Check.
    /// \param checkClassName The name of the class of the Check.
    /// \param checkLibraryName The name of the library containing the Check. If not specified it is taken from already loaded libraries.
    void addCheck(const std::string name, const std::string checkClassName, const std::string checkLibraryName="");
    void addOrUpdateCheck(std::string checkName, CheckDefinition check);
    void setQualityForCheck(std::string checkName, Quality quality);
    CheckDefinition getCheck(std::string checkName) const;

    void 	Draw (Option_t *option) override;
    TObject *DrawClone(Option_t *option) const override;

    // Names of special objects published by the framework for each task, behind the scene.
  public:
    static constexpr char SYSTEM_OBJECT_PUBLICATION_LIST[] = "objectsList"; // list of objects published by the task

  private:
    std::string mName;
    TObject *mObject;
    std::map<std::string /*checkName*/, CheckDefinition> mChecks;
    std::string mTaskName;

    // indicates that we are the owner of mObject. It is the case by default. It is not the case when a task creates the object.
    // TODO : maybe we should always be the owner ?
    bool mIsOwner;

  ClassDefOverride(MonitorObject,2);
};

} // namespace core
} // namespace QualityControl
} // namespace o2

#endif // QUALITYCONTROL_LIBS_CORE_MONITOROBJECT_H_
