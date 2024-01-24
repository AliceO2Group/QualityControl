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
/// \file   HelperHist.h
/// \author Artur Furs afurs@cern.ch
/// \brief Histogram helper

#ifndef QC_MODULE_FIT_FITHELPERHIST_H
#define QC_MODULE_FIT_FITHELPERHIST_H

#include <map>
#include <vector>
#include <string>
#include <tuple>
#include <utility>
#include <memory>
#include <type_traits>
#include <regex>
#include <set>

#include <boost/property_tree/ptree.hpp>

#include "TH1D.h"
#include "TAxis.h"

#include "QualityControl/QcInfoLogger.h"

#include <FITCommon/HelperCommon.h>
namespace o2::quality_control_modules::fit
{

namespace helper
{

// Preparation for map axis -> map with bin labels
template <std::size_t NStep, std::size_t NArgLocal, std::size_t NAxis, typename TupleArgs>
inline void getMapBinLabelsPerAxis(std::map<unsigned int, std::map<unsigned int, std::string>>& mapAxisToMapBinLabels, const TupleArgs& tupleArgs)
{
  if constexpr (NStep < std::tuple_size_v<TupleArgs>) {
    using ElementType = typename std::tuple_element<NStep, TupleArgs>::type;
    const auto& arg = std::get<NStep>(tupleArgs);
    constexpr bool isArgMapBinLabels = std::is_same_v<std::decay_t<ElementType>, std::map<unsigned int, std::string>>;
    constexpr bool isArgTupleBins = std::is_same_v<std::decay_t<ElementType>, std::tuple<int, float, float>>;
    if constexpr (isArgMapBinLabels && NArgLocal == 0) { // map with bin labels
      mapAxisToMapBinLabels.insert({ NAxis, arg });
      getMapBinLabelsPerAxis<NStep + 1, 0, NAxis + 1>(mapAxisToMapBinLabels, tupleArgs);
    } else if constexpr (isArgTupleBins && NArgLocal == 0) { // tuple of bin params
      getMapBinLabelsPerAxis<NStep + 1, 0, NAxis + 1>(mapAxisToMapBinLabels, tupleArgs);
    } else if constexpr (!isArgMapBinLabels && NArgLocal < 2) { // first or second bin argument
      getMapBinLabelsPerAxis<NStep + 1, NArgLocal + 1, NAxis>(mapAxisToMapBinLabels, tupleArgs);
    } else if constexpr (!isArgMapBinLabels && NArgLocal == 2) { // third bin argument
      getMapBinLabelsPerAxis<NStep + 1, 0, NAxis + 1>(mapAxisToMapBinLabels, tupleArgs);
    } else {
      // error
      // static_assert();
    }
  }
}

// For unpacking std::map<unsigned int, std::string> into tuple of bin parameters
template <typename Arg>
inline auto unpackHistArg(const Arg& arg)
{
  if constexpr (std::is_same_v<std::decay_t<Arg>, std::map<unsigned int, std::string>>) {
    return std::make_tuple(static_cast<int>(arg.size()), float(0.0), static_cast<float>(arg.size()));
  } else if constexpr (std::is_same_v<std::decay_t<Arg>, std::tuple<int, float, float>>) {
    return arg;
  } else {
    return std::make_tuple(arg);
  }
}

// Makes hist with map bin->label(std::map<unsigned int, std::string>) as bin parameter
// Example:
// auto h1 = makeHist<TH2D>("hist","hist",10,0,10,map1)
// auto h2 = makeHist<TH1F>("hist2","hist2",map1)

template <typename HistType, typename... BinArgs>
inline auto makeHist(const std::string& name, const std::string& title, BinArgs&&... binArgs)
{

  const auto tupleArgs = std::tuple_cat(std::make_tuple(name, title), unpackHistArg(std::forward<BinArgs>(binArgs))...);
  auto ptr = std::apply(
    [](const std::string& name, const std::string& title, auto&&... args) {
      return std::move(std::make_unique<std::decay_t<HistType>>(name.c_str(), title.c_str(), std::forward<decltype(args)>(args)...));
    },
    tupleArgs);

  std::map<unsigned int, std::map<unsigned int, std::string>> mapAxisToMapBinLabels;
  getMapBinLabelsPerAxis<0, 0, 0>(mapAxisToMapBinLabels, std::make_tuple(std::forward<BinArgs>(binArgs)...));
  // Preparing labels
  auto getAxis = [](std::unique_ptr<std::decay_t<HistType>>& ptrHist, unsigned int axis) -> TAxis* {
    if (axis == 0) {
      return ptrHist->GetXaxis();
    } else if (axis == 1) {
      return ptrHist->GetYaxis();
    } else if (axis == 2) {
      return ptrHist->GetZaxis();
    }
    return nullptr;
  };
  auto makeLabels = [](TAxis* axis, const std::map<unsigned int, std::string>& mapBinLabels) {
    for (const auto& entry : mapBinLabels) {
      axis->SetBinLabel(entry.first + 1, entry.second.c_str());
    }
  };
  for (const auto& axisToMapBinLabels : mapAxisToMapBinLabels) {
    makeLabels(getAxis(ptr, axisToMapBinLabels.first), axisToMapBinLabels.second);
  }
  return std::move(ptr);
}

template <typename HistType, typename ManagerType, typename... BinArgs>
inline auto registerHist(ManagerType manager, const std::string& defaultDrawOption, const std::string& name, const std::string& title, BinArgs&&... binArgs)
{
  auto ptrHist = makeHist<HistType>(name, title, std::forward<BinArgs>(binArgs)...);
  manager->startPublishing(ptrHist.get());
  if (defaultDrawOption.size() > 0) {
    manager->setDefaultDrawOptions(ptrHist.get(), defaultDrawOption);
  }
  return std::move(ptrHist);
}

template <typename HistSrcType>
auto makeProj(const HistSrcType* histSrc,
              const std::string& name,
              const std::string& title,
              const std::pair<double, double>& rangeProj,
              int axis = 0 /*axis along which to do projection: 0 - xAxis; 1 - yAxis*/
              ) -> TH1D*
{
  if (axis != 0 || axis != 1) {
    return nullptr;
  }
  const TAxis* axisObj = axis == 0 ? histSrc->GetXaxis() : histSrc->GetYaxis();
  const auto binMin = axisObj->FindFixBin(rangeProj.first);
  const auto binMax = axisObj->FindFixBin(rangeProj.second);
  TH1D* histProj = ((TH1D*)(axis == 0 ? histSrc->ProjectionX(name.c_str(), binMin, binMax) : histSrc->ProjectionX(name.c_str(), binMin, binMax)));
  histProj->LabelsDeflate();
  histProj->SetTitle(title.c_str());
  return histProj;
}

template <typename HistSrcType, typename SetBinsType>
auto makePartialProj(const HistSrcType* histSrc,
                     const std::string& name,
                     const std::string& title,
                     const SetBinsType& setBinsToProj, // bins to participate in projection
                     const std::pair<double, double>& rangeProj,
                     int axis = 0 // axis along which to do projection: 0 - xAxis; 1 - yAxis
                     ) -> TH1D*
{
  if (axis != 0 || axis != 1) {
    return nullptr;
  }
  auto histTmp = std::unique_ptr<HistSrcType>((HistSrcType*)histSrc->Clone("histTmp"));
  for (int iBinX = 0; iBinX < histSrc->GetXaxis()->GetNbins() + 2; iBinX++) {
    for (int iBinY = 0; iBinY < histSrc->GetYaxis()->GetNbins() + 2; iBinY++) {
      bool isBinOutOfSet = false;
      if constexpr (std::is_same_v<std::decay_t<SetBinsType>, std::set<std::pair<int, int>>>) {
        isBinOutOfSet = (setBinsToProj.find(axis == 0 ? iBinX : iBinY) == setBinsToProj.end());
      } else if constexpr (std::is_same_v<std::decay_t<SetBinsType>, std::set<std::pair<int, int>>>) {
        isBinOutOfSet = (setBinsToProj.find({ iBinX, iBinY }) == setBinsToProj.end());
      } else {
        // assertion, shouldn't be compiled
      }
      if (isBinOutOfSet) {
        histTmp->SetBinContent(iBinX, iBinY, 0);
      }
    }
  }
  return makeProj(histTmp.get(), name, title, rangeProj, axis);
}

template <typename HistType, typename HistSrcType, typename... ArgsNom, typename... ArgsDen>
inline auto getRatioHistFrom2D(const HistSrcType* hist,
                               const std::string& name,
                               const std::string& title,
                               std::function<HistType*(const HistSrcType*, ArgsNom...)> funcNom,
                               const std::tuple<ArgsNom...>& argsNom,
                               std::function<HistType*(const HistSrcType*, ArgsDen...)> funcDen,
                               const std::tuple<ArgsDen...>& argsDen) -> HistType*
{
  std::unique_ptr<HistType> histNom(funcWithArgsAsTuple(funcNom, std::tuple_cat(std::tuple(hist), argsNom)));
  std::unique_ptr<HistType> histDen(funcWithArgsAsTuple(funcDen, std::tuple_cat(std::tuple(hist), argsDen)));
  std::unique_ptr<HistType> histResult((HistType*)histNom->Clone(name.c_str()));
  histResult->SetTitle(title.c_str());
  histResult->Divide(histDen);
  return std::move(histResult);
}

} // namespace helper
} // namespace o2::quality_control_modules::fit
#endif