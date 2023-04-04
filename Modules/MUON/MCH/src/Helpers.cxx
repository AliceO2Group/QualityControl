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
/// \file   PhysicsTask.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#include "MCH/Helpers.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QualityObject.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include "MCHRawElecMap/Mapper.h"
#include <TLine.h>
#include <TLine.h>
#include <TText.h>
#include <TMath.h>
#include <fmt/format.h>
#include <iostream>
#include <limits>
#include <chrono>

using namespace o2::quality_control;
using namespace o2::quality_control::core;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

std::string getHistoPath(int deId)
{
  return fmt::format("ST{}/DE{}/", (deId - 100) / 200 + 1, deId);
}

//_________________________________________________________________________________________

bool matchHistName(std::string hist, std::string name)
{
  if (name.empty()) {
    return false;
  }

  int64_t pos = hist.find(name);
  int64_t histSize = hist.size();
  int64_t nameSize = name.size();
  int64_t diff = histSize - nameSize;
  return ((pos >= 0) && (pos == diff));
}

//_________________________________________________________________________________________

int getChamberIndex(int deId)
{
  return (deId / 100) - 1;
}

int getNumDEinChamber(int chIndex)
{
  int nDE = 0;
  switch (chIndex) {
    case 0:
    case 1:
    case 2:
    case 3:
      nDE = 4;
      break;
    case 4:
    case 5:
      nDE = 18;
      break;
    case 6:
    case 7:
    case 8:
    case 9:
      nDE = 26;
      break;
    default:
      break;
  }
  return nDE;
}

std::pair<int, int> getDEindexInChamber(int deId)
{
  std::pair<int, int> result = std::make_pair(int(-1), int(-1));
  int nDE = getNumDEinChamber(getChamberIndex(deId));
  if (nDE == 0) {
    return result;
  }
  // number of detectors in one half chamber
  int nDEhc = nDE / 2;

  // detector index within the chamber
  int idx = (deId - 100) % 100;
  // minimum and maximum detector indexes for the L side
  int lMin = (nDEhc + 1) / 2;
  int lMax = lMin + nDEhc - 1;
  if (idx >= lMin && idx <= lMax) {
    // detector on the L side, simply sibtract lMin
    result.second = 0;
    result.first = idx - lMin;
  } else {
    // detector on the R side, compute index separately above and below middle horizontal axis
    result.second = 1;
    if (idx > lMax) {
      idx -= nDE;
    }
    result.first = lMin - idx - 1;
  }
  return result;
}

int getChamberOffset(int chIndex)
{
  int offset = 0;
  for (int c = 0; c < chIndex; c++) {
    offset += getNumDEinChamber(c);
  }
  return offset;
}

int getDEindex(int deId)
{
  auto idx = getDEindexInChamber(deId);
  if (idx.first < 0 || idx.second < 0) {
    return -1;
  }
  int offset = getChamberOffset(getChamberIndex(deId));

  // number of detectors in one half chamber
  int nDE = getNumDEinChamber(getChamberIndex(deId));
  if (idx.second > 0) {
    idx.first += nDE / 2;
  }

  return idx.first + offset;
}

//_________________________________________________________________________________________

QualityChecker::QualityChecker()
{
  for (auto de : o2::mch::constants::deIdsForAllMCH) {
    auto idx = getDEindex(de);
    if (idx >= 0) {
      mChamberMap[idx] = getChamberIndex(de);
      mDeMap[idx] = getDEindexInChamber(de);
    }
  }

  reset();
}

void QualityChecker::reset()
{
  std::fill(mQuality.begin(), mQuality.end(), Quality::Null);
}

void QualityChecker::addCheckResult(gsl::span<Quality> result)
{
  if (mQuality.size() > result.size()) {
    return;
  }

  for (int i = 0; i < mQuality.size(); i++) {
    if ((mQuality[i] == Quality::Null) || (result[i] == Quality::Bad)) {
      mQuality[i] = result[i];
    }
  }
}

