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
/// \file   SkeletonCheck.cxx
/// \author My Name
///

#include "Skeleton/SkeletonCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "Skeleton/SkeletonTask.h"
#include "QualityControl/Data.h"
#include "QualityControl/DataAdapters.h"
// ROOT
#include <TH1.h>

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::skeleton
{

void SkeletonCheck::configure()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  // This method is called whenever CustomParameters are set.

  // Example of retrieving a custom parameter
  std::string parameter = mCustomParameters.atOrDefaultValue("myOwnKey1", "default");
}

Quality SkeletonCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  auto data = createData(*moMap);
  return check(data);
}

Quality SkeletonCheck::check(const quality_control::core::QCInputs& data)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  Quality result = Quality::Null;

  // You can get details about the activity via the object mActivity:
  ILOG(Debug, Devel) << "Run " << mActivity->mId << ", type: " << mActivity->mType << ", beam: " << mActivity->mBeamType << ENDM;
  // and you can get your custom parameters:
  ILOG(Debug, Devel) << "custom param physics.pp.myOwnKey1 : " << mCustomParameters.atOrDefaultValue("myOwnKey1", "default_value", "physics", "pp") << ENDM;

  constexpr static auto name = "example";
  // get MonitorObject with a given name from generic data object and converts it into requested type (TH1 here)
  const auto histOpt = getMonitorObject<TH1>(data, name);
  if (!histOpt.has_value()) {
    ILOG(Warning, Support) << "Data object does not contain any MonitorObject with a name: " << name << ", or it couldn't be transformed into TH1" << ENDM;
    return result;
  }

  const TH1& histogram = histOpt.value().get();
  // unless we find issues, we assume the quality is good
  result = Quality::Good;

  // an example of a naive quality check: we want bins 1-7 to be non-empty and bins 0 and >7 to be empty.
  for (int i = 0; i < histogram.GetNbinsX(); i++) {
    if (i > 0 && i < 8 && histogram.GetBinContent(i) == 0) {
      result = Quality::Bad;
      // optionally, we can add flags indicating the effect on data and a comment explaining why it was assigned.
      result.addFlag(FlagTypeFactory::BadPID(), "It is bad because there is nothing in bin " + std::to_string(i));
      break;
    } else if ((i == 0 || i > 7) && histogram.GetBinContent(i) > 0) {
      result = Quality::Medium;
      // optionally, we can add flags indicating the effect on data and a comment explaining why it was assigned.
      result.addFlag(FlagTypeFactory::Unknown(), "It is medium because bin " + std::to_string(i) + " is not empty");
      result.addFlag(FlagTypeFactory::BadTracking(), "We can assign more than one Flag to a Quality");
    }
  }
  // optionally, we can associate some custom metadata to a Quality
  result.addMetadata("mykey", "myvalue");

  return result;
}

void SkeletonCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.

  // This method lets you decorate the checked object according to the computed Quality
  if (mo->getName() == "example") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Error, Support) << "Could not cast `example` to TH1*, skipping" << ENDM;
      return;
    }

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red" << ENDM;
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange" << ENDM;
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

void SkeletonCheck::reset()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "SkeletonCheck::reset" << ENDM;
  // please reset the state of the check here to allow for reuse between consecutive runs.
}

void SkeletonCheck::startOfActivity(const Activity& activity)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "SkeletonCheck::start : " << activity.mId << ENDM;
  mActivity = make_shared<Activity>(activity);
}

void SkeletonCheck::endOfActivity(const Activity& activity)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "SkeletonCheck::end : " << activity.mId << ENDM;
}

} // namespace o2::quality_control_modules::skeleton
