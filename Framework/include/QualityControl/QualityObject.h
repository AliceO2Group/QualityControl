
#ifndef QC_CORE_QUALITYOBJECT_H
#define QC_CORE_QUALITYOBJECT_H

// std
#include <iostream>
#include <map>
// ROOT
#include <TObject.h>
// O2
#include <Common/Exceptions.h>
// QC
#include "QualityControl/Quality.h"

namespace o2::quality_control::core
{

class QualityObject : public TObject
{
 public:
  /// Default constructor
  QualityObject(const std::string& checkerName, std::vector<std::string> inputs);
  QualityObject(const std::string& checkerName);
  QualityObject();

  /// Destructor
  ~QualityObject() override;

  /// Copy constructor
  QualityObject(const QualityObject& other) = default;
  /// Move constructor
  QualityObject(QualityObject&& other) /*noexcept*/ = default;
  /// Copy assignment operator
  QualityObject& operator=(const QualityObject& other) = default;
  /// Move assignment operator
  QualityObject& operator=(QualityObject&& other) /*noexcept*/ = default;

  /// \brief Return the name of the encapsulated object (if any).
  /// @return The name of the encapsulated object or "" if there is no object.
  const std::string getName() const { return mCheckName; };

  /// \brief Overwrite the TObject's method just to avoid confusion.
  /// @return The name of the encapsulated object or "" if there is no object.
  const char* GetName() const override;

  ///
  /// \brief Get the quality of this object.
  ///
  /// The method returns the lowest quality met amongst all the checks listed in \ref mChecks.
  /// If there are no checks, the method returns \ref Quality::Null.
  ///
  /// @return the quality of the object
  ///
  void updateQuality(Quality quality);
  Quality getQuality();

  /**
   * Use o2::framework::DataSpecUtils::describe(input) to get string
   */
  void setInputs(std::vector<std::string> inputs) { mInputs = inputs; }
  std::vector<std::string> getInputs() { return mInputs; }

  /// \brief Add key value pair that will end up in the database
  /// Add a metadata (key value pair) to the QualityObject. It will be stored in the database.
  /// If the key already exists the value will be updated.
  void addMetadata(std::string key, std::string value);

  std::map<std::string, std::string> getMetadataMap();

 private:
  //Quality
  unsigned int mQualityLevel;
  std::string mQualityName;

  std::string mCheckName;
  std::vector<std::string> mInputs;
  std::map<std::string, std::string> mUserMetadata;

  ClassDefOverride(QualityObject, 1);
};

} // namespace o2::quality_control::core

#endif