Quality QualityChecker::getQuality()
{
  Quality result = Quality::Null;
  for (int i = 0; i < mQuality.size(); i++) {
    if (mQuality[i] != Quality::Null) {
      result = Quality::Good;
    }
  }

  // number of bad detection elements for ST12 and ST345 and separately for L/R sides
  int nBadDE[2][2] = { { 0, 0 }, { 0, 0 } };

  for (int i = 0; i < mQuality.size(); i++) {
    // if detector is OK, skip it
    if (mQuality[i] != Quality::Bad) {
      continue;
    }
    auto chamber1 = mChamberMap[i];
    int station1 = chamber1 / 2;
    auto deidx1 = mDeMap[i];

    int si = (station1 < 2) ? 0 : 1;
    int lr = deidx1.second;
    nBadDE[si][lr] += 1;

    for (int j = i + 1; j < mQuality.size(); j++) {
      // if detector is OK, skip it
      if (mQuality[j] != Quality::Bad) {
        continue;
      }

      // if we reach here, it means that both detectors are bad
      // now we check if they are also facing each other in the same station,
      // and therefore could lead to acceptance holes
      auto chamber2 = mChamberMap[j];
      int station2 = chamber2 / 2;
      auto deidx2 = mDeMap[j];

      // we only check detector pairs that:
      // a. belong to the same station(s)
      // b. are not in the same chamber
      // c. are on the same side of the spectrometer
      // In the quadrants case, we combine together both stations since
      // the tracking requires 3 chambers out of 4, as opposed to SLATs
      // where one point per station is required

      // skip detection elements in the same chamber
      if (chamber2 == chamber1) {
        continue;
      }
      // skip detection elements on opposite sides
      if (deidx1.second != deidx2.second) {
        continue;
      }

      if (station1 < 2) {
        // Quadrants: check that both DE are in ST12
        if (station2 >= 2) {
          continue;
        }
      } else {
        // SLATs: check that both DE belong to the same station
        if (station2 != station1) {
          continue;
        }
      }

      // if two detectors directly facing each other are both bad, than
      // we flag the overall quality as bad
      if (deidx2.first == deidx1.first) {
        result = Quality::Bad;
        break;
      }
      if (station1 >= 2) {
        // for SLATs, we also combine detector I in one chamber
        // with detectors I-1 and I+1 in the other chamber
        if (deidx2.first == (deidx1.first - 1)) {
          result = Quality::Bad;
          break;
        }
        if (deidx2.first == (deidx1.first + 1)) {
          result = Quality::Bad;
          break;
        }
      }
    }
  }

  if (result == Quality::Good) {
    // If no bad combination was found, check the total number of bad DEs,
    // separately for ST12 and ST345.
    // If the number is above threshold, set the quality to Medium
    if ((nBadDE[0][0] + nBadDE[0][1]) > mMaxBadST12) {
      result = Quality::Medium;
    }
    if ((nBadDE[1][0] + nBadDE[1][1]) > mMaxBadST345) {
      result = Quality::Medium;
    }
  }

  return result;
}

//_________________________________________________________________________________________

void addChamberDelimiters(TH1F* h, float xmin, float xmax)
{
  if (!h) {
    return;
  }
  // disable ticks on horizontal axis
  h->GetXaxis()->SetTickLength(0.0);
  h->GetXaxis()->SetLabelSize(0.0);
  h->GetYaxis()->SetTickLength(0);

  TText* xtitle = new TText();
  xtitle->SetNDC();
  xtitle->SetText(0.87, 0.03, "chamber");
  xtitle->SetTextSize(10);
  xtitle->SetTextFont(42);
  h->GetListOfFunctions()->Add(xtitle);

  float scaleMin = xmin;
  float scaleMax = xmax;
  if (xmin == xmax) {
    scaleMin = h->GetMinimum() * 0.95;
    scaleMax = h->GetMaximum() * 1.05;
  }

  // draw chamber delimiters
  for (int ch = 1; ch <= 9; ch++) {
    float xpos = static_cast<float>(getChamberOffset(ch));
    TLine* delimiter = new TLine(xpos, scaleMin, xpos, scaleMax);
    delimiter->SetLineColor(kBlack);
    delimiter->SetLineStyle(kDashed);
    h->GetListOfFunctions()->Add(delimiter);
  }

  // draw x-axis labels
  for (int ch = 0; ch <= 9; ch += 1) {
    float x1 = static_cast<float>(getChamberOffset(ch));
    float x2 = (ch < 9) ? static_cast<float>(getChamberOffset(ch + 1)) : h->GetXaxis()->GetXmax();
    float x0 = 0.8 * (x1 + x2) / (2 * h->GetXaxis()->GetXmax()) + 0.1;
    float y0 = 0.05;
    TText* label = new TText();
    label->SetNDC();
    label->SetText(x0, y0, TString::Format("%d", ch + 1));
    label->SetTextSize(10);
    label->SetTextFont(42);
    label->SetTextAlign(22);
    h->GetListOfFunctions()->Add(label);
  }
}

