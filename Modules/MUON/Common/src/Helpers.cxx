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

#include "MUONCommon/Helpers.h"
#include <TLine.h>
#include <TAxis.h>
#include <TH1.h>
#include <TList.h>
#include <regex>
#include "QualityControl/ObjectsManager.h"

namespace o2::quality_control_modules::muon
{
template <>
std::string getConfigurationParameter(o2::quality_control::core::CustomParameters customParameters, std::string parName, const std::string defaultValue)
{
  std::string result = defaultValue;
  auto parOpt = customParameters.atOptional(parName);
  if (parOpt.has_value()) {
    result = parOpt.value();
  }
  return result;
}

template <>
std::string getConfigurationParameter(o2::quality_control::core::CustomParameters customParameters, std::string parName, const std::string defaultValue, const o2::quality_control::core::Activity& activity)
{
  auto parOpt = customParameters.atOptional(parName, activity);
  if (parOpt.has_value()) {
    std::string result = parOpt.value();
    return result;
  }
  return getConfigurationParameter<std::string>(customParameters, parName, defaultValue);
}

//_________________________________________________________________________________________

TLine* addHorizontalLine(TH1& histo, double y,
                         int lineColor, int lineStyle,
                         int lineWidth)
{
  auto nbins = histo.GetXaxis()->GetNbins();

  TLine* line = new TLine(histo.GetBinLowEdge(1), y, histo.GetBinLowEdge(nbins) + histo.GetBinWidth(nbins), y);
  line->SetLineColor(lineColor);
  line->SetLineStyle(lineStyle);
  line->SetLineWidth(lineWidth);
  histo.GetListOfFunctions()->Add(line);
  return line;
}

void markBunchCrossing(TH1& histo,
                       gsl::span<int> bunchCrossings)
{
  for (auto b : bunchCrossings) {
    addVerticalLine(histo, b * 1.0, 1, 10, 1);
  }
}

TLine* addVerticalLine(TH1& histo, double x,
                       int lineColor, int lineStyle,
                       int lineWidth)
{
  TLine* line = new TLine(x, histo.GetMinimum(),
                          x, histo.GetMaximum());
  line->SetLineColor(lineColor);
  line->SetLineStyle(lineStyle);
  line->SetLineWidth(lineWidth);
  histo.GetListOfFunctions()->Add(line);
  return line;
}

// remove all elements of class c
// from histo->GetListOfFunctions()
void cleanup(TH1& histo, const char* classname)
{
  TList* elements = histo.GetListOfFunctions();
  TIter next(elements);
  TObject* obj;
  std::vector<TObject*> toBeRemoved;
  while ((obj = next())) {
    if (strcmp(obj->ClassName(), classname) == 0) {
      toBeRemoved.push_back(obj);
    }
  }
  for (auto o : toBeRemoved) {
    elements->Remove(o);
  }
}
} // namespace o2::quality_control_modules::muon
