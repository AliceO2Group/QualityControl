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
  virtual ~Quality();

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

 private:
  unsigned int mLevel; /// 0 is no quality, 1 is best quality, then it only goes downhill...
  std::string mName;

  ClassDef(Quality, 1);
};

} // namespace o2::quality_control::core

#endif // QC_CORE_QUALITY_H
