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

/// \file   HistoHelper.cxx
/// \author Diego Stocco

#include "MID/HistoHelper.h"

#include "TH1.h"
#include "TH2.h"
#include "TString.h"
#include "TLatex.h"
#include "TList.h"

namespace o2::quality_control_modules::mid
{

HistoHelper::HistoHelper()
{
  mColors[o2::quality_control::core::Quality::Null.getLevel()] = kWhite;
  mColors[o2::quality_control::core::Quality::Good.getLevel()] = kGreen;
  mColors[o2::quality_control::core::Quality::Medium.getLevel()] = kOrange;
  mColors[o2::quality_control::core::Quality::Bad.getLevel()] = kRed;
}

bool HistoHelper::normalizeHisto(TH1* histo, double scale) const
{
  if (mNTFs > 0) {
    histo->Scale(1. / (scale * getNTFsAsSeconds()));
    return true;
  }
  return false;
}

void HistoHelper::normalizeHistoToHz(TH1* histo) const
{
  if (normalizeHisto(histo, 1.)) {
    updateTitle(histo, "(Hz)");
  }
}

void HistoHelper::normalizeHistoTokHz(TH1* histo) const
{
  if (normalizeHisto(histo, 1000.)) {
    updateTitle(histo, "(kHz)");
  }
}

void HistoHelper::updateTitle(TH1* histo, std::string suffix) const
{
  if (!histo) {
    return;
  }
  TString title = histo->GetTitle();
  title.Append(" ");
  title.Append(suffix.c_str());
  histo->SetTitle(title);
}

std::string HistoHelper::getCurrentTime() const
{
  time_t t;
  time(&t);

  struct tm* tmp;
  tmp = localtime(&t);

  char timestr[500];
  strftime(timestr, sizeof(timestr), "(%x - %X)", tmp);

  std::string result = timestr;
  return result;
}

void HistoHelper::addLatex(TH1* histo, double xmin, double ymin, int color, std::string text) const
{

  TLatex* tl = new TLatex(xmin, ymin, Form("%s", text.c_str()));
  tl->SetNDC();
  tl->SetTextFont(22); // Normal 42
  tl->SetTextSize(0.06);
  tl->SetTextColor(color);
  histo->GetListOfFunctions()->Add(tl);
}

int HistoHelper::getColor(const o2::quality_control::core::Quality& quality) const
{
  auto found = mColors.find(quality.getLevel());
  if (found != mColors.end()) {
    return found->second;
  }
  return 1;
}

void HistoHelper::updateTitleWithNTF(TH1* histo) const
{
  updateTitle(histo, Form("TF=%lu", mNTFs));
}

} // namespace o2::quality_control_modules::mid