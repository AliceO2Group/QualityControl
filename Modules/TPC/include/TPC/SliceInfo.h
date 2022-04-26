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
/// \brief  Structure for the reductor quantities for a single pad of the TPC.
///
/// Structure gathering all the reductor quantities related to the trending of
/// the 'pads' (ROCs, sectors, slices,...) of the TPC. The reductor receives a
/// vector of SliceInfo with one element per slice, and fills it accordingly to
/// the json configuration.
///

struct SliceInfo {
  double entries = 0.;     // Number of entries in the slice/canvas.
  double meanX = 0.;       // Standard mean for a given range in X.
  double stddevX = 0.;     // Standard deviation for the range in X.
  double errMeanX = 0.;    // Error on the mean along X.
  double meanY = 0.;       // Standard mean in Y.
  double stddevY = 0.;     // Standard deviation in Y.
  double errMeanY = 0.;    // Error on the mean along Y.
  double sliceLabelX = 0.; // Stores numerical center of slice along X in case of slicing or pad number in case of canvas
  double sliceLabelY = 0.; // Stores numerical center of slice along Y in case of slicing or pad number in case of canvas

  /// \brief Check if the argument is a floating number or a string.
  bool isStringFloating(std::string var)
  {
    std::istringstream iss(var);
    float f;
    iss >> std::noskipws >> f;
    return iss.eof() && !iss.fail();
  }

  /// \brief Return the struct member/float corresponding to the argument.
  double RetrieveValue(std::string varType)
  {
    if (isStringFloating(varType)) {
      return std::stod(varType);
    } else {
      if (varType == "entries") {
        return entries;
      } else if (varType == "meanX") {
        return meanX;
      } else if (varType == "stddevX") {
        return stddevX;
      } else if (varType == "errMeanX") {
        return errMeanX;
      } else if (varType == "meanY") {
        return meanY;
      } else if (varType == "stddevY") {
        return stddevY;
      } else if (varType == "errMeanY") {
        return errMeanY;
      } else if (varType == "sliceLabelX") {
        return sliceLabelX;
      } else if (varType == "sliceLabelY") {
        return sliceLabelY;
      } else {
        ILOG(Error, Support) << "TPC SliceInfo.h: 'varType' " << varType.data()
                             << " in 'RetrieveValue' unknown. Breaking." << ENDM;
        exit(0);
      }
    }
  }

  ClassDefNV(SliceInfo, 1);
};

struct SliceInfoQuality {
  UInt_t qualitylevel = 0;

  /// \brief Return the struct member/float corresponding to the argument.
  double RetrieveValue(std::string varType)
  {
    if (varType == "qualitylevel") {
      return (double)qualitylevel;
    } else {
      ILOG(Error, Support) << "TPC SliceInfoQuality: 'varType' " << varType.data()
                           << " in 'RetrieveValue' unknown. Breaking." << ENDM;
      exit(0);
    }
  }

  ClassDefNV(SliceInfoQuality, 1);
};

} // namespace o2::quality_control_modules::tpc

#endif // QUALITYCONTROL_SLICEINFO_H
