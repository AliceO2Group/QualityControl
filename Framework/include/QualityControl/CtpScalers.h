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
/// \file   CtpScalers.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_CTPSCALERS_H
#define QUALITYCONTROL_CTPSCALERS_H

#include <Rtypes.h>
#include "QualityControl/DatabaseInterface.h"

namespace o2::ctp
{
class CTPRateFetcher;
}

namespace o2::quality_control::core
{

class CtpScalers
{
 public:
  CtpScalers() = default;
  virtual ~CtpScalers() = default;

  /// \brief Call it to enable the retrieval of CTP scalers and use `getScalers` later
  void enableCtpScalers(size_t runNumber);
  /// \brief Get the scalers's value for the given source
  double getScalersValue(std::string sourceName, size_t runNumber);

  void setScalersRepo(std::shared_ptr<repository::DatabaseInterface> database)
  {
    mScalersRepo = database;
  }

 private:
  /// \brief Retrieve fresh scalers from the QCDB (with cache)
  /// \return true if success, false if failure
  bool updateScalers(size_t runNumber);

  std::shared_ptr<o2::ctp::CTPRateFetcher> mCtpFetcher;
  std::chrono::steady_clock::time_point mScalersLastUpdate;
  bool mScalersEnabled = false;
  std::shared_ptr<repository::DatabaseInterface> mScalersRepo; //! where to get the scalers from

  ClassDef(CtpScalers, 1)
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_USERCODEINTERFACE_H