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
/// \file    ReferenceComparatorTask.xx
/// \author  Andrea Ferrero
/// \brief   Post-processing task that compares a given set of plots with reference ones
///

#include "Common/ReferenceComparatorTask.h"
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/ActivityHelpers.h"
// ROOT
#include <TClass.h>
#include <TH1.h>
#include <TH2.h>
#include <TCanvas.h>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;
using namespace o2::quality_control;

namespace o2::quality_control_modules::common
{

static bool splitObjectPath(std::string fullPath, std::string& path, std::string& name)
{
  std::string delimiter = "/";
  std::string det;
  size_t pos = fullPath.rfind(delimiter);
  if (pos == std::string::npos) {
    return false;
  }
  path = fullPath.substr(0, pos);
  name = fullPath.substr(pos + 1);
  return true;
}

//_________________________________________________________________________________________
//
// Helper function for retrieving the last MonitorObject for a give run number

static std::shared_ptr<MonitorObject> getMOFromRun(repository::DatabaseInterface* qcdb, std::string fullPath, uint32_t run, Activity activity)
{
  uint64_t timeStamp = 0;
  activity.mId = run;
  const auto filterMetadata = activity_helpers::asDatabaseMetadata(activity, false);
  const auto objectValidity = qcdb->getLatestObjectValidity(activity.mProvenance + "/" + fullPath, filterMetadata);
  if (objectValidity.isValid()) {
    timeStamp = objectValidity.getMax() - 1;
  } else {
    ILOG(Warning, Devel) << "Could not find the object '" << fullPath << "' for run " << activity.mId << ENDM;
    return nullptr;
  }

  std::string path;
  std::string name;
  if (!splitObjectPath(fullPath, path, name)) {
    return nullptr;
  }
  // retrieve MO from CCDB
  auto mo = qcdb->retrieveMO(path, name, timeStamp, activity);

  return mo;
}

//_________________________________________________________________________________________
// Helper function for retrieving a MonitorObject from the QCDB, in the form of a std::pair<std::shared_ptr<MonitorObject>, bool>
// A non-null MO is returned in the first element of the pair if the MO is found in the QCDB
// The second element of the pair is set to true if the MO has a time stamp more recent than a user-supplied threshold

static std::pair<std::shared_ptr<MonitorObject>, bool> getMO(repository::DatabaseInterface& qcdb, std::string fullPath, Trigger t, long notOlderThan)
{
  // find the time-stamp of the most recent object matching the current activity
  // if ignoreActivity is true the activity matching criteria are not applied
  auto timestamp = t.timestamp;
  const auto filterMetadata = activity_helpers::asDatabaseMetadata(t.activity, false);
  const auto objectValidity = qcdb.getLatestObjectValidity(t.activity.mProvenance + "/" + fullPath, filterMetadata);
  if (objectValidity.isValid()) {
    timestamp = objectValidity.getMax() - 1;
  } else {
    ILOG(Warning, Devel) << "Could not find the object '" << fullPath << "' for activity " << t.activity << ENDM;
    return { nullptr, false };
  }

  std::string path;
  std::string name;
  if (!splitObjectPath(fullPath, path, name)) {
    return { nullptr, false };
  }
  // retrieve QO from CCDB - do not associate to trigger activity if ignoreActivity is true
  auto qo = qcdb.retrieveMO(path, name, timestamp, t.activity);
  if (!qo) {
    return { nullptr, false };
  }

  long elapsed = static_cast<long>(t.timestamp) - timestamp;
  // check if the object is not older than a given number of milliseconds
  if (elapsed > notOlderThan) {
    ILOG(Warning, Devel) << "Object '" << fullPath << "' for activity " << t.activity << " is too old: " << elapsed << " > " << notOlderThan << ENDM;
    return { qo, false };
  }

  return { qo, true };
}

//_________________________________________________________________________________________
// Helper functions for retrieving configuration parameters as text strings

template <>
std::string ReferenceComparatorTask::getParameter(std::string parName, const std::string defaultValue, const o2::quality_control::core::Activity& activity)
{
  std::string result = defaultValue;
  auto parOpt = mCustomParameters.atOptional(parName, activity);
  if (parOpt.has_value()) {
    result = parOpt.value();
  }
  return result;
}

template <>
std::string ReferenceComparatorTask::getParameter(std::string parName, const std::string defaultValue)
{
  std::string result = defaultValue;
  auto parOpt = mCustomParameters.atOptional(parName);
  if (parOpt.has_value()) {
    result = parOpt.value();
  }
  return result;
}

//_________________________________________________________________________________________
//
// Get the reference plot for a given MonitorObject path

std::shared_ptr<MonitorObject> ReferenceComparatorTask::getRefPlot(repository::DatabaseInterface& qcdb, std::string fullPath, Activity activity)
{
  return getMOFromRun(&qcdb, fullPath, mRefRun, activity);
}

//_________________________________________________________________________________________

void ReferenceComparatorTask::configure(const boost::property_tree::ptree& config)
{
  mConfig = ReferenceComparatorTaskConfig(getID(), config);
}

//_________________________________________________________________________________________
//
// Helper functions to initialize the task histograms, depending on the type of the source histograms

template <class HIST, class HISTRATIO>
void ReferenceComparatorTask::initHistograms1D(TObject* refObject, const ReferenceComparatorTaskConfig::DataGroup& group, std::string path)
{
  HIST* refHist = dynamic_cast<HIST*>(refObject);
  if (!refHist) {
    return;
  }

  auto fullPath = group.inputPath + "/" + path;
  auto fullOutPath = group.outputPath + "/" + path;

  mHistogramRatios[fullPath] = std::make_unique<HISTRATIO>((fullOutPath + "_RefRatio").c_str(),
      refHist->GetTitle(),
      refHist->GetXaxis()->GetNbins(),
      refHist->GetXaxis()->GetXmin(),
      refHist->GetXaxis()->GetXmax(),
      false);
  HISTRATIO* hRatio = dynamic_cast<HISTRATIO*>(mHistogramRatios[fullPath].get());
  hRatio->GetXaxis()->SetTitle(refHist->GetXaxis()->GetTitle());
  hRatio->GetYaxis()->SetTitle("histogram / reference");

  hRatio->SetStats(0);
  hRatio->SetOption("HIST");
  getObjectsManager()->startPublishing(hRatio);
  getObjectsManager()->setDefaultDrawOptions(hRatio, "HIST");

  auto& plotWithRef = mHistograms[fullPath];

  plotWithRef.mPlot = std::make_shared<TH1F>((fullOutPath + "_hist").c_str(),
      refHist->GetTitle(),
      refHist->GetXaxis()->GetNbins(),
      refHist->GetXaxis()->GetXmin(),
      refHist->GetXaxis()->GetXmax());
  plotWithRef.mPlot->GetXaxis()->SetTitle(refHist->GetXaxis()->GetTitle());
  plotWithRef.mPlot->GetYaxis()->SetTitle(refHist->GetYaxis()->GetTitle());
  plotWithRef.mPlot->SetOption("hist");
  plotWithRef.mPlot->SetStats(0);

  //plotWithRef.mRefMO = refMO;
  plotWithRef.mRefPlot = std::make_shared<TH1F>((fullOutPath + "_hist_ref").c_str(),
      refHist->GetTitle(),
      refHist->GetXaxis()->GetNbins(),
      refHist->GetXaxis()->GetXmin(),
      refHist->GetXaxis()->GetXmax());
  plotWithRef.mRefPlot->SetLineColor(kRed);
  plotWithRef.mRefPlot->SetLineStyle(kDashed);
  plotWithRef.mRefPlot->SetLineWidth(2);
  plotWithRef.mRefPlot->SetOption("hist");

  plotWithRef.mCanvas = std::make_shared<TCanvas>(fullOutPath.c_str(), fullOutPath.c_str(), 800, 600);

  getObjectsManager()->startPublishing(plotWithRef.mCanvas.get());
}

//_________________________________________________________________________________________

template <class HIST, class HISTRATIO>
void ReferenceComparatorTask::initHistograms2D(TObject* refObject, const ReferenceComparatorTaskConfig::DataGroup& group, std::string path)
{
  HIST* refHist = dynamic_cast<HIST*>(refObject);
  if (!refHist) {
    return;
  }

  auto fullPath = group.inputPath + "/" + path;
  auto fullOutPath = group.outputPath + "/" + path;

  mHistogramRatios[fullPath] = std::make_unique<HISTRATIO>((fullOutPath + "_RefRatio").c_str(),
      refHist->GetTitle(),
      refHist->GetXaxis()->GetNbins(),
      refHist->GetXaxis()->GetXmin(),
      refHist->GetXaxis()->GetXmax(),
      refHist->GetYaxis()->GetNbins(),
      refHist->GetYaxis()->GetXmin(),
      refHist->GetYaxis()->GetXmax(),
      false);
  HISTRATIO* hRatio = dynamic_cast<HISTRATIO*>(mHistogramRatios[fullPath].get());
  hRatio->GetXaxis()->SetTitle(refHist->GetXaxis()->GetTitle());
  hRatio->GetYaxis()->SetTitle(refHist->GetYaxis()->GetTitle());
  hRatio->GetZaxis()->SetTitle("histogram / reference");

  hRatio->SetStats(0);
  hRatio->SetOption("COLZ");
  getObjectsManager()->startPublishing(hRatio);
  getObjectsManager()->setDefaultDrawOptions(hRatio, "COLZ");
}

//_________________________________________________________________________________________

void ReferenceComparatorTask::addHistoWithReference(std::shared_ptr<o2::quality_control::core::MonitorObject> refMO, ReferenceComparatorTaskConfig::DataGroup group, std::string path)
{
  if (!refMO) {
    return;
  }
  auto refObject = refMO->getObject();
  // only consider MOs that are histograms
  if (!refObject || !refObject->InheritsFrom("TH1")) {
    return;
  }

  if (refObject->IsA() == TClass::GetClass<TH1F>() || refObject->InheritsFrom("TH1F")) {
    initHistograms1D<TH1F, TH1FRatio>(refObject, group, path);
  }

  if (refObject->IsA() == TClass::GetClass<TH1D>() || refObject->InheritsFrom("TH1D")) {
    initHistograms1D<TH1D, TH1DRatio>(refObject, group, path);
  }

  if (refObject->IsA() == TClass::GetClass<TH2F>() || refObject->InheritsFrom("TH2F")) {
    initHistograms2D<TH2F, TH2FRatio>(refObject, group, path);
  }

  if (refObject->IsA() == TClass::GetClass<TH2D>() || refObject->InheritsFrom("TH2D")) {
    initHistograms2D<TH2D, TH2DRatio>(refObject, group, path);
  }
}

//_________________________________________________________________________________________

void ReferenceComparatorTask::initialize(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef services)
{
  // reset all existing objects
  mPlotNames.clear();
  mHistograms.clear();
  mHistogramRatios.clear();
  mRefScale.clear();
  mRefPlots.clear();

  auto& qcdb = services.get<repository::DatabaseInterface>();
  mNotOlderThan = getParameter<int>("notOlderThan", 120, t.activity);
  mRefRun = getParameter<int>("referenceRun", 0, t.activity);

  // load and initialize the input groups
  for (auto group : mConfig.dataGroups) {
    auto groupName = group.name;
    auto& plotVec = mPlotNames[groupName];
    for (auto path : group.objects) {
      auto fullPath = group.inputPath + "/" + path.first;
      auto refPlot = getRefPlot(qcdb, fullPath, t.activity);
      if (!refPlot) {
        continue;
      }

      plotVec.push_back(fullPath);
      mRefPlots[fullPath] = refPlot;
      mRefScale[fullPath] = path.second;

      addHistoWithReference(refPlot, group, path.first);
    }
  }
}

//_________________________________________________________________________________________

static bool checkAxis(TH1* h1, TH1* h2)
{
  // check consistency of X-axis binning
  if (h1->GetXaxis()->GetNbins() != h2->GetXaxis()->GetNbins()) {
    return false;
  }
  if (h1->GetXaxis()->GetXmin() != h2->GetXaxis()->GetXmin()) {
    return false;
  }
  if (h1->GetXaxis()->GetXmax() != h2->GetXaxis()->GetXmax()) {
    return false;
  }

  // check consistency of Y-axis binning
  if (h1->GetYaxis()->GetNbins() != h2->GetYaxis()->GetNbins()) {
    return false;
  }
  if (h1->GetYaxis()->GetXmin() != h2->GetYaxis()->GetXmin()) {
    return false;
  }
  if (h1->GetYaxis()->GetXmax() != h2->GetYaxis()->GetXmax()) {
    return false;
  }

  // check consistency of Z-axis binning
  if (h1->GetZaxis()->GetNbins() != h2->GetZaxis()->GetNbins()) {
    return false;
  }
  if (h1->GetZaxis()->GetXmin() != h2->GetZaxis()->GetXmin()) {
    return false;
  }
  if (h1->GetZaxis()->GetXmax() != h2->GetZaxis()->GetXmax()) {
    return false;
  }

  return true;
}

//_________________________________________________________________________________________

void updateHistograms(TH1* num, TH1* den, TH1* hist, TH1* ref, bool scaleDen)
{
  if (!num || !den || !hist || !ref) {
    return;
  }

  if (!checkAxis(hist, ref)) {
    return;
  }

  if (!checkAxis(hist, num)) {
    return;
  }

  if (!checkAxis(num, den)) {
    return;
  }

  double scale = 1;
  if (scaleDen) {
    // the reference histogram is scaled to mach the integral of the current histogram
    double integral = hist->Integral();
    double integralRef = ref->Integral();
    scale = integral / integralRef;
  }

  if (hist->GetDimension() == 1) {
    for (int xbin = 1; xbin <= num->GetXaxis()->GetNbins(); xbin++) {
      num->SetBinContent(xbin, hist->GetBinContent(xbin));
      num->SetBinError(xbin, hist->GetBinError(xbin));
      den->SetBinContent(xbin, scale * ref->GetBinContent(xbin));
      den->SetBinError(xbin, scale * ref->GetBinError(xbin));
    }
  }

  if (hist->GetDimension() == 2) {
    for (int xbin = 1; xbin <= num->GetXaxis()->GetNbins(); xbin++) {
      for (int ybin = 1; ybin <= num->GetYaxis()->GetNbins(); ybin++) {
        num->SetBinContent(xbin, ybin, hist->GetBinContent(xbin, ybin));
        num->SetBinError(xbin, ybin, hist->GetBinError(xbin, ybin));
        den->SetBinContent(xbin, ybin, scale * ref->GetBinContent(xbin, ybin));
        den->SetBinError(xbin, ybin, scale * ref->GetBinError(xbin, ybin));
      }
    }
  }
}

//_________________________________________________________________________________________

void ReferenceComparatorTask::updatePlot(std::string plot, TObject* object)
{
  // object inherits from TH1, uodate the plot ratios and the histigrams with superimposed reference
  // first check if a corresponding ratio plot was initialized
  auto iter = mHistograms.find(plot);
  if (mHistograms.find(plot) == mHistograms.end()) {
    return;
  }

  TH1* hist = dynamic_cast<TH1*>(object);
  if (!hist) {
    return;
  }
  auto moRef = mRefPlots[plot];
  TH1* histRef = dynamic_cast<TH1*>(moRef->getObject());

  bool scaleRef = mRefScale[plot];

  auto& plotWithRef = mHistograms[plot];
  updateHistograms(plotWithRef.mPlot.get(), plotWithRef.mRefPlot.get(), hist, histRef, scaleRef);

  // draw the histograms superimposed in the canvas
  plotWithRef.mCanvas->Clear();
  plotWithRef.mCanvas->cd();
  plotWithRef.mPlot->Draw("hist");
  plotWithRef.mRefPlot->Draw("histsame");
}

//_________________________________________________________________________________________

void ReferenceComparatorTask::updatePlotRatio(std::string plot, TObject* object)
{
  // object inherits from TH1, uodate the plot ratios and the histigrams with superimposed reference
  // first check if a corresponding ratio plot was initialized
  auto iter = mHistogramRatios.find(plot);
  if (iter == mHistogramRatios.end()) {
    return;
  }

  TH1* hist = dynamic_cast<TH1*>(object);
  if (!hist) {
    return;
  }
  auto moRef = mRefPlots[plot];
  TH1* histRef = dynamic_cast<TH1*>(moRef->getObject());

  bool scaleRef = mRefScale[plot];

  if (object->IsA() == TClass::GetClass<TH1F>() || object->InheritsFrom("TH1F")) {
    auto histRatio = dynamic_cast<TH1FRatio*>(iter->second.get());
    if (!histRatio) {
      return;
    }
    updateHistograms(histRatio->getNum(), histRatio->getDen(), hist, histRef, scaleRef);
    histRatio->update();
  }

  if (object->InheritsFrom("TH1D")) {
    auto histRatio = dynamic_cast<TH1DRatio*>(iter->second.get());
    if (!histRatio) {
      return;
    }
    updateHistograms(histRatio->getNum(), histRatio->getDen(), hist, histRef, scaleRef);
    histRatio->update();
  }

  if (object->InheritsFrom("TH2F")) {
    auto histRatio = dynamic_cast<TH2FRatio*>(iter->second.get());
    if (!histRatio) {
      return;
    }
    updateHistograms(histRatio->getNum(), histRatio->getDen(), hist, histRef, scaleRef);
    histRatio->update();
  }

  if (object->InheritsFrom("TH2D")) {
    auto histRatio = dynamic_cast<TH2DRatio*>(iter->second.get());
    if (!histRatio) {
      return;
    }
    updateHistograms(histRatio->getNum(), histRatio->getDen(), hist, histRef, scaleRef);
    histRatio->update();
  }
}

//_________________________________________________________________________________________

void ReferenceComparatorTask::update(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  for (auto& [groupName, plotVec] : mPlotNames) {
    for (auto& plot : plotVec) {
      // get object for current timestamp - age limit is converted to milliseconds
      auto object = getMO(qcdb, plot, t, mNotOlderThan * 1000);

      // skip objects that are not found or too old
      if (!object.second) {
        continue;
      }

      // if the object is valid, draw it together with the reference
      if (object.first && object.first->getObject() && object.first->getObject()->InheritsFrom("TH1")) {
        updatePlot(plot, object.first->getObject());
        updatePlotRatio(plot, object.first->getObject());
      }
    }
  }
}

//_________________________________________________________________________________________

void ReferenceComparatorTask::finalize(quality_control::postprocessing::Trigger t, framework::ServiceRegistryRef)
{
}

} // namespace o2::quality_control_modules::common
