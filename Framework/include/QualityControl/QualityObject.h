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

/// \brief  Encapsulation of a Quality into a TObject that can be streamed and stored.
///
/// \author Barthelemy von Haller
class QualityObject : public TObject
{
 public:
  /// Default constructor
  explicit QualityObject(const std::string& checkName, std::vector<std::string> inputs = {}, const std::string& detectorName = "DET");
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
  [[nodiscard]] std::string getName() const { return mCheckName; };

  /// \brief Overwrite the TObject's method just to avoid confusion.
  /// @return The name of the encapsulated object or "" if there is no object.
  [[nodiscard]] const char* GetName() const override;

  ///
  /// \brief Get the quality of this object.
  ///
  /// The method returns the lowest quality met amongst all the checks listed in \ref mChecks.
  /// If there are no checks, the method returns \ref Quality::Null.
  ///
  /// @return the quality of the object
  ///
  void updateQuality(Quality quality);
  [[nodiscard]] Quality getQuality() const;

  /**
   * Use o2::framework::DataSpecUtils::describe(input) to get string
   */
  void setInputs(std::vector<std::string> inputs) { mInputs = inputs; }
  [[nodiscard]] std::vector<std::string> getInputs() const { return mInputs; }

  /// \brief Add key value pair that will end up in the database
  /// Add a metadata (key value pair) to the QualityObject. It will be stored in the database.
  /// If the key already exists the value will be updated.
  void addMetadata(std::string key, std::string value);

  std::map<std::string, std::string> getMetadataMap();

  /// \brief Build the path to this object.
  /// Build the path to this object as it will appear in the GUI.
  /// \return A string containing the path.
  [[nodiscard]] std::string getPath() const;

  [[nodiscard]] const std::string& getDetectorName() const;
  void setDetectorName(const std::string& detectorName);

  void setQuality(const Quality& quality);

 private:
  Quality mQuality;
  std::string mDetectorName;
  std::string mCheckName;
  std::vector<std::string> mInputs;
  std::map<std::string, std::string> mUserMetadata;

  ClassDefOverride(QualityObject, 2);
};

} // namespace o2::quality_control::core

#endif
