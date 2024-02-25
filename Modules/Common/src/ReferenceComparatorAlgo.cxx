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

#include "Common/ReferenceComparatorAlgo.h"
// ROOT
#include <TClass.h>
#include <TH1.h>
#include <TH2.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TDirectory.h>

//#include <DataFormatsQualityControl/FlagReasons.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::common
{

//_________________________________________________________________________________________
//
// Helper function for comparing two histograms based on the chi2 probability
// It returns a Good quality if the probability is higher or equal to pMin

static Quality compChi2(TH1* hist, TH1* refHist, double pMin)
{
  auto p = hist->Chi2Test(refHist, "UU NORM");

  if (p < pMin) {
    return Quality::Bad;
  }
  return Quality::Good;
}

//_________________________________________________________________________________________
//
// Helper function for comparing two histograms based on the average deviation between the bins
// It returns a Good quality if the deviation is lower or equal to deltaMax

static Quality compDeviation(TH1* hist, TH1* refHist, double deltaMax)
{
  if (hist->GetNcells() < 3) {
    return Quality::Null;
  }
  if (hist->GetNcells() != refHist->GetNcells()) {
    return Quality::Null;
  }

  double delta = 0;
  for (int bin = 1; bin < (hist->GetNcells() - 1); bin++) {
    double val = hist->GetBinContent(bin);
    double refVal = refHist->GetBinContent(bin);
    delta += (refVal == 0) ? 0 : std::abs((val - refVal) / refVal);
  }
  delta /= hist->GetNcells() - 2;

  //std::cout << "Average deviation for '" << hist->GetName() << "' = " << delta << ", max = " << deltaMax << std::endl;

  if (delta > deltaMax) {
    return Quality::Bad;
  }
  return Quality::Good;
}

//_________________________________________________________________________________________
//
// Compare an histogram with a reference one, using the selected comparison method
// Returns the quality flag given by the comparison function

Quality compareTH1(TH1* hist, TH1* refHist, MOCompMethod method, double threshold, bool normalize)
{
  if (!hist) {
    return Quality::Null;
  }
  if (!refHist) {
    return Quality::Null;
  }
  if (refHist->GetEntries() < 1) {
    return Quality::Null;
  }

  Quality result = Quality::Null;

  if (normalize) {
    auto integral = hist->Integral();
    auto refIntegral = refHist->Integral();
    if (integral != 0 && refIntegral != 0) {
      hist->Scale(refIntegral / integral);
    }
  }

  switch (method) {
    case MOCompChi2:
      result = compChi2(hist, refHist, threshold);
      break;
    case MOCompDeviation:
      result = compDeviation(hist, refHist, threshold);
      break;
    default:
      break;
  }

  return result;
}

//_________________________________________________________________________________________
//
// Compare a graph with a reference one, using the selected comparison method
// As the two graphs mght have a different number of points, the cmparion is based
// on the average +/- variance of the values in each graph
// Returns the quality flag given by the comparison function

static double getAverage(TGraph* graph)
{
  double average = 0;
  int N = graph->GetN();
  if (N < 1) {
    return average;
  }

  for (int i = 0; i < N; i++) {
    Double_t x, y;
    graph->GetPoint(i, x, y);
    average += y;
  }
  average /= N;

  return average;
}

static double getVariance(TGraph* graph, double average)
{
  double variance = 0;
  int N = graph->GetN();
  if (N < 2) {
    return variance;
  }

  for (int i = 0; i < N; i++) {
    Double_t x, y;
    graph->GetPoint(i, x, y);
    variance += (y - average) * (y - average);
  }
  variance /= (N - 1);

  return variance;
}

Quality compareTGraph(TGraph* graph, TGraph* refGraph, MOCompMethod method, double threshold)
{
  Quality result = Quality::Null;

  if (!graph) {
    return result;
  }
  if (!refGraph) {
    return result;
  }

  if (refGraph->GetN() < 1) {
    return result;
  }

  double average = getAverage(graph);
  double variance = getVariance(graph, average);

  double refAverage = getAverage(refGraph);
  double refVariance = getVariance(refGraph, refAverage);

  // compare using histograms
  {
    TDirectory::TContext ctx(nullptr);
    TH1D* h1 = new TH1D("h1", "h1", 1, 0, 1);
    h1->SetBinContent(1, average);
    h1->SetBinError(1, std::sqrt(variance));
    TH1D* h2 = new TH1D("h2", "h2", 1, 0, 1);
    h2->SetBinContent(1, refAverage);
    h2->SetBinError(1, std::sqrt(refVariance));
    result = compareTH1(h1, h2, method, threshold, false);
    delete h1;
    delete h2;
  }

  return result;
}

//_________________________________________________________________________________________
//
// Given a MonitorObject containing a TH1, compare the histogram with the corresponding
// object from the reference run, using the selected comparison method

Quality checkTH1(TObject* obj, TObject* objRef, MOCompMethod method, double threshold)
{
  if (!obj || !objRef) {
    return Quality::Null;
  }

  TH1* hist = dynamic_cast<TH1*>(obj);
  TH1* refHist = dynamic_cast<TH1*>(objRef);

  return compareTH1(hist, refHist, method, threshold, true);
}

//_________________________________________________________________________________________
//
// Given a MonitorObject containing a TGraph, compare it with the corresponding
// object from the reference run, using the selected comparison method

Quality checkTGraph(TObject* obj, TObject* objRef, MOCompMethod method, double threshold)
{
  if (!obj || !objRef) {
    return Quality::Null;
  }

  TGraph* graph = dynamic_cast<TGraph*>(obj);
  TGraph* refGraph = dynamic_cast<TGraph*>(objRef);

  return compareTGraph(graph, refGraph, method, threshold);
}

//_________________________________________________________________________________________
//
// Helper function for retrieving a TObject embedded in a TCanvas, given the object name

static TObject* getObjectFromCanvas(std::string name, TCanvas* canvas)
{
  auto pos = name.rfind('/');
  const std::string baseName = (pos < std::string::npos) ? name.substr(pos + 1) : name;

  TIter next(canvas->GetListOfPrimitives());
  TObject* obj;
  while ((obj = next())) {
    const std::string objName(obj->GetName());
    auto pos = objName.rfind('/');
    const std::string objBaseName = (pos < std::string::npos) ? objName.substr(pos + 1) : objName;

    if (baseName == objBaseName) {
      return obj;
    }
  }

  return nullptr;
}

//_________________________________________________________________________________________
//
// Given a TCanvas, compare all the embedded histograms and graphs

static Quality checkTCanvas(TObject* obj, TObject* objRef, MOCompMethod method, double threshold)
{
  if (!obj || !objRef) {
    return Quality::Null;
  }

  TCanvas* canvas = dynamic_cast<TCanvas*>(obj);
  TCanvas* refCanvas = dynamic_cast<TCanvas*>(objRef);
  if (!canvas || !refCanvas) {
    return Quality::Null;
  }

  Quality result = Quality::Null;
  TIter next(canvas->GetListOfPrimitives());
  TObject* cobj;
  while ((cobj = next())) {
    auto q = compareTObjects(cobj, getObjectFromCanvas(cobj->GetName(), refCanvas), method, threshold);
    if (q == Quality::Null) {
      continue;
    }
    if (result == Quality::Null || q.isWorseThan(result)) {
      result = q;
    }
  }
  return result;
}

//_________________________________________________________________________________________
//
// Compare a TObject with the corresponding reference, using the appropriate checker method
// based on the actual object inheritance (TH1, TGraph or TCanvas)

Quality compareTObjects(TObject* obj, TObject* objRef, MOCompMethod method, double threshold)
{
  if (!obj || !objRef) {
    return Quality::Null;
  }

  if (obj->IsA() != objRef->IsA()) {
    return Quality::Null;
  }

  // call the appropriate checked method based on the object inheritance
  if (obj->InheritsFrom("TH1")) {
    return checkTH1(obj, objRef, method, threshold);
  } else if (obj->IsA() == TClass::GetClass<TCanvas>()) {
    return checkTCanvas(obj, objRef, method, threshold);
  } else if (obj->IsA() == TClass::GetClass<TGraph>() || obj->InheritsFrom("TGraph")) {
    return checkTGraph(obj, objRef, method, threshold);
  }

  return Quality::Null;
}

}