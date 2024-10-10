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
  BigScreenElement(std::string name, int index, float padding, float labelOffset, int borderWidth,
                   int foregroundColor)
    : mLabel(padding, 1.0 - padding, name.c_str()),
      mPave(padding, padding, 1.0 - padding, 1.0 - padding - labelOffset, "NB NDC"),
      mBox(padding, padding, 1.0 - padding, 1.0 - padding - labelOffset),
      mPadIndex(index + 1)
  {
    mLabel.SetNDC();
    mLabel.SetTextAlign(11);
    mLabel.SetTextSize(0.2);
    mLabel.SetTextFont(42);
    mLabel.SetTextColor(foregroundColor);

    mPave.SetBorderSize(0);
    mPave.SetFillColor(kWhite);

    mBox.SetLineColor(kBlack);
    mBox.SetLineWidth(borderWidth);
    mBox.SetFillStyle(0);
  }

  void DrawInCanvas(TPad* c)
  {
    c->cd(mPadIndex);
    mPave.Draw();
    mBox.Draw();
    mLabel.Draw();
  }

  TText mLabel;
  TPaveText mPave;
  TBox mBox;
  int mPadIndex;
};

BigScreenCanvas::BigScreenCanvas(std::string name, std::string title, int nRows, int nCols, int borderWidth,
                                 int foregroundColor, int backgroundColor)
  : TCanvas(name.c_str(), title.c_str(), 800, 600), mNRows(nRows), mNCols(nCols), mBorderWidth(borderWidth), mForegroundColor(foregroundColor), mBackgroundColor(backgroundColor)
{
  mColors[Quality::Null.getName()] = kViolet - 6;
  mColors[Quality::Bad.getName()] = kRed;
  mColors[Quality::Medium.getName()] = kOrange - 3;
  mColors[Quality::Good.getName()] = kGreen + 2;

  // TPad filling the whole canvas and used to draw the background color
  mBackgoundPad = std::make_shared<TPad>((name + "_pad").c_str(), (title + "_pad").c_str(), 0, 0, 1, 1);
  mBackgoundPad->SetBorderSize(0);
  mBackgoundPad->SetBorderMode(0);
  mBackgoundPad->SetMargin(0, 0, 0, 0);
  mBackgoundPad->SetFillColor(mBackgroundColor);
  mBackgoundPad->Draw();
}

void BigScreenCanvas::addBox(std::string boxName, int index)
{
  mBoxes[boxName] = std::make_shared<BigScreenElement>(boxName, index, mPadding, mLabelOffset, mBorderWidth, mForegroundColor);
}

void BigScreenCanvas::setText(std::string boxName, int color, std::string text)
{
  auto index = mBoxes.find(boxName);
  if (index == mBoxes.end()) {
    return;
  }
  auto& pave = index->second->mPave;
  pave.Clear();
  pave.SetFillColor(color);
  pave.AddText((std::string(" ") + text + std::string(" ")).c_str());
}

void BigScreenCanvas::setQuality(std::string boxName, Quality quality)
{
  auto color = mColors.find(quality.getName());
  if (color != mColors.end()) {
    setText(boxName, color->second, quality.getName());
  }
}

void BigScreenCanvas::update()
{
  mBackgoundPad->Clear();
  mBackgoundPad->Divide(mNCols, mNRows, 0, 0);

  // set the sub-pads as fully transparent to show the color of the main backgound pad
  for (int padIndex = 1; padIndex <= (mNCols * mNRows); padIndex++) {
    mBackgoundPad->cd(padIndex);
    gPad->SetFillStyle(4000);
  }

  for (auto& [key, box] : mBoxes) {
    box->DrawInCanvas(mBackgoundPad.get());
  }
}

} // namespace o2::quality_control_modules::common
