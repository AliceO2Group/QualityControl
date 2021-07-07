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
/// \file   ReductorTPC.h
/// \author Marcel Lesch
/// \author Cindy Mordasini
/// \author Based on the work from Piotr Konopka
///
#ifndef QUALITYCONTROL_REDUCTORTPC_H
#define QUALITYCONTROL_REDUCTORTPC_H

#include <TObject.h>
#include <vector>

namespace o2::quality_control_modules::tpc
{

/// \brief An interface for storing data derived from QC objects into a TTree
///
/// A TPC-specific reductor class from which each reductor used for the trending of the TPC-related quantities inherit.
class ReductorTPC
{
 public:
  /// \brief Constructor
  ReductorTPC() = default;
  /// \brief Destructor
  virtual ~ReductorTPC() = default;

  /// \brief Branch address getter
  /// \return A pointer to a structure/variable which will be used to fill a TTree. It must not change later!
  virtual void* getBranchAddress() = 0;
  /// \brief Branch leaf list getter
  /// \return A C string with a description of a branch format, formatted accordingly to the TTree interface
  virtual const char* getBranchLeafList() = 0;
  /// \brief Fill the data structure with new data
  /// \param An object to be reduced
  virtual void update(TObject* obj, std::vector<std::vector<float>>& axis, bool isCanvas) = 0;
};

} // namespace o2::quality_control_modules::tpc

#endif //QUALITYCONTROL_REDUCTORTPC_H