//_________________________________________________________________________________________

void addChamberDelimiters(TH2F* h)
{
  if (!h) {
    return;
  }
  // disable ticks on horizontal axis
  h->GetXaxis()->SetTickLength(0.0);
  h->GetXaxis()->SetLabelSize(0.0);
  h->GetYaxis()->SetTickLength(0);

  TText* xtitle = new TText();
  xtitle->SetNDC();
  xtitle->SetText(0.87, 0.03, "chamber");
  xtitle->SetTextSize(10);
  xtitle->SetTextFont(42);
  h->GetListOfFunctions()->Add(xtitle);

  float scaleMin = h->GetYaxis()->GetXmin();
  float scaleMax = h->GetYaxis()->GetXmax();

  // draw chamber delimiters
  for (int ch = 1; ch <= 9; ch++) {
    float xpos = static_cast<float>(getChamberOffset(ch));
    TLine* delimiter = new TLine(xpos, scaleMin, xpos, scaleMax);
    delimiter->SetLineColor(kBlack);
    delimiter->SetLineStyle(kDashed);
    h->GetListOfFunctions()->Add(delimiter);
  }

  // draw x-axis labels
  for (int ch = 0; ch <= 9; ch += 1) {
    float x1 = static_cast<float>(getChamberOffset(ch));
    float x2 = (ch < 9) ? static_cast<float>(getChamberOffset(ch + 1)) : h->GetXaxis()->GetXmax();
    float x0 = 0.8 * (x1 + x2) / (2 * h->GetXaxis()->GetXmax()) + 0.1;
    float y0 = 0.05;
    TText* label = new TText();
    label->SetNDC();
    label->SetText(x0, y0, TString::Format("%d", ch + 1));
    label->SetTextSize(10);
    label->SetTextFont(42);
    label->SetTextAlign(22);
    h->GetListOfFunctions()->Add(label);
  }
}

//_________________________________________________________________________________________

void splitDataSourceName(std::string s, std::string& type, std::string& name)
{
  std::string delimiter = ":";
  size_t pos = s.find(delimiter);
  if (pos != std::string::npos) {
    type = s.substr(0, pos);
    s.erase(0, pos + delimiter.length());
  }
  name = s;
}

//_________________________________________________________________________________________

static long getMoTimeStamp(std::shared_ptr<o2::quality_control::core::MonitorObject> mo)
{
  long timeStamp{ 0 };

  auto iter = mo->getMetadataMap().find(repository::metadata_keys::created);
  if (iter != mo->getMetadataMap().end()) {
    timeStamp = std::stol(iter->second);
  }

  return timeStamp;
}

//_________________________________________________________________________________________

static bool checkMoTimeStamp(std::shared_ptr<o2::quality_control::core::MonitorObject> mo, uint64_t& timeStampOld)
{
  auto timeStamp = getMoTimeStamp(mo);
  bool result = (timeStamp != timeStampOld) ? true : false;
  timeStampOld = timeStamp;
  return result;
}

//_________________________________________________________________________________________

template <typename T>
T* getPlotFromMO(std::shared_ptr<o2::quality_control::core::MonitorObject> mo)
{
  // Get ROOT object
  TObject* obj = mo ? mo->getObject() : nullptr;
  if (!obj) {
    return nullptr;
  }

  // Get histogram object
  T* h = dynamic_cast<T*>(obj);
  return h;
}

//_________________________________________________________________________________________

CcdbObjectHelper::CcdbObjectHelper()
{
  setStartIme();
}

//_________________________________________________________________________________________

CcdbObjectHelper::CcdbObjectHelper(std::string p, std::string n) : mPath(p), mName(n)
{
  setStartIme();
}

//_________________________________________________________________________________________

