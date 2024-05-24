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

#include "MUONCommon/TracksCheck.h"
#include "MUONCommon/Helpers.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"
#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

// ROOT
#include <TLine.h>
#include <TPaveText.h>
#include <TPolyLine.h>
#include <TH1.h>
#include <array>

using namespace o2::quality_control;

namespace o2::quality_control_modules::muon
{

void TracksCheck::configure()
{
}

void TracksCheck::startOfActivity(const Activity& activity)
{
  mMinTracksPerTF = getConfigurationParameter<int>(mCustomParameters, "minTracksPerTF", mMinTracksPerTF, activity);
  mMaxTracksPerTF = getConfigurationParameter<int>(mCustomParameters, "maxTracksPerTF", mMaxTracksPerTF, activity);
  mMaxDeltaPhi = getConfigurationParameter<float>(mCustomParameters, "maxDeltaPhi", mMaxDeltaPhi, activity);
  mMaxChargeAsymmetry = getConfigurationParameter<float>(mCustomParameters, "maxChargeAsymmetry", mMaxChargeAsymmetry, activity);
  mMarkerSize = getConfigurationParameter<float>(mCustomParameters, "markerSize", mMarkerSize, activity);

  mQualities.clear();
}

void TracksCheck::endOfActivity(const Activity& activity)
{
}

Quality TracksCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  for (auto& [moName, mo] : *moMap) {

    // number of tracks per TF
    if (mo->getName().find("TracksPerTF") != std::string::npos) {
      TH1* h = dynamic_cast<TH1*>(mo->getObject());
      if (!h) {
        continue;
      }

      // quality flags initialization
      mQualities[mo->getName()] = Quality::Good;
      if (result == Quality::Null) {
        result.set(Quality::Good);
      }

      // check that the average number of tracks per TF is within given limits
      if (h->GetMean() < mMinTracksPerTF || h->GetMean() > mMaxTracksPerTF) {
        result.set(Quality::Bad);
        result.addFlag(FlagTypeFactory::BadTracking(), "Number of tracks per TF not in the expected range");
        mQualities[mo->getName()] = Quality::Bad;
      }
    }

    // azimuthal distribution of tracks
    if (mo->getName().find("TrackPhi") != std::string::npos) {
      TH1* h = dynamic_cast<TH1*>(mo->getObject());
      if (!h) {
        continue;
      }

      // quality flags initialization
      mQualities[mo->getName()] = Quality::Good;
      if (result == Quality::Null) {
        result.set(Quality::Good);
      }

      // compute the average number of entries in each quadrant, as well as the global average
      float quadrantNBins = (float)(h->GetXaxis()->GetNbins()) / 4;
      int quadrantFirstBin[4]{
        (int)(quadrantNBins * 2), // Q0
        (int)(quadrantNBins * 3), // Q1
        0,                        // Q2
        (int)(quadrantNBins)      // Q3
      };
      int quadrantLastBin[4]{
        (int)(quadrantNBins * 3) - 1, // Q0
        h->GetXaxis()->GetNbins(),    // Q1
        (int)(quadrantNBins)-1,       // Q2
        (int)(quadrantNBins * 2) - 1  // Q3
      };

      std::array<double, 4> quadrantAverage;
      double globalAverage = 0;
      for (int q = 0; q < 4; q++) {
        quadrantAverage[q] = 0;
        for (int i = quadrantFirstBin[q]; i <= quadrantLastBin[q]; i++) {
          quadrantAverage[q] += h->GetBinContent(i);
          globalAverage += h->GetBinContent(i);
        }
        quadrantAverage[q] /= quadrantLastBin[q] - quadrantFirstBin[q] + 1;
      }
      globalAverage /= h->GetXaxis()->GetNbins();

      // Check that the relative deviation between each quadrant is below a given threshold
      // Set the quality to Bad if any pair of quadrants deviates more than the threshold
      for (int qi = 0; qi < 4; qi++) {
        for (int qj = qi + 1; qj < 4; qj++) {
          auto delta = std::fabs(quadrantAverage[qi] - quadrantAverage[qj]) / globalAverage;
          if (delta > mMaxDeltaPhi) {
            result.set(Quality::Bad);
            result.addFlag(FlagTypeFactory::BadTracking(), fmt::format("large deviation in phi distribution {:.2f} > {:.2f}", delta, mMaxDeltaPhi));
            mQualities[mo->getName()] = Quality::Bad;
          }
        }
      }
    }

    // distribution of charge-over-pT
    if (mo->getName().find("TrackQOverPt") != std::string::npos) {
      TH1* h = dynamic_cast<TH1*>(mo->getObject());
      if (!h) {
        continue;
      }

      // quality flags initialization
      mQualities[mo->getName()] = Quality::Good;
      if (result == Quality::Null) {
        result.set(Quality::Good);
      }

      // compute the charge asymmetry as the relative difference
      // between the total number of positive and negative tracks
      int zeroBin = h->GetXaxis()->FindBin((double)(0));
      double negIntegral = h->Integral(1, zeroBin - 1);
      double posIntegral = h->Integral(zeroBin, h->GetXaxis()->GetNbins());
      double chargeRatio = (posIntegral > 0) ? negIntegral / posIntegral : 0;
      double chargeAsymmetry = std::fabs(chargeRatio - 1);

      // check that the charge asymmetry is below a given threshold
      if (chargeAsymmetry > mMaxChargeAsymmetry) {
        result.set(Quality::Bad);
        result.addFlag(FlagTypeFactory::BadTracking(), fmt::format("large charge asymmetry {:.2f} > {:.2f}", chargeAsymmetry, mMaxChargeAsymmetry));
        mQualities[mo->getName()] = Quality::Bad;
      }
    }
  }

