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
/// \file   ObjectMetadataKeys.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_OBJECTMETADATAKEYS_H
#define QUALITYCONTROL_OBJECTMETADATAKEYS_H

/// \brief Definitions of keys for metadata stored in the repository
namespace o2::quality_control::repository::metadata_keys
{

// CCDB
constexpr auto validFrom = "Valid-From";
constexpr auto validUntil = "Valid-Until";
constexpr auto created = "Created";
constexpr auto md5sum = "Content-MD5";
constexpr auto objectType = "ObjectType";
constexpr auto lastModified = "lastModified";
// General QC framework
constexpr auto qcVersion = "qc_version";
constexpr auto qcDetectorCode = "qc_detector_name";
constexpr auto qcTaskName = "qc_task_name";
constexpr auto qcTaskClass = "qc_task_class";
constexpr auto qcQuality = "qc_quality";
constexpr auto qcCheckName = "qc_check_name";
constexpr auto qcTRFCName = "qc_trfc_name";
constexpr auto qcAdjustableEOV = "adjustableEOV"; // this is a keyword for the CCDB
// QC Activity
constexpr auto runType = "RunType";
constexpr auto runNumber = "RunNumber";
constexpr auto passName = "PassName";
constexpr auto periodName = "PeriodName";
constexpr auto beamType = "BeamType";

} // namespace o2::quality_control::repository::metadata_keys

#endif // QUALITYCONTROL_OBJECTMETADATAKEYS_H