void CcdbObjectHelper::setStartIme()
{
  mTimeStart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

//_________________________________________________________________________________________

bool CcdbObjectHelper::update(repository::DatabaseInterface* qcdb, long timeStamp, const core::Activity& activity)
{
  mObject = qcdb->retrieveMO(mPath, mName, timeStamp, activity);
  if (!mObject) {
    return false;
  }

  if (timeStamp > mTimeStart) {
    // we are requesting a new object, so check that the retrieved one
    // was created after the start of the processing
    if (getMoTimeStamp(mObject) < mTimeStart) {
      return false;
    }
  }

  if (!checkMoTimeStamp(mObject, mTimeStamp)) {
    return false;
  }

  return true;
}

//_________________________________________________________________________________________

static long getQoTimeStamp(std::shared_ptr<o2::quality_control::core::QualityObject> qo)
{
  long timeStamp{ 0 };

  auto iter = qo->getMetadataMap().find(repository::metadata_keys::created);
  if (iter != qo->getMetadataMap().end()) {
    timeStamp = std::stol(iter->second);
  }

  return timeStamp;
}

//_________________________________________________________________________________________

static bool checkQoTimeStamp(std::shared_ptr<o2::quality_control::core::QualityObject> qo, uint64_t& timeStampOld)
{
  auto timeStamp = getQoTimeStamp(qo);
  bool result = (timeStamp != timeStampOld) ? true : false;
  timeStampOld = timeStamp;
  return result;
}

//_________________________________________________________________________________________

QualityObjectHelper::QualityObjectHelper()
{
  setStartIme();
}

//_________________________________________________________________________________________

QualityObjectHelper::QualityObjectHelper(std::string p, std::string n) : mPath(p), mName(n)
{
  setStartIme();
}

//_________________________________________________________________________________________

void QualityObjectHelper::setStartIme()
{
  mTimeStart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

//_________________________________________________________________________________________

bool QualityObjectHelper::update(repository::DatabaseInterface* qcdb, long timeStamp, const core::Activity& activity)
{
  mObject = qcdb->retrieveQO(mPath + "/" + mName, timeStamp, activity);
  if (!mObject) {
    return false;
  }

  if (timeStamp > mTimeStart) {
    // we are requesting a new object, so check that the retrieved one
    // was created after the start of the processing
    if (getQoTimeStamp(mObject) < mTimeStart) {
      return false;
    }
  }

  if (!checkQoTimeStamp(mObject, mTimeStamp)) {
    mUpdated = false;
  } else {
    mUpdated = true;
  }

  return true;
}

//_________________________________________________________________________________________

TrendGraph::TrendGraph(std::string name, std::string title, std::string label, std::optional<float> refValue)
  : TCanvas(name.c_str(), title.c_str()),
    mRefValue(refValue),
    mAxisLabel(label)
{
  mGraphHist = std::make_unique<TGraph>(0);

  mGraph = std::make_unique<TGraph>(0);
  mGraph->SetMarkerStyle(kCircle);
  mGraph->SetTitle(fmt::format("{};time;{}", title, label).c_str());

  if (refValue) {
    mGraphRef = std::make_unique<TGraph>(0);
    mGraphRef->SetLineColor(kRed);
    mGraphRef->SetLineStyle(kDashed);

    float delta = (0.85 - 0.15) / 2;
    for (int i = 0; i < 2; i++) {
      mLegends[i] = std::make_unique<TLegend>((delta * i) + 0.15, 0.79, (delta * (i + 1)) + 0.15, 0.89);
      mLegends[i]->SetBorderSize(0);
    }
    mLegends[0]->AddEntry(mGraph.get(), "data     ", "lp");
    mLegends[1]->AddEntry(mGraphRef.get(), "reference", "l");
  }
}

//_________________________________________________________________________________________

void TrendGraph::update(uint64_t time, float val)
{
  mGraph->AddPoint(time, val);
  mGraphHist->AddPoint(time, 0);
  if (mRefValue && mGraphRef) {
    mGraphRef->AddPoint(time, mRefValue.value());
  }

  Clear();
  cd();

  mGraphHist->Draw("A");

  auto hAxis = mGraphHist->GetHistogram();
  hAxis->SetTitle(GetTitle());
  hAxis->GetYaxis()->SetTitle(mAxisLabel.c_str());
  hAxis->GetXaxis()->SetTimeDisplay(1);
  hAxis->GetXaxis()->SetNdivisions(505);
  hAxis->GetXaxis()->SetTimeOffset(0.0);
  hAxis->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");

  Double_t min = mGraphRef ? mRefValue.value() : std::numeric_limits<float>::max();
  Double_t max = mGraphRef ? mRefValue.value() : std::numeric_limits<float>::min();
  min = TMath::Min(min, TMath::MinElement(mGraph->GetN(), mGraph->GetY()));
  max = TMath::Max(max, TMath::MaxElement(mGraph->GetN(), mGraph->GetY()));
  auto delta = max - min;
  min -= 0.1 * delta;
  if (mGraphRef) {
    // leave some space at the top for the legend
    max += 0.3 * delta;
  } else {
    max += 0.1 * delta;
  }

  hAxis->SetMinimum(min);
  hAxis->SetMaximum(max);
  hAxis->Draw("AXIS");

  mGraph->Draw("PL,SAME");
  if (mGraphRef) {
    mGraphRef->Draw("L,SAME");
  }

  for (auto& l : mLegends) {
    if (l) {
      l->Draw();
    }
  }
}

//_________________________________________________________________________________________

QualityTrendGraph::QualityTrendGraph(std::string name, std::string title) : TCanvas(name.c_str(), title.c_str())
{
  SetGridy(1);
  mGraphHist = std::make_unique<TGraph>(0);

  mGraph = std::make_unique<TGraph>(0);
  mGraph->SetMarkerStyle(kCircle);
  mGraph->SetTitle(fmt::format("{};time;quality", title).c_str());

  mLabels[0] = std::make_unique<TText>(0.09, 0.1 + 0.8 / 8.0, "Null");
  mLabels[1] = std::make_unique<TText>(0.09, 0.1 + 3.0 * 0.8 / 8.0, "Bad");
  mLabels[2] = std::make_unique<TText>(0.09, 0.1 + 5.0 * 0.8 / 8.0, "Medium");
  mLabels[3] = std::make_unique<TText>(0.09, 0.1 + 7.0 * 0.8 / 8.0, "Good");
  for (auto& l : mLabels) {
    l->SetNDC();
    l->SetTextAlign(32);
    l->SetTextSize(0.08);
    l->SetTextFont(42);
  }
}

//_________________________________________________________________________________________

void QualityTrendGraph::update(uint64_t time, Quality q)
{
  float val = 0.5;
  if (q == Quality::Bad) {
    val = 1.5;
  }
  if (q == Quality::Medium) {
    val = 2.5;
  }
  if (q == Quality::Good) {
    val = 3.5;
  }
  mGraph->AddPoint(time, val);
  mGraphHist->AddPoint(time, 0);

  Clear();
  cd();

  mGraphHist->Draw("A");

  auto hAxis = mGraphHist->GetHistogram();
  hAxis->SetTitle(GetTitle());
  hAxis->GetXaxis()->SetTimeDisplay(1);
  hAxis->GetXaxis()->SetNdivisions(505);
  hAxis->GetXaxis()->SetTimeOffset(0.0);
  hAxis->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
  hAxis->GetYaxis()->SetTitle("");
  hAxis->GetYaxis()->SetTickLength(0.0);
  hAxis->GetYaxis()->SetLabelSize(0.0);
  hAxis->GetYaxis()->SetNdivisions(3, kFALSE);
  hAxis->SetMinimum(0);
  hAxis->SetMaximum(4);
  hAxis->Draw("AXIS");

  mGraph->Draw("PL,SAME");

  for (auto& l : mLabels) {
    l->Draw();
  }
}

//_________________________________________________________________________________________

TrendMultiGraph::TrendMultiGraph(std::string name, std::string title, std::string label)
  : TCanvas(name.c_str(), title.c_str()),
    mAxisLabel(label)
{
  mGraphHist = std::make_unique<TGraph>(0);
}

//_________________________________________________________________________________________

void TrendMultiGraph::addGraph(std::string name, std::string title, std::optional<float> refValue)
{
  if (mNGraphs >= 10) {
    return;
  }

  std::array<int, 10> markerStyles{ kPlus, kStar, kCircle, kMultiply, kFullTriangleUp,
                                    kFullTriangleDown, kOpenCircle, kOpenSquare, kOpenTriangleUp, kOpenDiamond };
  std::array<int, 10> markerColors{ 2, 4, 1, 3, kOrange, 6, 7, 8, 9, 11 };

  mGraphs[mNGraphs] = std::make_unique<TGraph>(0);
  mGraphs[mNGraphs]->SetNameTitle(name.c_str(), title.c_str());
  mGraphs[mNGraphs]->SetMarkerStyle(markerStyles[mNGraphs]);
  mGraphs[mNGraphs]->SetMarkerColor(markerColors[mNGraphs]);
  mGraphs[mNGraphs]->SetLineColor(markerColors[mNGraphs]);

  if (refValue) {
    mRefValues[mNGraphs] = refValue;
    mGraphsRef[mNGraphs] = std::make_unique<TGraph>(0);
    mGraphsRef[mNGraphs]->SetNameTitle((name + "Ref").c_str(), title.c_str());
    mGraphsRef[mNGraphs]->SetMarkerStyle(markerStyles[mNGraphs]);
    mGraphsRef[mNGraphs]->SetMarkerColor(markerColors[mNGraphs]);
    mGraphsRef[mNGraphs]->SetLineColor(markerColors[mNGraphs]);
    mGraphsRef[mNGraphs]->SetLineStyle(kDashed);
  }

  mNGraphs += 1;
}

//_________________________________________________________________________________________

void TrendMultiGraph::addLegends()
{
  int nrows = 0;
  int ncols = 0;
  switch (mNGraphs) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      nrows = 1;
      ncols = mNGraphs;
      break;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
      nrows = 2;
      ncols = mNGraphs / 2;
      break;
    default:
      break;
  }

  if (nrows == 0 || ncols == 0) {
    return;
  }

  float delta = (0.85 - 0.15) / ncols;
  for (int i = 0; i < ncols; i++) {
    mLegends[i] = std::make_unique<TLegend>((delta * i) + 0.15, 0.79, (delta * (i + 1)) + 0.15, 0.89);
    mLegends[i]->SetBorderSize(0);
  }

  for (int i = 0; i < mNGraphs; i++) {
    int lid = i % ncols;
    mLegends[lid]->AddEntry(mGraphs[i].get(), mGraphs[i]->GetTitle(), "lp");
    if (mGraphsRef[i]) {
      mLegends[lid]->AddEntry(mGraphsRef[i].get(), (std::string("ref. ") + mGraphs[i]->GetTitle()).c_str(), "l");
    }
  }
}

//_________________________________________________________________________________________

void TrendMultiGraph::update(long time, gsl::span<double> values)
{
  if (values.size() > mNGraphs) {
    return;
  }

  mGraphHist->AddPoint(time, 0);
  for (int i = 0; i < values.size(); i++) {
    auto& gr = mGraphs[i];
    gr->AddPoint(time, values[i]);
    if (mRefValues[i] && mGraphsRef[i]) {
      auto& grref = mGraphsRef[i];
      grref->AddPoint(time, mRefValues[i].value());
    }
  }

  // compute optimal vertical range if not set by user
  Double_t min;
  Double_t max;
  if (mYmin != mYmax) {
    min = mYmin;
    max = mYmax;
  } else {
    min = std::numeric_limits<float>::max();
    max = std::numeric_limits<float>::min();
    for (int i = 0; i < values.size(); i++) {
      auto& gr = mGraphs[i];
      min = TMath::Min(min, TMath::MinElement(gr->GetN(), gr->GetY()));
      max = TMath::Max(max, TMath::MaxElement(gr->GetN(), gr->GetY()));
    }
    auto delta = max - min;
    min -= 0.1 * delta;
    max += 0.3 * delta;
  }

  Clear();
  cd();

  mGraphHist->Draw("A");
  auto hAxis = mGraphHist->GetHistogram();
  hAxis->SetTitle(GetTitle());
  // hAxis->GetYaxis()->SetTitleOffset(1.4);
  hAxis->GetYaxis()->SetTitle(mAxisLabel.c_str());
  hAxis->GetXaxis()->SetTimeDisplay(1);
  hAxis->GetXaxis()->SetNdivisions(505);
  hAxis->GetXaxis()->SetTimeOffset(0.0);
  hAxis->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
  hAxis->SetMinimum(min);
  hAxis->SetMaximum(max);
  hAxis->Draw("AXIS");

  for (auto& gr : mGraphs) {
    if (gr) {
      gr->Draw("PL,SAME");
    }
  }

  for (auto& gr : mGraphsRef) {
    if (gr) {
      gr->Draw("L,SAME");
    }
  }

  for (auto& l : mLegends) {
    if (l) {
      l->Draw();
    }
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
