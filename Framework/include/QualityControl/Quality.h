// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   Quality.h
/// \author Barthelemy von Haller
///

#ifndef QC_CORE_QUALITY_H
#define QC_CORE_QUALITY_H

#include <Rtypes.h>
#include <string>
#include <map>

namespace o2::quality_control::core
{

/// \brief  Class representing the quality of a MonitorObject.
///
/// \author Barthelemy von Haller
class Quality
{
 public:
  /// Default constructor
  /// Not 'explicit', we allow implicit conversion from uint to Quality.
  explicit Quality(unsigned int level = Quality::NullLevel, std::string name = "");
  /// Destructor
  virtual ~Quality() = default;
  // Copy constructor
  Quality(const Quality& q);

  unsigned int getLevel() const;
  const std::string& getName() const;

  static const Quality Null;
  static const Quality Good;
  static const Quality Medium;
  static const Quality Bad;
  static const unsigned int NullLevel;

  friend bool operator==(const Quality& lhs, const Quality& rhs)
  {
    return (lhs.getName() == rhs.getName() && lhs.getLevel() == rhs.getLevel());
  }
  friend bool operator!=(const Quality& lhs, const Quality& rhs) { return !operator==(lhs, rhs); }
  friend std::ostream& operator<<(std::ostream& out, const Quality& q); // output

  /**
   * \brief Checks whether this quality object is worst than another one.
   * If compared to Null it returns false.
   * @param quality
   * @return true if it is worse, false otherwise or if compared to Quality::Null.
   */
  bool isWorstThan(const Quality& quality) const { return this->mLevel > quality.getLevel(); }
  /**
   * \brief Checks whether this quality object is better than another one.
   * If compared to Null it returns false.
   * @param quality
   * @return true if it is better, false otherwise or if compared to Quality::Null.
   */
  bool isBetterThan(const Quality& quality) const { return this->mLevel < quality.getLevel(); }

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

 private:
  unsigned int mLevel; /// 0 is no quality, 1 is best quality, then it only goes downhill...
  std::string mName;
  std::map<std::string, std::string> mUserMetadata; //! no need to store it as we store it in the headers

 ClassDef(Quality, 2);
};

} // namespace o2::quality_control::core

#endif // QC_CORE_QUALITY_H
