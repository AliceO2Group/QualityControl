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
/// \file   OutOfBunchCollTask.h
/// \author Sebastian Bysiak sbysiak@cern.ch
///

#ifndef QC_MODULE_FT0_OUTOFBUNCHCOLLTASK_H
#define QC_MODULE_FT0_OUTOFBUNCHCOLLTASK_H

#include "CommonDataFormat/BunchFilling.h"
#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/DatabaseInterface.h"
#include "FT0Base/Constants.h"
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include "CCDB/CcdbApi.h"

#include "TList.h"
#include "Rtypes.h"

class TH1F;
class TH2F;

namespace o2::quality_control_modules::ft0
{

/// \brief PostProcessing task which finds collisions not compatible with BC pattern
/// \author Sebastian Bysiak sbysiak@cern.ch
class OutOfBunchCollTask final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  OutOfBunchCollTask() = default;
  ~OutOfBunchCollTask() override;
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  void configure(std::string, const boost::property_tree::ptree&) override;

 private:
  std::string mPathDigitQcTask;
  std::string mPathBunchFilling;
  o2::quality_control::repository::DatabaseInterface* mDatabase = nullptr;
  std::string mCcdbUrl;
  o2::ccdb::CcdbApi mCcdbApi;
  TList* mListHistGarbage;
  std::map<int, std::string> mMapDigitTrgNames;
  std::map<unsigned int, TH2F*> mMapOutOfBunchColl;
  // if storage size matters it can be replaced with TH1
  // and TH2 can be created based on it on the fly, but only TH1 would be stored
  std::unique_ptr<TH2F> mHistBcPattern;
};

} // namespace o2::quality_control_modules::ft0

#endif //QC_MODULE_FT0_OUTOFBUNCHCOLLTASK_H
