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
/// \file   ReferenceComparatorCheck.cxx
/// \author Andrea Ferrero
///

#include "Common/ReferenceComparatorCheck.h"
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/RootClassFactory.h"

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

// ROOT
#include <TH1.h>
#include <TCanvas.h>
#include <TPaveText.h>
#include <TColor.h>

using namespace o2::quality_control;

namespace o2::quality_control_modules::common
{

void ReferenceComparatorCheck::configure()
{
  auto moduleName = mCustomParameters.atOptional("moduleName").value_or("");
  auto comparatorName = mCustomParameters.atOptional("comparatorName").value_or("");
  mComparator.reset(root_class_factory::create<ObjectComparatorInterface>(moduleName, comparatorName));
}

void ReferenceComparatorCheck::startOfActivity(const Activity& activity)
{
  if (mComparator) {
    mComparator->configure(mCustomParameters, activity);
  }
}

void ReferenceComparatorCheck::endOfActivity(const Activity& activity)
{
}

//_________________________________________________________________________________________
//
// Get the current and reference histograms from the canvas.
// The two histograms are returned as a std::pair
static std::pair<TH1*, TH1*> getPlotsFromCanvas(TCanvas* canvas, std::string& message)
{
  // Get the pad containing the current histogram, as well as the reference one in the case of 1-D plots
  TPad* padHist = (TPad*)canvas->GetPrimitive(TString::Format("%s_PadHist", canvas->GetName()));
  if (!padHist) {
    message = "missing PadHist";
    return {nullptr, nullptr};
  }
  // Get the pad containing the reference histogram.
  // This pad is only present ofr 2-D histograms.
  // 1-D histograms are drawn superimposed in the same pad
  TPad* padHistRef = (TPad*)canvas->GetPrimitive(TString::Format("%s_PadHistRef", canvas->GetName()));

  // Get the current histogram
  TH1* hist = dynamic_cast<TH1*>(padHist->GetPrimitive(TString::Format("%s_hist", canvas->GetName())));
  if (!hist) {
    message = "missing histogram";
    return {nullptr, nullptr};
  }

  // Get the reference histogram, trying both pads
  TH1* histRef = nullptr;
  if (padHistRef) {
    histRef = dynamic_cast<TH1*>(padHistRef->GetPrimitive(TString::Format("%s_hist_ref", canvas->GetName())));
  } else {
    histRef = dynamic_cast<TH1*>(padHist->GetPrimitive(TString::Format("%s_hist_ref", canvas->GetName())));
  }

  if (!histRef) {
    message = "missing reference histogram";
    return {nullptr, nullptr};
  }

  // return a pair with the two histograms
  return {hist, histRef};
}

// Get the current and reference histograms from the canvas, and compare them using the comparator object passed as parameter
static Quality compare(TCanvas* canvas, ObjectComparatorInterface* comparator, std::string& message)
{
  if (!canvas) {
    message = "missing canvas";
    return Quality::Null;
  }
  if (!comparator) {
    message = "missing comparator";
    return Quality::Null;
  }

  // extract the histograms from the canvas
  auto plots = getPlotsFromCanvas(canvas, message);
  if (!plots.first || !plots.second) {
    return Quality::Null;
  }

  // Return the result of the comparison. Details are stored in the "message" string.
  return comparator->compare(plots.first, plots.second, message);
}

//_________________________________________________________________________________________
//
// Get the THxyRatio histogram from the MO, and compare the current histogram (numerator) with
// the reference histogram (denominator), using the comparator object passed as parameter

/*static Quality compare(std::shared_ptr<MonitorObject> mo, ObjectComparatorInterface* comparator, std::string& message)
{
  if (!mo) {
    message = "missing mo";
    return Quality::Null;
  }
  if (!comparator) {
    message = "missing comparator";
    return Quality::Null;
  }

  auto object = mo->getObject();
  if (!object) {
    message = "missing object";
    return Quality::Null;
  }

  // check if the MO contains an histogram derived from THxRatio
  if (auto histRatio = dynamic_cast<TH1FRatio*>(object)) {
    return comparator->compare(histRatio->getNum(), histRatio->getDen(), message);
  } else if (auto histRatio = dynamic_cast<TH1DRatio*>(object)) {
    return comparator->compare(histRatio->getNum(), histRatio->getDen(), message);
  } else if (auto histRatio = dynamic_cast<TH2FRatio*>(object)) {
    return comparator->compare(histRatio->getNum(), histRatio->getDen(), message);
  } else if (auto histRatio = dynamic_cast<TH2DRatio*>(object)) {
    return comparator->compare(histRatio->getNum(), histRatio->getDen(), message);
  }

  message = "object is not a TH*Ratio";
  return Quality::Null;
}

static bool isRatio(const std::string& name)
{
  static const std::string suffix = "Ratio";
  bool result = false;
  auto pos = name.rfind(suffix);
  if (pos == (name.size() - suffix.size())) {
    result = true;
  }
  std::cout << "TOTO3 name '" << name << "'  size " << name.size() << "  pos " << pos << "  result " << result << std::endl;
  return result;
}*/

//_________________________________________________________________________________________
//
// Loop over all the input MOs and compare each of them with the corresponding MO from the reference run
// The final quality is the combination of the individual values stored in the mQualityFlags map

Quality ReferenceComparatorCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Good;
  for (auto& [key, mo] : *moMap) {
    auto moName = mo->getName();

    auto* canvas = dynamic_cast<TCanvas*>(mo->getObject());
    std::cout << "TOTO3 moName '" << moName << "  canvas " << canvas << std::endl;
    if (!canvas) {
      continue;
    }

    Quality quality;
    std::string message;
    quality = compare(canvas, mComparator.get(), message);
    std::cout << "TOTO3 " << message << std::endl;

    // update the overall quality
    if (quality.isWorseThan(result)) {
      result.set(quality);
    }

    // add the message if not empty
    if (!message.empty()) {
      quality.addFlag(FlagTypeFactory::Unknown(), fmt::format("{} {}", moName, message));
      result.addFlag(FlagTypeFactory::Unknown(), fmt::format("{} {}", moName, message));
    }

    // store the quality associated to this MO
    // will be used to beautify the corresponding plots
    mQualityFlags[moName] = quality;
  }

