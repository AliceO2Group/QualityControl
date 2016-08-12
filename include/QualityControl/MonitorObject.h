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

namespace AliceO2 {
namespace QualityControl {
namespace Core {

struct CheckDefinition {
    std::string name;
    std::string className;
    std::string libraryName;
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
    virtual ~MonitorObject();

    const std::string &getName() const
    {
      return mName;
    }

    /// \brief Overwrite the TObject's method just to avoid confusion.
    ///        One should rather use getName().
    virtual const char * 	GetName () const override
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

    const Quality &getQuality() const
    {
      return mQuality;
    }

    void setQuality(const Quality &quality)
    {
      mQuality = quality;
    }

    TObject *getObject() const
    {
      return mObject;
    }

    void setObject(TObject *object)
    {
      mObject = object;
    }

    std::vector<CheckDefinition> getChecks() const
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
    void addCheck(const std::string name, const std::string checkClassName, const std::string checkLibraryName="")
    {
      CheckDefinition check;
      check.name = name;
      check.libraryName = checkLibraryName;
      check.className = checkClassName;
      mChecks.push_back(check);
    }

    virtual void 	Draw (Option_t *option="") override
    {
      mObject->Draw(option);
    }

    virtual TObject *DrawClone(Option_t *option="") const override
    {
      MonitorObject* clone = new MonitorObject();
      clone->setName(this->getName());
      clone->setTaskName(this->getTaskName());
      clone->setObject(mObject->DrawClone(option));
      return clone;
    }

  private:
    std::string mName;
    Quality mQuality;
    TObject *mObject;
    std::vector<CheckDefinition> mChecks;
    std::string mTaskName;

    // indicates that we are the owner of mObject. It is the case by default. It is not the case when a task creates the object.
    // TODO : maybe we should always be the owner ?
    bool mIsOwner;

  ClassDefOverride(MonitorObject,1);
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_MONITOROBJECT_H_
