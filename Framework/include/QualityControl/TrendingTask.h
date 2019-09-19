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
/// \file    TrendingTask.h
/// \author  Piotr Konopka
///

#ifndef QUALITYCONTROL_TRENDINGINTERFACE_H
#define QUALITYCONTROL_TRENDINGINTERFACE_H

#include <memory>
#include <functional>
#include <TObject.h>
#include <TGraph.h>
#include "QualityControl/PostProcessingInterface.h"

namespace o2::quality_control::repository {
class DatabaseInterface;
}
//class TGraph;

using ReduceFcn = std::function<std::pair<double, double>(TObject*)>; // value and error returned

namespace o2::quality_control::postprocessing {

class TrendingTask : public PostProcessingInterface {
  public :
  TrendingTask() = default;
  ~TrendingTask() override = default;

  void initialize(Trigger, framework::ServiceRegistry&) override;
  void update(Trigger, framework::ServiceRegistry&) override;
  void finalize(Trigger, framework::ServiceRegistry&) override;

  private:
  // maybe use merger to do the job?
//  void trend(TObject* newEntry);
  void trend();
  void store();

  std::string mStorage = "TGraph";
  std::unique_ptr<TGraph> mTrend;
  Int_t mPoints = 0;
  ReduceFcn mReduceFcn;
  repository::DatabaseInterface* mDatabase = nullptr;
};

} // namespace o2::quality_control::postprocessing


#endif //QUALITYCONTROL_TRENDINGINTERFACE_H
