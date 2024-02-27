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
/// \file    RelValTask.h
/// \author  Andrea Ferrero
/// \brief   Quality post-processing task that generates a canvas showing the aggregated quality of each system
///

#include "Common/BigScreenCanvas.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::common
{

BigScreenCanvas::BigScreenCanvas(std::string name, std::string title, int nRows, int nCols, int borderWidth)
  : TCanvas(name.c_str(), title.c_str(), 800, 600), mNRows(nRows), mNCols(nCols), mBorderWidth(borderWidth)
{
  mColors[Quality::Null.getName()] = kViolet - 6;
  mColors[Quality::Bad.getName()] = kRed;
  mColors[Quality::Medium.getName()] = kOrange - 3;
  mColors[Quality::Good.getName()] = kGreen + 2;
}

void BigScreenCanvas::add(std::string name, int index)
{
  float dx = 1.0 / mNCols;
  float dy = 1.0 / mNRows;
  float paveHalfWidth = 0.3 * dx;
  float paveHalfHeight = 0.3 * dy;

  auto row = index / mNCols;
  auto col = index % mNCols;

  if (row >= mNRows) {
    return;
  }
  // put first pave on top-left
  row = mNRows - row - 1;

  // coordinates of the center of the pave area
  float xc = dx * (0.5 + col);
  float yc = dy * (0.5 + row) - 0.01;

  // coordinates of the pave corners
  float x1 = xc - paveHalfWidth;
  float x2 = xc + paveHalfWidth;
  float y1 = yc - paveHalfHeight;
  float y2 = yc + paveHalfHeight;

  // label coordinates
  float xl = x1;
  float yl = y2 + 0.01;

  // add pave associated to the detector
  mPaves[name] = std::make_unique<TPaveText>(x1, y1, x2, y2);
  auto& p = mPaves[name];
  p->SetOption("");
  p->SetLineWidth(mBorderWidth);

  // add label with detector status
  mLabels[name] = std::make_unique<TText>(xl, yl, name.c_str());
  auto& l = mLabels[name];
  l->SetNDC();
  l->SetTextAlign(11);
  l->SetTextSize(0.05);
  l->SetTextFont(42);
}

void BigScreenCanvas::set(std::string name, int color, std::string text)
{
  auto pave = mPaves[name].get();
  if (!pave) {
    return;
  }
  auto label = mLabels[name].get();
  if (!label) {
    return;
  }

  pave->Clear();
  pave->SetFillColor(color);
  pave->AddText((std::string(" ") + text + std::string(" ")).c_str());
}

void BigScreenCanvas::set(std::string name, Quality quality)
{
  int color = mColors[quality.getName()];
  set(name, color, quality.getName());
}

void BigScreenCanvas::update()
{
  Clear();
  cd();

  for (auto& p : mPaves) {
    p.second->Draw();
  }
  for (auto& l : mLabels) {
    l.second->Draw();
  }
}

} // namespace o2::quality_control_modules::common
