// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   ReductorConditionAny.h
/// \author Piotr Konopka
///
#ifndef QUALITYCONTROL_REDUCTORCONDITIONANY_H
#define QUALITYCONTROL_REDUCTORCONDITIONANY_H

#include "QualityControl/Reductor.h"
#include <QualityControl/ConditionAccess.h>

namespace o2::quality_control::postprocessing
{

/// \brief An interface for storing data derived from any object into a TTree
class ReductorConditionAny : public Reductor
{
 public:
  class ConditionRetriever;

  /// \brief Constructor
  ReductorConditionAny() = default;
  /// \brief Destructor
  virtual ~ReductorConditionAny() = default;

  /// \brief A helper for the caller to have the ConditionRetriever created
  virtual bool update(core::ConditionAccess& conditionAccess, uint64_t timestamp, const std::string& path) final
  {
    ConditionRetriever retriever{ conditionAccess, timestamp, path };
    return update(retriever);
  }

  /// \brief Fill the data structure with new data
  /// \param An object getter, object presence is not guaranteed
  /// \return false if failed, true if success
  virtual bool update(ConditionRetriever& retriever) = 0;

  /// \brief A wrapper class to allow implementations of ReductorConditionAny to state the expected type of the reduced object.
  ///
  /// A wrapper class to allow implementations of ReductorConditionAny to state the expected type of the reduced object.
  /// It is declared within ReductorConditionAny, as it is intended to be used only in this context.
  struct ConditionRetriever {
   public:
    ConditionRetriever(core::ConditionAccess& conditionAccess, uint64_t timestamp, const std::string& path)
      : conditionAccess(conditionAccess), timestamp(timestamp), path(path){};
    ~ConditionRetriever() = default;

    /// \brief Gets the object with a specified type. The result can be nullptr, the pointer should not be deleted!
    template <typename T>
    T* retrieve()
    {
      return conditionAccess.retrieveConditionAny<T>(path, {}, timestamp);
    }

   private:
    core::ConditionAccess& conditionAccess;
    uint64_t timestamp;
    const std::string& path;
  };
};

} // namespace o2::quality_control::postprocessing
#endif // QUALITYCONTROL_REDUCTORCONDITIONANY_H
