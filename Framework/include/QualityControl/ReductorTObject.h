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
/// \file   ReductorTObject.h
/// \author Piotr Konopka
///
#ifndef QUALITYCONTROL_REDUCTORTOBJECT_H
#define QUALITYCONTROL_REDUCTORTOBJECT_H

#include "QualityControl/Reductor.h"
#include <RtypesCore.h>

class TObject;

namespace o2::quality_control::postprocessing
{

/// \brief An interface for storing data derived from TObjects into a TTree
class ReductorTObject : public Reductor
{
 public:
  /// \brief Constructor
  ReductorTObject() = default;
  /// \brief Destructor
  virtual ~ReductorTObject() = default;

  /// \brief Fill the data structure with new data
  /// \param An object to be reduced into a limited set of observables
  virtual void update(TObject* obj) = 0;
};

} // namespace o2::quality_control::postprocessing
#endif // QUALITYCONTROL_REDUCTORTOBJECT_H
