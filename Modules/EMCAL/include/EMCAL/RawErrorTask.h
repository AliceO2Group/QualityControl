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
#include <string_view>

class TH2;
class TH1;

using namespace o2::quality_control::core;
namespace o2::emcal
{
class Geometry;
}
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
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  TH2* mErrorTypeAltro = nullptr;    ///< Error from
  TH2* mErrorTypePage = nullptr;     ///< Error from
  TH2* mErrorTypeMinAltro = nullptr; ///< Error from
  TH2* mErrorTypeFit = nullptr;      ///< Error from
  TH2* mErrorTypeGeometry = nullptr; ///< Error from
  TH2* mErrorTypeGain = nullptr;     ///< Error from

  o2::emcal::Geometry* mGeometry = nullptr; ///< EMCAL geometry
};

} // namespace o2::quality_control_modules::emcal

#endif // QC_MODULE_EMCAL_EMCALRAWERRORTASK_H
