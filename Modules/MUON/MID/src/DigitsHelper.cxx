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

/// \file   DigitsHelper.h
/// \author Diego Stocco

#include "MID/DigitsHelper.h"

#include <fmt/format.h>
#include "TH1.h"
#include "TH2.h"
#include "MIDBase/DetectorParameters.h"
#include "MIDBase/GeometryParameters.h"
#include "MIDGlobalMapping/GlobalMapper.h"

namespace o2::quality_control_modules::mid
{
DigitsHelper::DigitsHelper()
{
  mStripsMap.clear();
  mStripsInfo.clear();

  o2::mid::GlobalMapper gm;
  auto infos = gm.buildStripsInfo();

  for (size_t idx = 0; idx < infos.size(); ++idx) {
    mStripsMap[infos[idx].id] = idx;
    StripInfo si;
    si.deId = infos[idx].deId;
    si.columnId = infos[idx].columnId;
    si.lineId = infos[idx].lineId;
    si.stripId = infos[idx].stripId;
    si.cathode = infos[idx].cathode;
    si.xwidth = std::abs(infos[idx].xwidth);
    si.ywidth = infos[idx].ywidth;
    mStripsInfo.emplace_back(si);
  }

  for (int ide = 0; ide < o2::mid::detparams::NDetectionElements; ++ide) {
    for (int icol = gm.getMapping().getFirstColumn(ide); icol < 7; ++icol) {
      int idx = getColumnIdx(icol, ide);
      mColumnInfo[idx].firstLine = gm.getMapping().getFirstBoardBP(icol, ide);
      mColumnInfo[idx].lastLine = gm.getMapping().getLastBoardBP(icol, ide);
      mColumnInfo[idx].nStripsNB = gm.getMapping().getNStripsNBP(icol, ide);
    }
  }
}

TH1F DigitsHelper::makeStripHisto(const std::string name, const std::string title) const
{
  TH1F histo(name.c_str(), title.c_str(), mStripsInfo.size(), 0, mStripsInfo.size());
  histo.SetXTitle("Strip index");
  return histo;
}

unsigned long DigitsHelper::countDigits(const o2::mid::ColumnData& col, int cathode) const
{
  unsigned long counts = 0;
  int firstCathode = 0;
  int lastCathode = 1;
  if (cathode == 0 || cathode == 1) {
    firstCathode = lastCathode = cathode;
  }
  int lastLine = (cathode == 1) ? 0 : 3;
  for (int icath = firstCathode; icath <= lastCathode; ++icath) {
    for (int iline = 0; iline <= lastLine; ++iline) {
      for (int istrip = 0; istrip < 16; ++istrip) {
        if (col.isStripFired(istrip, icath, iline)) {
          ++counts;
        }
      }
    }
  }
  return counts;
}

TH2F DigitsHelper::makeStripMapHisto(std::string name, std::string title, int cathode) const
{
  int nBinsX = (cathode == 0) ? 14 : 224;
  int nBinsY = (cathode == 0) ? 64 * o2::mid::detparams::NRPCLines : 4 * o2::mid::detparams::NRPCLines;
  TH2F histo = TH2F(name.c_str(), title.c_str(), nBinsX, -7, 7, nBinsY, 0., 9.);
  histo.SetXTitle("Column");
  histo.SetYTitle("Line");
  histo.SetOption("COLZ");
  histo.SetStats(0);
  return histo;
}

TH2F DigitsHelper::makeBoardMapHisto(std::string name, std::string title) const
{
  TH2F histo = TH2F(name.c_str(), title.c_str(), 14, -7., 7., 4 * o2::mid::detparams::NRPCLines, 0., 9.);
  histo.SetXTitle("Column");
  histo.SetYTitle("Line");
  histo.SetOption("COLZ");
  histo.SetStats(0);
  return histo;
}

void DigitsHelper::fillMapHistos(const StripInfo& info, TH2* stripHistoB, TH2* stripHistoNB, TH2* boardHisto, int wgt) const
{
  int irpc = o2::mid::detparams::getRPCLine(info.deId);
  bool isRightSide = o2::mid::detparams::isRightSide(info.deId);

  int idx = getColumnIdx(info.columnId, info.deId);

  int firstLine = mColumnInfo[idx].firstLine;
  int lastLine = mColumnInfo[idx].lastLine;

  int xbinOffsetBoard = 7 - info.columnId;
  if (isRightSide) {
    xbinOffsetBoard = 15 - xbinOffsetBoard;
  }
  int ybinOffsetBoard = 4 * irpc + 1;

  if (info.cathode == 0) {
    // BP
    int ybinOffsetStrip = 64 * irpc + 1;
    int pitchB = info.ywidth;
    int iline = info.lineId;
    int istrip = info.stripId;
    for (int ibin = 0; ibin < pitchB; ++ibin) {
      stripHistoB->AddBinContent(stripHistoB->GetBin(xbinOffsetBoard, ybinOffsetStrip + pitchB * (16 * iline + istrip) + ibin), wgt);
      stripHistoB->SetEntries(stripHistoB->GetEntries() + wgt);
    }
    if (boardHisto) {
      for (int ibin = 0; ibin < pitchB; ++ibin) {
        boardHisto->AddBinContent(boardHisto->GetBin(xbinOffsetBoard, ybinOffsetBoard + pitchB * iline + ibin), wgt);
        boardHisto->SetEntries(boardHisto->GetEntries() + wgt);
      }
    }
  } else {
    // NBP
    int xbinOffsetStrip = 16 * (xbinOffsetBoard - 1) + 1;
    int pitchNB = info.xwidth;
    if (info.columnId == 6) {
      pitchNB = 2;
    }
    if (o2::mid::geoparams::isShortRPC(info.deId) && info.columnId == 1) {
      if (isRightSide) {
        xbinOffsetStrip += 8;
      }
    }
    if (lastLine != 2) {
      lastLine = 3;
    }
    int binWidth = pitchNB / 2;
    int istrip = info.stripId;
    for (int iline = firstLine; iline <= lastLine; ++iline) {
      for (int ibin = 0; ibin < binWidth; ++ibin) {
        stripHistoNB->AddBinContent(stripHistoNB->GetBin(xbinOffsetStrip + binWidth * istrip + ibin, ybinOffsetBoard + iline), wgt);
        stripHistoNB->SetEntries(stripHistoNB->GetEntries() + wgt);
      }
      if (boardHisto) {
        boardHisto->AddBinContent(boardHisto->GetBin(xbinOffsetBoard, ybinOffsetBoard + iline), wgt);
        boardHisto->SetEntries(boardHisto->GetEntries() + wgt);
      }
    }
  }
}

void DigitsHelper::fillMapHistos(const TH1* stripHisto, std::array<std::unique_ptr<TH2F>, 4>& stripHistosB, std::array<std::unique_ptr<TH2F>, 4>& stripHistosNB, std::array<std::unique_ptr<TH2F>, 5>& boardHistos) const
{
  for (int ibin = 1, nBins = stripHisto->GetNbinsX(); ibin <= nBins; ++ibin) {
    auto& info = mStripsInfo[ibin - 1];
    int ch = o2::mid::detparams::getChamber(info.deId);
    fillMapHistos(info, stripHistosB[ch].get(), stripHistosNB[ch].get(), boardHistos[ch].get(), stripHisto->GetBinContent(ibin));
  }
  boardHistos[4]->Reset();
  for (size_t ich = 0; ich < 4; ++ich) {
    boardHistos[4]->Add(boardHistos[ich].get());
  }
}

void DigitsHelper::fillMapHistos(const TH1* stripHisto, std::array<std::unique_ptr<TH2F>, 4>& stripHistosB, std::array<std::unique_ptr<TH2F>, 4>& stripHistosNB) const
{
  for (int ibin = 1, nBins = stripHisto->GetNbinsX(); ibin <= nBins; ++ibin) {
    auto& info = mStripsInfo[ibin - 1];
    int ch = o2::mid::detparams::getChamber(info.deId);
    fillMapHistos(info, stripHistosB[ch].get(), stripHistosNB[ch].get(), nullptr, stripHisto->GetBinContent(ibin));
  }
}

void DigitsHelper::fillMapHistos(const TH1* stripHisto, TH2* stripHistosB, TH2* stripHistosNB, TH2* boardHistos, int chamber) const
{
  for (int ibin = 1, nBins = stripHisto->GetNbinsX(); ibin <= nBins; ++ibin) {
    auto& info = mStripsInfo[ibin - 1];
    int ch = o2::mid::detparams::getChamber(info.deId);
    if (ch != chamber) {
      continue;
    }
    fillMapHistos(info, stripHistosB, stripHistosNB, boardHistos, stripHisto->GetBinContent(ibin));
  }
}

void DigitsHelper::fillStripHisto(const o2::mid::ColumnData& col, TH1* histo) const
{
  int idx = getColumnIdx(col.columnId, col.deId);
  int firstLine = mColumnInfo[idx].firstLine;
  int lastLine = mColumnInfo[idx].lastLine;
  for (int iline = firstLine; iline <= lastLine; ++iline) {
    for (int istrip = 0; istrip < 16; ++istrip) {
      if (col.isBPStripFired(istrip, iline)) {
        auto found = mStripsMap.find(o2::mid::getStripId(col.deId, col.columnId, iline, istrip, 0));
        // The histogram bin corresponds to the strip index + 1
        // Since we know the bin, we can use AddBinContent instead of Fill to save time
        histo->AddBinContent(found->second + 1);
        histo->SetEntries(histo->GetEntries() + 1);
      }
    }
  }
  int nStrips = mColumnInfo[idx].nStripsNB;
  for (int istrip = 0; istrip < nStrips; ++istrip) {
    if (col.isNBPStripFired(istrip)) {
      auto found = mStripsMap.find(o2::mid::getStripId(col.deId, col.columnId, firstLine, istrip, 1));
      // The histogram bin corresponds to the strip index + 1
      // Since we know the bin, we can use AddBinContent instead of Fill to save time
      histo->AddBinContent(found->second + 1);
      histo->SetEntries(histo->GetEntries() + 1);
    }
  }
}

} // namespace o2::quality_control_modules::mid