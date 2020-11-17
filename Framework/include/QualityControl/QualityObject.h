// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef QC_CORE_QUALITYOBJECT_H
#define QC_CORE_QUALITYOBJECT_H

// std
#include <map>
// ROOT
#include <TObject.h>
// O2
#include <Common/Exceptions.h>
// QC
#include "QualityControl/Quality.h"

namespace o2::quality_control::core
{

/// \brief  Encapsulation of a Quality into a TObject that can be streamed and stored.
///
/// \author Barthelemy von Haller
class QualityObject : public TObject
{
 public:
  /// Default constructor
  QualityObject();

  QualityObject(
    Quality quality,
    std::string checkName,
    std::string detectorName = "DET",
    std::string policyName = "",
    std::vector<std::string> inputs = {},
    std::vector<std::string> monitorObjectsNames = {},
    std::map<std::string, std::string> metadata = {});

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

  friend std::ostream& operator<<(std::ostream& out, const QualityObject& q); // output

  /// \brief Return the name of the check.
  /// @return The name of the check.
  /// @deprecated Prefer getCheckName()
  std::string getName() const;

  /// \brief Return the name of the check.
  /// @return The name of the check.
  /// @deprecated Prefer getCheckName()
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
  Quality getQuality() const;

  /**
   * Use o2::framework::DataSpecUtils::describe(input) to get string
   */
  void setInputs(std::vector<std::string> inputs) { mInputs = inputs; }
  std::vector<std::string> getInputs() const { return mInputs; }

  /// \brief Add key value pair that will end up in the database
  /// Add a metadata (key value pair) to the QualityObject. It will be stored in the database.
  /// If the key already exists the value will be updated.
  void addMetadata(std::string key, std::string value);
  /// \brief Add key value pairs that will end up in the database as metadata of the object
  /// Add all the key-value pairs in the map to the MonitorObject. It will be stored in the database as metadata.
  /// If a key already exists the value will NOT be updated.
  void addMetadata(std::map<std::string, std::string> pairs);
  /// \brief Update the value of metadata.
  /// If the key does not exist it will ignore it.
  void updateMetadata(std::string key, std::string value);
  /// \brief Get the full map of user's metadata
  const std::map<std::string, std::string>& getMetadataMap() const;
  /// \brief Get a metadata
  /// \return the value corresponding to the key if it was found.
  /// \throw ObjectNotFoundError in case the key is not found.
  const std::string getMetadata(std::string key);

  /// \brief Build the path to this object.
  /// Build the path to this object as it will appear in the GUI.
  /// If the QO was generated by the policy OnEachSeparately, it appends <task_name>/<mo_name> at the end
  /// \return A string containing the path.
  std::string getPath() const;

  const std::string& getDetectorName() const;
  void setDetectorName(const std::string& detectorName);

  void setQuality(const Quality& quality);
  const std::string& getCheckName() const;

  const std::string& getPolicyName() const;

  const std::vector<std::string> getMonitorObjectsNames() const;

 private:
  Quality mQuality;
  std::string mCheckName;
  std::string mDetectorName;
  std::string mPolicyName;
  std::vector<std::string> mInputs;
  std::vector<std::string> mMonitorObjectsNames;

  ClassDefOverride(QualityObject, 3);
};

using QualityObjectsType = std::vector<std::shared_ptr<QualityObject>>;
using QualityObjectsMapType = std::map<std::string, std::shared_ptr<const QualityObject>>;

} // namespace o2::quality_control::core

#endif
