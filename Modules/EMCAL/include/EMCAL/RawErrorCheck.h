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
/// \file   RawErrorCheck.h
/// \author Markus Fasel
///

#ifndef QC_MODULE_EMCAL_EMCALRAWERRORCHECK_H
#define QC_MODULE_EMCAL_EMCALRAWERRORCHECK_H

#include "QualityControl/CheckInterface.h"

#include <map>

namespace o2::emcal
{
class Geometry;
}

namespace o2::quality_control_modules::emcal
{

/// \class RawErrorCheck
/// \brief Checker for histograms with error code published by the RawErrorTask
/// \author Markus Fasel
///
/// Checking for presence of an error code. Any presence of error code (non-0 entry)
/// will define data as bad.
class RawErrorCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// \brief Default constructor
  RawErrorCheck() = default;
  /// \brief Destructor
  ~RawErrorCheck() override = default;

  /// \brief Configure checker setting thresholds from taskParameters where specified
  void configure() override;

  /// \brief Check for each object whether the number of errors of type is above the expected threshold
  /// \param moMap List of histos to check
  /// \return Quality of the selection
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;

  /// \brief Beautify the monitoring objects showing the quality determination and a supermodule grid for channel based histograms
  /// \param mo Monitoring object to beautify
  /// \param checkResult Quality status of this checker
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;

  /// \brief Accept only TH2 histograms as input
  /// \return Name of the accepted object: TH2
  std::string getAcceptedType() override;

  ClassDefOverride(RawErrorCheck, 2);

 private:
  /// \brief Decode key of a configurable parameter as boolean
  /// \param value Value to be decoded (true or false, case-insensitive)
  /// \return Boolean representation of the value
  /// \throw std::runtime_error in case value is not a boolean value
  bool decodeBool(std::string value) const;

  /// \brief Find error code of the raw decoder error
  /// \param errorname Name of the raw decoder error
  /// \return Error code of the error name (-1 in case not found)
  int findErrorCodeRDE(const std::string_view errorname) const;

  /// \brief Find error code of the page error
  /// \param errorname Name of the page error
  /// \return Error code of the error name (-1 in case not found)
  int findErrorCodePE(const std::string_view errorname) const;

  /// \brief Find error code of the major ALTRO decoding error
  /// \param errorname Name of the major ALTRO decoding error
  /// \return Error code of the error name (-1 in case not found)
  int findErrorCodeMAAE(const std::string_view errorname) const;

  /// \brief Find error code of the minor ALTRO decoding error
  /// \param errorname Name of minor ALTRO decoding error
  /// \return Error code of the error name (-1 in case not found)
  int findErrorCodeMIAE(const std::string_view errorname) const;

  /// \brief Find error code of the raw fit error
  /// \param errorname Name of the raw fit error
  /// \return Error code of the error name (-1 in case not found)
  int findErrorCodeRFE(const std::string_view errorname) const;

  /// \brief Find error code of the geometry error
  /// \param errorname Name of the geometry error
  /// \return Error code of the error name (-1 in case not found)
  int findErrorCodeGEE(const std::string_view errorname) const;

  /// \brief Find error code of the gain type error
  /// \param errorname Name of the gain type error
  /// \return Error code of the error name (-1 in case not found)
  int findErrorCodeGTE(const std::string_view errorname) const;

  o2::emcal::Geometry* mGeometry;              ///< Geometry for mapping position between SM and full EMCAL
  bool mNotifyInfologger = true;               ///< Switch for notification to infologger
  std::map<int, int> mErrorCountThresholdRDE;  ///< Thresholds for Raw Decoder Error histogram
  std::map<int, int> mErrorCountThresholdPE;   ///< Thresholds for Page Error histogram
  std::map<int, int> mErrorCountThresholdMAAE; ///< Thresholds for Major ALTRO Error histogram
  std::map<int, int> mErrorCountThresholdMIAE; ///< Thresholds for Minor ALTRO Error histogram
  std::map<int, int> mErrorCountThresholdRFE;  ///< Thresholds for Raw Fit Error histogram
  std::map<int, int> mErrorCountThresholdGEE;  ///< Thresholds for Geometry Error histogram
  std::map<int, int> mErrorCountThresholdGTE;  ///< Thresholds for Gain Type Error histogram
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALRAWERRORCHECK_H
