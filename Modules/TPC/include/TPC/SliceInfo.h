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
/// \file     SliceInfo.h
/// \author   Marcel Lesch
/// \author   Cindy Mordasini
///

#ifndef QUALITYCONTROL_SLICEINFO_H
#define QUALITYCONTROL_SLICEINFO_H

#include "QualityControl/QcInfoLogger.h"
#include <Rtypes.h>
#include <string>
#include <sstream>

namespace o2::quality_control_modules::tpc
{
/// \brief  Structure for the reductor quantites for a single pad of the TPC.
///
/// Structure gathering all the reductor quantities related to the trending of
/// the 'pads' (ROCs, sectors, slices,...) of the TPC. The reductor receives a
/// vector of SliceInfo with one element per slice, and fills it accordingly to
/// the json configuration.
///
/// \author Marcel Lesch
/// \author Cindy Mordasini

struct SliceInfo {
  double entries;  // Number of entries in the slice/canvas.
  double meanX;    // Standard mean for a given range in X.
  double stddevX;  // Standard deviation for the range in X.
  double errMeanX; // Error on the mean along X.
  double meanY;    // Standard mean in Y.
  double stddevY;  // Standard deviation in Y.
  double errMeanY; // Error on the mean along Y.

  /// \brief Check if the argument is a floating number or a string.
  bool isStringFloating(std::string var)
  {
    std::istringstream iss(var);
    float f;
    iss >> std::noskipws >> f;
    return iss.eof() && !iss.fail();
  }

  /// \brief Return the struct member/the float corresponding to the argument.
  double RetrieveValue(std::string VarType)
  {
    if (isStringFloating(VarType)) {
      return std::stod(VarType);
    } else {
      if (strcmp(VarType.data(), "entries") == 0) {
        return entries;
      } else if (strcmp(VarType.data(), "meanX") == 0) {
        return meanX;
      } else if (strcmp(VarType.data(), "stddevX") == 0) {
        return stddevX;
      } else if (strcmp(VarType.data(), "errMeanX") == 0) {
        return errMeanX;
      } else if (strcmp(VarType.data(), "meanY") == 0) {
        return meanY;
      } else if (strcmp(VarType.data(), "stddevY") == 0) {
        return stddevY;
      } else if (strcmp(VarType.data(), "errMeanY") == 0) {
        return errMeanY;
      } else {
        ILOG(Error, Support) << "TPC SliceInfo.h: 'VarType' " << VarType.data()
                             << " in 'RetrieveValue' unknown. Breaking." << ENDM;
        exit(0);
      }
    }
  }

  ClassDefNV(SliceInfo, 1);
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_SLICEINFO_H
