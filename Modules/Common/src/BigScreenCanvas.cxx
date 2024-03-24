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
/// \file    BigScreenCanvas.cxx
/// \author  Andrea Ferrero
/// \brief   Canvas showing the aggregated quality of configurable groups
///

#include "Common/BigScreenCanvas.h"
#include <TPaveText.h>
#include <TText.h>
#include <TBox.h>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::common
{

struct BigScreenElement {
  BigScreenElement(std::string name, int index, float padding, float labelOffset, int borderWidth)
    : mLabel(padding, 1.0 - padding, name.c_str()),
      mPave(padding, padding, 1.0 - padding, 1.0 - padding - labelOffset, "NB NDC"),
      mBox(padding, padding, 1.0 - padding, 1.0 - padding - labelOffset),
      mPadIndex(index + 1)
  {
    mLabel.SetNDC();
    mLabel.SetTextAlign(11);
    mLabel.SetTextSize(0.2);
    mLabel.SetTextFont(42);

    mPave.SetBorderSize(0);
    mPave.SetFillColor(kWhite);

    mBox.SetLineColor(kBlack);
    mBox.SetLineWidth(borderWidth);
    mBox.SetFillStyle(0);
  }
  TText mLabel;
  TPaveText mPave;
  TBox mBox;
  int mPadIndex;
};

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
  mElements[name] = std::make_shared<BigScreenElement>(name, index, mPadding, mLabelOffset, mBorderWidth);
}

void BigScreenCanvas::set(std::string name, int color, std::string text)
{
  auto index = mElements.find(name);
  if (index == mElements.end()) {
    return;
  }
  auto& pave = index->second->mPave;
  pave.Clear();
  pave.SetFillColor(color);
  pave.AddText((std::string(" ") + text + std::string(" ")).c_str());
}

void BigScreenCanvas::set(std::string name, Quality quality)
{
  auto color = mColors.find(quality.getName());
  if (color != mColors.end()) {
    set(name, color->second, quality.getName());
  }
}

void BigScreenCanvas::update()
{
  Clear();
  Divide(mNCols, mNRows);

  for (auto& [key, element] : mElements) {
    cd(element->mPadIndex);
    element->mPave.Draw();
    element->mBox.Draw();
    element->mLabel.Draw();
  }
}

} // namespace o2::quality_control_modules::common
