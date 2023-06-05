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
/// \file   RawErrorTask.h
/// \author Cristina Terrevoli
//  \author Markus Fasel
///

#ifndef QC_MODULE_EMCAL_EMCALRAWERRORTASK_H
#define QC_MODULE_EMCAL_EMCALRAWERRORTASK_H

#include "QualityControl/TaskInterface.h"
#include <array>
#include <memory>
#include <string_view>

class TH2;
class TH1;

using namespace o2::quality_control::core;
namespace o2::emcal
{
class Geometry;
class MappingHandler;
} // namespace o2::emcal
namespace o2::quality_control_modules::emcal
{

/// \brief Example Quality Control DPL Task
/// \author My Name
class RawErrorTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  RawErrorTask() = default;
  /// Destructor
  ~RawErrorTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  std::string getConfigValue(const std::string_view key);
  std::string getConfigValueLower(const std::string_view key);

  TH2* mErrorTypeAll = nullptr;         ///< Base histogram any raw data error
  TH2* mErrorTypeAltro = nullptr;       ///< Major ALTRO payload decoding errors
  TH2* mErrorTypePage = nullptr;        ///< DMA page decoding errors
  TH2* mErrorTypeMinAltro = nullptr;    ///< Minor ALTRO payload decoding errors
  TH2* mErrorTypeFit = nullptr;         ///< Raw fit errors
  TH2* mErrorTypeGeometry = nullptr;    ///< Geometry errors
  TH2* mErrorTypeGain = nullptr;        ///< Gain type errors
  TH1* mErrorTypeUnknown = nullptr;     ///< Counter of defined error codes
  TH2* mErrorGainLow = nullptr;         ///< FEC with LGnoHG error
  TH2* mErrorGainHigh = nullptr;        ///< FEC with HGnoLG error
  TH2* mChannelGainLow = nullptr;       ///< Tower with LGnoHG error
  TH2* mChannelGainHigh = nullptr;      ///< Tower with HGnoLG error
  TH2* mFecIdMinorAltroError = nullptr; ///< Minor Altro Error  per DDL

  bool mExcludeGainErrorsFromOverview; ///< exclude gain error from global overview panel

  o2::emcal::Geometry* mGeometry = nullptr;           ///< EMCAL geometry
  std::unique_ptr<o2::emcal::MappingHandler> mMapper; ///< EMCAL mapper
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALRAWERRORTASK_H
