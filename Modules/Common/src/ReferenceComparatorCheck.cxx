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
#include "QualityControl/ReferenceUtils.h"
#include "Common/TH1Ratio.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/RootClassFactory.h"

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

#include <Framework/ServiceRegistryRef.h>

#include <TClass.h>

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
}

void ReferenceComparatorCheck::startOfActivity(const Activity& activity)
{
  auto moduleName = mCustomParameters.atOptional("moduleName", activity).value_or("");
  auto comparatorName = mCustomParameters.atOptional("comparatorName", activity).value_or("");
  double threshold = std::stof(mCustomParameters.atOptional("threshold", activity).value_or("0"));
  mReferenceRun = std::stoi(mCustomParameters.atOptional("referenceRun", activity).value_or("0"));
  mIgnorePeriodForReference = std::stoi(mCustomParameters.atOptional("ignorePeriodForReference", activity).value_or("1")) != 0;
  mIgnorePassForReference = std::stoi(mCustomParameters.atOptional("ignorePassForReference", activity).value_or("1")) != 0;

  mComparator.reset();
  if (!moduleName.empty() && !comparatorName.empty()) {
    mComparator.reset(root_class_factory::create<ObjectComparatorInterface>(moduleName, comparatorName));
  }

  if (mComparator) {
    mComparator->setThreshold(threshold);
  }

  mReferenceActivity = activity;
  mReferenceActivity.mId = mReferenceRun;
  if (mIgnorePeriodForReference) {
    mReferenceActivity.mPeriodName = "";
  }
  if (mIgnorePassForReference) {
    mReferenceActivity.mPassName = "";
  }

  // clear the cache of reference plots
  mReferencePlots.clear();
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
    return { nullptr, nullptr };
  }
  // Get the pad containing the reference histogram.
  // This pad is only present ofr 2-D histograms.
  // 1-D histograms are drawn superimposed in the same pad
  TPad* padHistRef = (TPad*)canvas->GetPrimitive(TString::Format("%s_PadHistRef", canvas->GetName()));

  // Get the current histogram
  TH1* hist = dynamic_cast<TH1*>(padHist->GetPrimitive(TString::Format("%s_hist", canvas->GetName())));
  if (!hist) {
    message = "missing histogram";
    return { nullptr, nullptr };
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
    return { nullptr, nullptr };
  }

  // return a pair with the two histograms
  return { hist, histRef };
}

static std::pair<TH1*, TH1*> getPlotsFromCanvas(TCanvas* canvas)
{
  std::string dummyMessage;
  return getPlotsFromCanvas(canvas, dummyMessage);
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

Quality ReferenceComparatorCheck::getSinglePlotQuality(std::shared_ptr<MonitorObject> mo, std::string& message)
{
  // retrieve the reference plot and compare
  auto* th1 = dynamic_cast<TH1*>(mo->getObject());
  if (th1 == nullptr) {
    message = "The MonitorObject is not a TH1";
    return Quality::Null;
  }

  // get path of mo and ref (we have to remove the provenance)
  std::string path = RepoPathUtils::getPathNoProvenance(mo);

  if (mReferenceRun == 0) {
    message = "No reference run provided";
    return Quality::Null;
  }

  // retrieve the reference plot only once and cache it for later use
  if (mReferencePlots.count(path) == 0) {
    mReferencePlots[path] = retrieveReference(path, mReferenceActivity);
  }

  auto referencePlot = mReferencePlots[path];
  if (!referencePlot) {
    message = "Reference plot not found";
    return Quality::Null;
  }
  auto* ref = dynamic_cast<TH1*>(referencePlot->getObject());
  if (!ref) {
    message = "The reference plot is not a TH1";
    return Quality::Null;
  }
  return mComparator.get()->compare(th1, ref, message);
}

//_________________________________________________________________________________________
//
// Loop over all the input MOs and compare each of them with the corresponding MO from the reference run
// The final quality is the combination of the individual values stored in the mQualityFlags map

Quality ReferenceComparatorCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [key, mo] : *moMap) {
    auto moName = mo->getName();
    Quality quality;
    std::string message;

    // run the comparison algorithm
    if (mo->getObject()->IsA() == TClass::GetClass<TCanvas>()) {
      // We got a canvas. It contains the plot and its reference.
      auto* canvas = dynamic_cast<TCanvas*>(mo->getObject());
      quality = compare(canvas, mComparator.get(), message);
    } else if (mo->getObject()->InheritsFrom(TH1::Class())) {
      // We got a plot, we have to find the reference before calling the comparator
      quality = getSinglePlotQuality(mo, message);
    } else {
      ILOG(Warning, Ops) << "Compared Monitor Object '" << mo->getName() << "' is not a TCanvas or a TH1, the detector QC responsible should review the configuration" << ENDM;
      continue;
    }

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

static void updateQualityLabel(TPaveText* label, const Quality& quality)
{
  // draw the quality label with the text color corresponding to the quality level
  label->SetTextColor(getQualityColor(quality));
  label->AddText(quality.getName().c_str());

  // add the first flag below the quality label, or an empty line if no flags are set
  auto flags = quality.getFlags();
  std::string message = flags.empty() ? "" : flags.front().second;
  auto pos = message.find(" ");
  if (pos != std::string::npos) {
    message.erase(0, pos + 1);
  }
  auto* text = label->AddText(message.c_str());
  text->SetTextColor(kGray + 1);
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

    updateQualityLabel(label, quality);
    break;
  }
}

void ReferenceComparatorCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  // get the quality associated to the current MO
  auto moName = mo->getName();
  auto quality = mQualityFlags[moName];

  auto* canvas = dynamic_cast<TCanvas*>(mo->getObject());
  if (canvas) {
    // retrieve the reference plot from the canvas and set the line color according to the quality
    auto plots = getPlotsFromCanvas(canvas);
    if (plots.second) {
      plots.second->SetLineColor(getQualityColor(quality));
    }

    // draw the quality label on the plot
    setQualityLabel(canvas, quality);
  }
  auto* th1 = dynamic_cast<TH1*>(mo->getObject());
  if (th1) {
    auto* qualityLabel = new TPaveText(0.75, 0.65, 0.98, 0.75, "brNDC");
    updateQualityLabel(qualityLabel, quality);
    th1->GetListOfFunctions()->Add(qualityLabel);
  }
}

} // namespace o2::quality_control_modules::common
