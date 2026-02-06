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

#ifndef QC_MODULE_EMCAL_EMCALRAWCHECK_H
#define QC_MODULE_EMCAL_EMCALRAWCHECK_H

#include "QualityControl/CheckInterface.h"
#include "EMCAL/IndicesConverter.h"
#include <array>
#include <tuple>
#include <vector>

class TH1;
class TH2;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::emcal
{

/// \class RawCheck
/// \brief EMCAL raw data check
/// \ingroup EMCALQCCheckers
/// \author Cristina Terrevoli
/// \since March 4th, 2020
class RawCheck final : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  RawCheck() = default;
  /// Destructor
  ~RawCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;

 private:
  Quality runPedestalCheck1D(TH1* adcdist, double minentries, double signalfraction) const;
  std::vector<Quality> runPedestalCheck2D(TH2* adchist, double minentries, double signalfraction) const;
  std::array<std::vector<int>, 20> reduceBadFECs(std::vector<int> fecids) const;
  int getExepctedFECs(int smID) const;
  std::tuple<int, double> getCutsBunchMinAmpHist(bool perSM, bool perFEC);

  void loadConfigValueInt(const std::string_view configvalue, int& target);
  void loadConfigValueDouble(const std::string_view configvalue, double& target);
  void loadConfigValueBool(const std::string_view configvalue, bool& target);

  /************************************************
   * Switches for InfoLogger Messages             *
   ************************************************/
  bool mIlMessageBunchMinAmpCheckSM = false;       ///< Switch for IL message for Bunch Min. amplitude check at supermodule level
  bool mIlMessageBunchMinAmpCheckDetector = false; ///< Switch for IL message for Bunch Min. amplitude check at subdetector level
  bool mIlMessageBunchMinAmpCheckFull = false;     ///< Switch for IL message for Bunch Min. amplitude check at full detector level
  bool mILMessageRawErrorCheck = false;            ///< Switch for IL message for error code
  bool mILMessageNoisyFECCheck = false;            ///< Switch for IL message for noisy FEC
  bool mILMessagePayloadSizeCheck = false;         ///< Switch for IL message for large payload size

  /************************************************
   * Conversion between online and offline indices *
   ************************************************/
  o2::emcal::IndicesConverter mIndicesConverter; ///< Converter for online and offline supermodule indices

  /************************************************
   * sigma cuts                                   *
   ************************************************/
  double mNsigmaFECMaxPayload = 2.; ///< Number of sigmas used in the FEC max payload check
  double mNsigmaPayloadSize = 2.;   ///< Number of sigmas used in the payload size check

  /************************************************
   * Settings for bunch min. amp checker          *
   ************************************************/

  int mBunchMinCheckMinEntries = 0;            ///< Min. number of entries for bunch min. amplitude in evaluation region
  double mBunchMinCheckFractionSignal = 0.5;   ///< Fraction of entries in signal region for bunch min. amp checker
  int mBunchMinCheckMinEntriesSM = 0;          ///< Min. number of entries for bunch min. amplitude in evaluation region (SM-based histograms, optional)
  double mBunchMinCheckFractionSignalSM = 0.;  ///< Fraction of entries in signal region for bunch min. amp checker (SM-based histograms, optional)
  int mBunchMinCheckMinEntriesFEC = 0;         ///< Min. number of entries for bunch min. amplitude in evaluation region (FEC-based histograms, optional)
  double mBunchMinCheckFractionSignalFEC = 0.; ///< Fraction of entries in signal region for bunch min. amp checker (FEC-based histograms, optional)

  ClassDefOverride(RawCheck, 2);
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALRAWCHECK_H