  return result;
}

void ReferenceComparatorCheck::reset()
{
  mQualityFlags.clear();
}

std::string ReferenceComparatorCheck::getAcceptedType() { return "TH1"; }

// return the ROOT color index associated to a give quality level
static int getQualityColor(const Quality& q)
{
  if (q == Quality::Null)
    return kViolet - 6;
  if (q == Quality::Bad)
    return kRed;
  if (q == Quality::Medium)
    return kOrange - 3;
  if (q == Quality::Good)
    return kGreen + 2;

  return 0;
}

// Write the quality level and flags in the existing PaveText inside the canvas
static void setQualityLabel(TCanvas* canvas, const Quality& quality)
{
  if (!canvas) {
    return;
  }

  canvas->cd();

  // search for the label to show the quality status
  TIter next(canvas->GetListOfPrimitives());
  TObject* obj;
  while ((obj = next())) {
    auto* label = dynamic_cast<TPaveText*>(obj);
    if (!label) {
      continue;
    }

    label->SetTextColor(getQualityColor(quality));
    label->AddText(quality.getName().c_str());

    auto flags = quality.getFlags();
    for (auto& flag : flags) {
      auto message = flag.second;
      auto pos = message.find(" ");
      if (pos != std::string::npos) {
        message.erase(0, pos + 1);
      }
      auto* text = label->AddText(message.c_str());
      text->SetTextColor(kGray + 1);
    }
    break;
  }
}

void ReferenceComparatorCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* canvas = dynamic_cast<TCanvas*>(mo->getObject());
  if (!canvas) {
    return;
  }

  auto moName = mo->getName();
  auto quality = mQualityFlags[moName];

  std::string dummyMessage;
  auto plots = getPlotsFromCanvas(canvas, dummyMessage);

  if (plots.second) {
    plots.second->SetLineColor(getQualityColor(quality));
  }

  setQualityLabel(canvas, quality);
}

} // namespace o2::quality_control_modules::common
