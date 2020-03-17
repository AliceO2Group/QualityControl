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
/// \file   MCHCheckPedestals.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MCH_MCHCHECKPEDESTALS_H
#define QC_MODULE_MCH_MCHCHECKPEDESTALS_H

#include "QualityControl/CheckInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

namespace o2::quality_control_modules::muonchambers
{

/// \brief  Check whether a plot is empty or not.
///
/// \author Barthelemy von Haller
class MCHCheckPedestals : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  MCHCheckPedestals();
  /// Destructor
  ~MCHCheckPedestals() override;

  // Override interface
  void configure(std::string name) override;
  virtual Quality check(const MonitorObject* mo);
  virtual void beautify(MonitorObject* mo, Quality checkResult = Quality::Null);
  std::string getAcceptedType() override;

  /// Minimum value for SAMPA pedestals
  Float_t minMCHpedestal;
  /// Maximum value for SAMPA pedestals
  Float_t maxMCHpedestal;

 private:
    
    /// Vector filled with DualSampas Ids that have been tested but sent back no data
    std::vector<int> missing;
    
  ClassDefOverride(MCHCheckPedestals, 1);
  
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_TOF_TOFCHECKRAWSTIME_H
