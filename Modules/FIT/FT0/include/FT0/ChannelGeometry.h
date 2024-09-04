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
/// \file   ChannelGeometry.h
/// \author Artur Furs afurs@cern.ch
///

#ifndef QC_MODULE_FT0_CHANNELGEOMETRY_H
#define QC_MODULE_FT0_CHANNELGEOMETRY_H

#include <TH2Poly.h>

#include <map>
#include <memory>
#include <string>
namespace o2::quality_control_modules::ft0
{

class ChannelGeometry
{
 public:
  ChannelGeometry() = default;
  ~ChannelGeometry() = default;
  typedef TH2Poly Hist_t;
  typedef std::map<int, int> ChannelMap_t;   // chID -> bin
  typedef std::pair<double, double> Point_t; // X/y coordinates
  typedef std::map<int, Point_t> ChannelGeometryMap_t;

  ChannelGeometryMap_t mChannelGeometryMap{};
  ChannelMap_t mChannelMapA{}; // A-side
  ChannelMap_t mChannelMapC{}; // C-side
  double mMargin{ 10. };       // margin between channels
  void parseChannelTable(const std::string& filepath, char delimeter = ';');
  void makeChannel(int chID, double x, double y);
  void initHists(double xMin, double xMax, double yMin, double yMax);
  void init(double xMin, double xMax, double yMin, double yMax, double margin);
  void clear();

  std::unique_ptr<Hist_t> makeHistSideA(const std::string& histName, const std::string& histTitle);
  std::unique_ptr<Hist_t> makeHistSideC(const std::string& histName, const std::string& histTitle);
  void setBinContent(Hist_t* histSideA, Hist_t* histSideC, int chID, double val);

  template <typename HistSrcType>
  void convertHist1D(HistSrcType* histSrc, Hist_t* histSideA, Hist_t* histSideC)
  {
    for (int iBin = 0; iBin < histSrc->GetNbinsX(); iBin++) {
      const auto val = histSrc->GetBinContent(iBin + 1);
      setBinContent(histSideA, histSideC, iBin, val);
    }
  }

  static std::string getFilepath(const std::string& filename = "FT0_LUT.csv")
  {
    const auto pathEnv = std::getenv("QUALITYCONTROL_ROOT");
    const std::string subfilepath = "/Modules/FIT/FT0/etc/" + filename;
    if (pathEnv) {
      return pathEnv + subfilepath;
    }
    return std::string{ "" };
  }

 private:
  std::unique_ptr<Hist_t> mHistSideA; //! hist template for A-side, use Clone()
  std::unique_ptr<Hist_t> mHistSideC; //! hist template for C-side, use Clone()
  bool mIsOk{ true };
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_CHANNELGEOMETRY_H
