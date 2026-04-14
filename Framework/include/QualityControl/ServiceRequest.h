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
/// \file   ServiceRequest.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_SERVICEREQUEST_H
#define QUALITYCONTROL_SERVICEREQUEST_H

namespace o2::quality_control::core
{

/// \brief Used to specify which services are needed by a concrete Actor
enum class ServiceRequest {
  Monitoring,
  InfoLogger,
  CCDB,
  Bookkeeping,
  QCDB
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_SERVICEREQUEST_H