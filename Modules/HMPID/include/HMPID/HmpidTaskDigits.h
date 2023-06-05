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
/// \file       HmpidTaskDigits.h
/// \author     Antonio Paz, Giacomo Volpe
/// \brief      Class to map data from HMPID detectors
/// \version    0.3.1
/// \date       26/10/2021
///

#ifndef QC_MODULE_HMPID_HMPIDHMPIDTASKDIGITS_H
#define QC_MODULE_HMPID_HMPIDHMPIDTASKDIGITS_H

#include "QualityControl/TaskInterface.h"

class TH1F;
class TH2F;
class TProfile;
// class TProfile2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::hmpid
{

class HmpidTaskDigits final : public TaskInterface
{
 public:
  /// \brief Constructor
  HmpidTaskDigits() = default;
  /// Destructor
  ~HmpidTaskDigits() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  static const Int_t numCham = 7;
  static const Int_t numEquip = 14;
  const Double_t factor = 11520.; //! ap   11520= 24*10*48

  TH1F* hHMPIDchargeDist[numCham];  // histogram of charges per chamber
  TH2F* hHMPIDdigitmapAvg[numCham]; // Map of hits per coordinate

  // TH1F* hOccupancyI;     // histogram of occupancy per data link
  TProfile* hOccupancyAvg; // average occupancy per data link
};

} // namespace o2::quality_control_modules::hmpid

#endif // QC_MODULE_HMPID_HMPIDHMPIDTASK_H