  return result;
}

std::string TracksCheck::getAcceptedType() { return "TH1"; }

static void updateTitle(TH1* hist, std::string suffix)
{
  if (!hist) {
    return;
  }
  TString title = hist->GetTitle();
  title.Append(" ");
  title.Append(suffix.c_str());
  hist->SetTitle(title);
}

static std::string getCurrentTime()
{
  time_t t;
  time(&t);

  struct tm* tmp;
  tmp = localtime(&t);

  char timestr[500];
  strftime(timestr, sizeof(timestr), "(%d/%m/%Y - %R)", tmp);

  std::string result = timestr;
  return result;
}

static int getQualityColor(Quality q)
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

// add a text label to an histogram, using a TPaveText to properly scale the text with the canvas size
// The x1,y1,x2,y2 parameters specify the corners of the pave
// The text is drawn with the ROOT color index specified in the color parameter
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
/// * logx, logy: wether the X or Y axis are in logarithmic scale
static TPolyLine* addMarker(TH1& histo, double x0, double y0, int color, float markerSize, bool logx, bool logy)
{
  double xmin = logx ? std::log10(histo.GetXaxis()->GetXmin()) : histo.GetXaxis()->GetXmin();
  double xmax = logx ? std::log10(histo.GetXaxis()->GetXmax()) : histo.GetXaxis()->GetXmax();
  double xSize = (xmax - xmin) / 20;
  double ySize = (histo.GetMaximum() - histo.GetMinimum()) / 10;
  double x1 = logx ? std::pow(10, std::log10(x0) - xSize * markerSize / 2) : x0 - xSize * markerSize / 2;
  double x2 = logx ? std::pow(10, std::log10(x0) + xSize * markerSize / 2) : x0 + xSize * markerSize / 2;
  double xMarker[4] = { x0, x1, x2, x0 };
  double yMarker[4] = { y0, y0 + ySize * markerSize, y0 + ySize * markerSize, y0 };
  auto* m = new TPolyLine(4, xMarker, yMarker);
  m->SetNDC(kFALSE);
  m->SetFillColor(color);
  m->SetOption("f");
  m->SetLineWidth(0);
  histo.GetListOfFunctions()->Add(m);
  return m;
}

void TracksCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality /*checkResult*/)
{
  auto currentTime = getCurrentTime();
  updateTitle(dynamic_cast<TH1*>(mo->getObject()), currentTime);

  Quality moQuality = Quality::Null;
  if (mQualities.count(mo->getName()) > 0) {
    moQuality = mQualities[mo->getName()];
  }

  int color = getQualityColor(moQuality);

  if (mo->getName().find("TracksPerTF") != std::string::npos) {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (h == nullptr) {
      return;
    }

    // get the maximum of the histogram
    double max = h->GetBinContent(h->GetMaximumBin());

    // adjust the vertical scale to provide some space on the top
    h->SetMinimum(0);
    h->SetMaximum(1.2 * max);

    // add vertical lines to display the acceptable range
    addVerticalLine(*h, mMinTracksPerTF, color, kDashed, 1);
    addVerticalLine(*h, mMaxTracksPerTF, color, kDashed, 1);

    // add a marker at the position of the histogram mean
    addMarker(*h, h->GetMean(), max * 1.05, color, mMarkerSize, true, false);

    // draw the quality flag associated to the plot
    drawQualityLabel(dynamic_cast<TH1*>(mo->getObject()), moQuality.getName(), color, 0.5, 0.75, 0.88, 0.88, 33);
  }

  if (mo->getName().find("TrackPhi") != std::string::npos) {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (h == nullptr) {
      return;
    }

    // adjust the vertical scale to provide some space on the top
    h->SetMinimum(0);
    h->SetMaximum(1.2 * h->GetMaximum());

    // draw the quality flag associated to the plot
    drawQualityLabel(dynamic_cast<TH1*>(mo->getObject()), moQuality.getName(), color, 0.5, 0.75, 0.88, 0.88, 33);

  } else if (mo->getName().find("TrackQOverPt") != std::string::npos) {
    auto* h = dynamic_cast<TH1*>(mo->getObject());
    if (h == nullptr) {
      return;
    }

    h->SetMinimum(0);

    // draw the quality flag associated to the plot
    drawQualityLabel(h, moQuality.getName(), color, 0.5, 0.75, 0.88, 0.88, 33);
  }
}

} // namespace o2::quality_control_modules::muon
