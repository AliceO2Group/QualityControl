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

#ifndef QUALITYCONTROL_RECOREQUESTSPECS_H
#define QUALITYCONTROL_RECOREQUESTSPECS_H

///
/// \file   RecoRequestSpecs.h
/// \author Piotr Konopka
///

//#include <DetectorsBase/GRPGeomHelper.h>
#include <string>

namespace o2::quality_control::core
{

// this reflects the GRPGeomRequest struct, but allows us to gather the values and fill the input specs at different stages
struct GRPGeomRequestSpec {
  std::string geomRequest = "None";

  bool askGRPECS = false;
  bool askGRPLHCIF = false;
  bool askGRPMagField = false;
  bool askMatLUT = false;
  bool askTime = false;            // need orbit reset time for precise timestamp calculation
  bool askOnceAllButField = false; // for all entries but field query only once
  bool needPropagatorD = false;    // init also PropagatorD

  bool anyRequestEnabled() const
  {
    return geomRequest != "None" || askGRPECS || askGRPLHCIF || askGRPMagField || askMatLUT || askTime || askOnceAllButField || needPropagatorD;
  }
};

struct RecoDataRequestSpec {
};
} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_RECOREQUESTSPECS_H
