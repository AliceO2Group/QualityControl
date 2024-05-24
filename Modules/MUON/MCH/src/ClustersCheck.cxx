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
/// \file   ClustersCheck.cxx
/// \author Andrea Ferrero
///

#include "MCH/ClustersCheck.h"
#include "MUONCommon/Helpers.h"
#include "QualityControl/MonitorObject.h"

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

// ROOT
#include <TH1.h>
#include <TProfile.h>
#include <TPolyLine.h>
#include <TPaveText.h>

using namespace std;
using namespace o2::quality_control;
using namespace o2::quality_control_modules::muon;

namespace o2::quality_control_modules::muonchambers
{

void ClustersCheck::configure()
{
}

void ClustersCheck::startOfActivity(const Activity& activity)
{
  mMinClustersPerTrack = getConfigurationParameter<double>(mCustomParameters, "minClustersPerTrack", mMinClustersPerTrack, activity);
  mMaxClustersPerTrack = getConfigurationParameter<double>(mCustomParameters, "maxClustersPerTrack", mMaxClustersPerTrack, activity);
  mMinClustersPerChamber = getConfigurationParameter<double>(mCustomParameters, "minClustersPerChamber", mMinClustersPerChamber, activity);
  mClustersPerChamberRangeMin = getConfigurationParameter<double>(mCustomParameters, "clustersPerChamberRangeMin", mClustersPerChamberRangeMin, activity);
  mClustersPerChamberRangeMax = getConfigurationParameter<double>(mCustomParameters, "clustersPerChamberRangeMax", mClustersPerChamberRangeMax, activity);

  mMinClusterSize = getConfigurationParameter<double>(mCustomParameters, "minClusterSize", mMinClusterSize, activity);
  mClusterSizeRangeMin = getConfigurationParameter<double>(mCustomParameters, "clusterSizeRangeMin", mClusterSizeRangeMin, activity);
  mClusterSizeRangeMax = getConfigurationParameter<double>(mCustomParameters, "clusterSizeRangeMax", mClusterSizeRangeMax, activity);

  mMarkerSize = getConfigurationParameter<float>(mCustomParameters, "markerSize", mMarkerSize, activity);

  mQualities.clear();
}

Quality ClustersCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    // number of clusters per track
    if (mo->getName().find("ClustersPerTrack") != std::string::npos) {
      TH1F* h = dynamic_cast<TH1F*>(mo->getObject());
      if (!h) {
        continue;
      }

      // quality flags initialization
      mQualities[mo->getName()] = Quality::Good;
      if (result == Quality::Null) {
        result.set(Quality::Good);
      }

      // check that the average number of clusters per tracks is within given limits
      if (h->GetMean() < mMinClustersPerTrack || h->GetMean() > mMaxClustersPerTrack) {
        result.set(Quality::Bad);
        result.addFlag(FlagTypeFactory::BadTracking(), "Number of clusters per track not in the expected range");
        mQualities[mo->getName()] = Quality::Bad;
      }
    }

    // average number of clusters per chamber
    if (mo->getName().find("ClustersPerChamber") != std::string::npos) {
      auto* h = dynamic_cast<TProfile*>(mo->getObject());
      if (!h) {
        continue;
      }

      mQualities[mo->getName()] = Quality::Good;
      if (result == Quality::Null) {
        result.set(Quality::Good);
      }

      for (int bin = 1; bin <= h->GetXaxis()->GetNbins(); bin++) {
        double nclus = h->GetBinContent(bin);
        if (nclus < mMinClustersPerChamber) {
          result.set(Quality::Bad);
          result.addFlag(FlagTypeFactory::BadTracking(), fmt::format("Too few clusters in CH{}", bin));
          mQualities[mo->getName()] = Quality::Bad;
        }
      }
    }

    // average cluster size per chamber
    if (mo->getName().find("ClusterSizePerChamber") != std::string::npos) {
      auto* h = dynamic_cast<TProfile*>(mo->getObject());
      if (!h) {
        continue;
      }

      mQualities[mo->getName()] = Quality::Good;
      if (result == Quality::Null) {
        result.set(Quality::Good);
      }

      for (int bin = 1; bin <= h->GetXaxis()->GetNbins(); bin++) {
        double nclus = h->GetBinContent(bin);
        if (nclus < mMinClusterSize) {
          result.set(Quality::Bad);
          result.addFlag(FlagTypeFactory::BadTracking(), fmt::format("Too small cluster size in CH{}", bin));
          mQualities[mo->getName()] = Quality::Bad;
        }
      }
    }
  }

  return result;
}

std::string ClustersCheck::getAcceptedType() { return "TH1"; }

// get the ROOT color code associated to a given quality level
static int getQualityColor(Quality q)
{
  int result = 0;
  if (q == Quality::Null) {
    result = kViolet - 6;
  }
  if (q == Quality::Bad) {
    result = kRed;
  }
  if (q == Quality::Medium) {
    result = kOrange - 3;
  }
  if (q == Quality::Good) {
    result = kGreen + 2;
  }

  return result;
}

