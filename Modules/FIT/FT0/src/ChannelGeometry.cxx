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
/// \file   ChannelGeometryPlot.cxx
/// \author Artur Furs afurs@cern.ch
///
#include "QualityControl/QcInfoLogger.h"
#include "FT0/ChannelGeometry.h"
#include <ROOT/RCsvDS.hxx>
#include <Framework/Logger.h>
#include <map>
#include <unordered_map>
#include <cstdlib>
#include <memory>
#include <iostream>

namespace o2::quality_control_modules::ft0
{
void ChannelGeometry::parseChannelTable(const std::string& filepath, char delimiter)
{
  clear();
  try {
    std::unordered_map<std::string, char> colTypes = { { "Long signal cable #", 'T' } };
    auto dataframe = ROOT::RDF::FromCSV(filepath.c_str(), true, delimiter, -1LL, std::move(colTypes));
    dataframe.Foreach([&channelGeometryMap = this->mChannelGeometryMap](const Long64_t& chID, const double& x, const double& y) {
      channelGeometryMap.insert({ static_cast<int>(chID), { x, y } });
    },
                      { "channel #", "coordinate X in mm", "coordinate Y in mm" });

    for (const auto& [chID, point] : mChannelGeometryMap) {
      // temporary hardcoded
      const auto& x = point.first;
      const auto& y = point.second;
      makeChannel(chID, x, y);
    }
  } catch (std::exception const& e) {
    mIsOk = false;
    LOGP(error, "FT0 channel map arsing error: {}", e.what());
  }
}

void ChannelGeometry::makeChannel(int chID, double x, double y)
{
  // For further development
  std::array<double, 4> x_borders = { x - mMargin, x + mMargin, x + mMargin, x - mMargin };
  std::array<double, 4> y_borders = { y + mMargin, y + mMargin, y - mMargin, y - mMargin };
  // side temporary hardcoded for FT0, should be taken from csv
  if (chID < 96) {
    const auto bin = mHistSideA->AddBin(4, x_borders.data(), y_borders.data());
    mChannelMapA.insert({ chID, static_cast<int>(bin) });
  } else if (chID < 208) {
    const auto bin = mHistSideC->AddBin(4, x_borders.data(), y_borders.data());
    mChannelMapC.insert({ chID, static_cast<int>(bin) });
  }
}

void ChannelGeometry::initHists(double xMin, double xMax, double yMin, double yMax)
{
  mHistSideA = std::make_unique<ChannelGeometry::Hist_t>("hDummyGeometryFT0A", "hDummyGeometryFT0A", xMin, xMax, yMin, yMax);
  mHistSideC = std::make_unique<ChannelGeometry::Hist_t>("hDummyGeometryFT0C", "hDummyGeometryFT0C", xMin, xMax, yMin, yMax);
}

void ChannelGeometry::init(double xMin, double xMax, double yMin, double yMax, double margin)
{
  mMargin = margin;
  initHists(xMin, xMax, yMin, yMax);
  const auto& filepath = ChannelGeometry::getFilepath();
  parseChannelTable(filepath);
}

std::unique_ptr<ChannelGeometry::Hist_t> ChannelGeometry::makeHistSideA(const std::string& histName, const std::string& histTitle)
{
  std::unique_ptr<ChannelGeometry::Hist_t> histPtr(dynamic_cast<ChannelGeometry::Hist_t*>(mHistSideA->Clone(histName.c_str())));
  histPtr->SetTitle(histTitle.c_str());
  return std::move(histPtr);
}

std::unique_ptr<ChannelGeometry::Hist_t> ChannelGeometry::makeHistSideC(const std::string& histName, const std::string& histTitle)
{
  std::unique_ptr<ChannelGeometry::Hist_t> histPtr(dynamic_cast<ChannelGeometry::Hist_t*>(mHistSideC->Clone(histName.c_str())));
  histPtr->SetTitle(histTitle.c_str());
  return std::move(histPtr);
}
void ChannelGeometry::setBinContent(Hist_t* histSideA, Hist_t* histSideC, int chID, double val)
{
  const auto itSideA = mChannelMapA.find(chID);
  const auto itSideC = mChannelMapC.find(chID);
  if (histSideA && itSideA != mChannelMapA.end()) {
    histSideA->SetBinContent(itSideA->second, val);
  } else if (histSideC && itSideC != mChannelMapC.end()) {
    histSideC->SetBinContent(itSideC->second, val);
  }
}

void ChannelGeometry::clear()
{
  mChannelGeometryMap.clear();
  mChannelMapA.clear();
  mChannelMapA.clear();
  mHistSideA->Reset("");
  mHistSideC->Reset("");
  mIsOk = true;
}

} // namespace o2::quality_control_modules::ft0