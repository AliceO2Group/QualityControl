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
    MonitorObject(const std::string &name, TObject *object);

    /// Destructor
    virtual ~MonitorObject();

    const std::string &getName() const
    {
      return mName;
    }

    /// \brief Overwrite the TObject's method just to avoid confusion.
    ///        One should rather use getName().
    virtual const char * 	GetName () const
    {
      return getName().c_str();
    }

    void setName(const std::string &name)
    {
      mName = name;
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
      std::cout << "A" << std::endl;
      std::cout << mChecks.size() << std::endl;
      return mChecks;
    }

    /// \brief Add a checker to be executed on this object when computing the quality.
    /// If a checker with the same name already exists it will be replaced by this check class name.
    /// Several checkers can be added for the same checker class name, but with different names (and
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

    virtual void 	Draw (Option_t *option="") {
      mObject->Draw(option);
    }

  private:
    std::string mName;
    Quality mQuality;
    TObject *mObject;
//    std::map<std::string /* name */, std::string /* check class name */> mChecks;
    std::vector<CheckDefinition> mChecks;

    ClassDef(MonitorObject,1);
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_MONITOROBJECT_H_