// add a text label to display the quality associated with a plot
static void drawQualityLabel(TH1* h, const std::string& label, int color, float x1, float y1, float x2, float y2, int align)
{
  if (!h) {
    return;
  }

  auto* t = new TPaveText(x1, y1, x2, y2, "brNDC");
  t->AddText(label.c_str());
  t->SetBorderSize(0);
  t->SetFillStyle(0);
  t->SetTextAlign(align);
  t->SetTextColor(color);
  t->SetTextFont(42);
  h->GetListOfFunctions()->Add(t);
}

/// Add a marker to an histogram at a given position
/// The marker is draw with a TPolyLine such that it scales nicely with the size of the pad
/// The default dimensions of the marker are
/// * horizontal: 1/20 of the X-axis range
/// * vertical: 1/10 of the histogram values range
/// Parameters:
/// * histo: the histogram to which the marker is associated
/// * x0, y0: coordinates of the tip of the marker
/// * color: ROOT index of the marker fill color
/// * markerSize: overall scaling factor for the marker dimensions
static TPolyLine* addMarker(TH1& histo, double x0, double y0, int color, float markerSize)
{
  double xSize = (histo.GetXaxis()->GetXmax() - histo.GetXaxis()->GetXmin()) / 20;
  double ySize = (histo.GetMaximum() - histo.GetMinimum()) / 10;
  double xMarker[4] = { x0, x0 - xSize * markerSize / 2, x0 + xSize * markerSize / 2, x0 };
  double yMarker[4] = { y0, y0 + ySize * markerSize, y0 + ySize * markerSize, y0 };
  auto* m = new TPolyLine(4, xMarker, yMarker);
  m->SetNDC(kFALSE);
  m->SetFillColor(color);
  m->SetOption("f");
  m->SetLineWidth(0);
  histo.GetListOfFunctions()->Add(m);
  return m;
}

void ClustersCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  Quality moQuality = Quality::Null;
  if (mQualities.count(mo->getName()) > 0) {
    moQuality = mQualities[mo->getName()];
  }

  int color = getQualityColor(moQuality);

  if (mo->getName().find("ClustersPerTrack") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (!h) {
      return;
    }

    // get the maximum of the histogram
    double max = h->GetBinContent(h->GetMaximumBin());

    // adjust the vertical scale to provide some space on the top
    h->SetMinimum(0);
    h->SetMaximum(1.2 * max);

    // add vertical lines to display the acceptable range
    addVerticalLine(*h, mMinClustersPerTrack, color, kDashed, 1);
    addVerticalLine(*h, mMaxClustersPerTrack, color, kDashed, 1);

    // add a marker at the position of the histogram mean
    addMarker(*h, h->GetMean(), max * 1.05, color, mMarkerSize);

    // draw the quality flag associated to the plot
    drawQualityLabel(dynamic_cast<TH1*>(mo->getObject()), moQuality.getName(), color, 0.5, 0.75, 0.88, 0.88, 33);
  }

  if (mo->getName().find("ClustersPerChamber") != std::string::npos) {
    auto* h = dynamic_cast<TProfile*>(mo->getObject());
    if (!h) {
      return;
    }

    // adjust the vertical scale to provide some space on the top
    h->SetMinimum(std::min(h->GetBinContent(h->GetMinimumBin()),
                           mClustersPerChamberRangeMin));
    h->SetMaximum(std::max(h->GetBinContent(h->GetMaximumBin()),
                           mClustersPerChamberRangeMax));
    h->SetMarkerStyle(kCircle);
    h->SetDrawOption("EP");

    // draw horizontal threshold
    addHorizontalLine(*h, mMinClustersPerChamber, color, kDashed, 1);

    // draw the quality flag associated to the plot
    drawQualityLabel(dynamic_cast<TH1*>(mo->getObject()), moQuality.getName(), color, 0.5, 0.75, 0.88, 0.88, 33);
  }

  if (mo->getName().find("ClusterSizePerChamber") != std::string::npos) {
    auto* h = dynamic_cast<TProfile*>(mo->getObject());
    if (!h) {
      return;
    }

    // adjust the vertical scale to provide some space on the top
    h->SetMinimum(std::min(h->GetBinContent(h->GetMinimumBin()),
                           mClusterSizeRangeMin));
    h->SetMaximum(std::max(h->GetBinContent(h->GetMaximumBin()),
                           mClusterSizeRangeMax));
    h->SetMarkerStyle(kCircle);
    h->SetDrawOption("EP");

    // draw horizontal threshold
    addHorizontalLine(*h, mMinClusterSize, color, kDashed, 1);

    // draw the quality flag associated to the plot
    drawQualityLabel(dynamic_cast<TH1*>(mo->getObject()), moQuality.getName(), color, 0.5, 0.75, 0.88, 0.88, 33);
  }
}

} // namespace o2::quality_control_modules::muonchambers
