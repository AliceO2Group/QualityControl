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
/// \file   Reductor.h
/// \author Piotr Konopka
///
#ifndef QUALITYCONTROL_REDUCTOR_H
#define QUALITYCONTROL_REDUCTOR_H

#include <TObject.h>

namespace o2::quality_control::postprocessing
{

/// \brief An interface for storing data derived from QC objects into a TTree
class Reductor
{
 public:
  /// \brief Constructor
  Reductor() = default;
  /// \brief Destructor
  virtual ~Reductor() = default;

  /// \brief Branch address getter
  /// \return A pointer to a structure/variable which will be used to fill a TTree. It must not change later!
  virtual void* getBranchAddress() = 0;
  /// \brief Branch leaf list getter
  /// \return A C string with a description of a branch format, formatted accordingly to the TTree interface
  virtual const char* getBranchLeafList() = 0;
  /// \brief Fill the data structure with new data
  /// \param An object to be reduced
  virtual void update(TObject* obj) = 0;
};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_REDUCTOR_H
