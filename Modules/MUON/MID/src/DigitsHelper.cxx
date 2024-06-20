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
  initMaps();
}

void DigitsHelper::initMaps()
{
  o2::mid::GlobalMapper gm;
  auto infos = gm.buildStripsInfo();

  auto stripHistoB = makeStripMapHisto("templateStripB", "templateStripB", 0);
  auto stripHistoNB = makeStripMapHisto("templateStripNB", "templateStripNB", 1);
  auto boardHisto = makeBoardMapHisto("templateBoard", "templateBoard");

  for (auto it = infos.begin(), end = infos.end(); it != end; ++it) {
    auto stripIdx = std::distance(infos.begin(), it);

    mStripsMap[it->id] = stripIdx;

    int irpc = o2::mid::detparams::getRPCLine(it->deId);
    bool isRightSide = o2::mid::detparams::isRightSide(it->deId);

    int firstLine = gm.getMapping().getFirstBoardBP(it->columnId, it->deId);
    int lastLine = gm.getMapping().getLastBoardBP(it->columnId, it->deId);

    int xbinOffsetBoard = 7 - it->columnId;
    if (isRightSide) {
      xbinOffsetBoard = 15 - xbinOffsetBoard;
    }
    int ybinOffsetBoard = 4 * irpc + 1;

    std::vector<int> stripMapBins;
    std::vector<int> boardMapBins;

    if (it->cathode == 0) {
      // BP
      int ybinOffsetStrip = 64 * irpc + 1;
      int pitchB = it->ywidth;
      int iline = it->lineId;
      int istrip = it->stripId;
      for (int ibin = 0; ibin < pitchB; ++ibin) {
        stripMapBins.emplace_back(stripHistoB.GetBin(xbinOffsetBoard, ybinOffsetStrip + pitchB * (16 * iline + istrip) + ibin));
      }
      for (int ibin = 0; ibin < pitchB; ++ibin) {
        boardMapBins.emplace_back(boardHisto.GetBin(xbinOffsetBoard, ybinOffsetBoard + pitchB * iline + ibin));
      }
    } else {
      // NBP
      int xbinOffsetStrip = 16 * (7 + it->columnId) + 1;
      int pitchNB = std::abs(it->xwidth);
      if (it->columnId == 6) {
        pitchNB = 2;
      }
      if (o2::mid::geoparams::isShortRPC(it->deId) && it->columnId == 1) {
        xbinOffsetStrip += 8;
      }
      if (lastLine != 2) {
        lastLine = 3;
      }
      int binWidth = pitchNB / 2;
      int istrip = it->stripId;
      for (int iline = firstLine; iline <= lastLine; ++iline) {
        for (int ibin = 0; ibin < binWidth; ++ibin) {
          int xbinStrip = xbinOffsetStrip + binWidth * istrip + ibin;
          if (!isRightSide) {
            xbinStrip = stripHistoNB.GetNbinsX() - xbinStrip + 1;
          }
          stripMapBins.emplace_back(stripHistoNB.GetBin(xbinStrip, ybinOffsetBoard + iline));
        }
        boardMapBins.emplace_back(boardHisto.GetBin(xbinOffsetBoard, ybinOffsetBoard + iline));
      }
    }
    MapInfo info;
    info.cathode = it->cathode;
    info.chamber = o2::mid::detparams::getChamber(it->deId);
    info.bins = stripMapBins;
    mStripIdxToStripMap.emplace_back(info);
    info.bins = boardMapBins;
    mStripIdxToBoardMap.emplace_back(info);
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
  TH1F histo(name.c_str(), title.c_str(), mStripsMap.size(), 0, mStripsMap.size());
  histo.SetXTitle("Strip index");
  return histo;
}

std::array<std::unique_ptr<TH2F>, 4> DigitsHelper::makeStripMapHistos(std::string name, std::string title, int cathode) const
{
  std::array<std::unique_ptr<TH2F>, 4> out;
  std::array<std::string, 4> chId{ "11", "12", "21", "22" };
  for (int ich = 0; ich < 4; ++ich) {
    out[ich] = std::make_unique<TH2F>(makeStripMapHisto(fmt::format("{}{}", name, chId[ich]), fmt::format("{} MT{}", title, chId[ich]), cathode));
  }
  return out;
}

std::array<std::unique_ptr<TH2F>, 4> DigitsHelper::makeBoardMapHistos(std::string name, std::string title) const
{
  std::array<std::unique_ptr<TH2F>, 4> out;
  std::array<std::string, 4> chId{ "11", "12", "21", "22" };
  for (int ich = 0; ich < 4; ++ich) {
    out[ich] = std::make_unique<TH2F>(makeBoardMapHisto(fmt::format("{}{}", name, chId[ich]), fmt::format("{} MT{}", title, chId[ich])));
  }
  return out;
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
  TH2F histo(name.c_str(), title.c_str(), nBinsX, -7, 7, nBinsY, 0., 9.);
  histo.SetXTitle("Column");
  histo.SetYTitle("Line");
  histo.SetOption("COLZ");
  histo.SetStats(0);
  return histo;
}

TH2F DigitsHelper::makeBoardMapHisto(std::string name, std::string title) const
{
  TH2F histo(name.c_str(), title.c_str(), 14, -7., 7., 4 * o2::mid::detparams::NRPCLines, 0., 9.);
  histo.SetXTitle("Column");
  histo.SetYTitle("Line");
  histo.SetOption("COLZ");
  histo.SetStats(0);
  return histo;
}

void DigitsHelper::fillMapHistos(const TH1* histo, std::array<std::unique_ptr<TH2F>, 4>& histoMapB, std::array<std::unique_ptr<TH2F>, 4>& histoMapNB, const std::vector<MapInfo>& infoMap) const
{
  for (int ibin = 1, nBins = histo->GetNbinsX(); ibin <= nBins; ++ibin) {
    auto idx = ibin - 1;
    auto histoMap = infoMap[idx].cathode == 0 ? histoMapB[infoMap[idx].chamber].get() : histoMapNB[infoMap[idx].chamber].get();
    auto wgt = histo->GetBinContent(ibin);
    for (auto& ib : infoMap[idx].bins) {
      FillBin(histoMap, ib, wgt);
    }
  }
}

void DigitsHelper::fillStripMapHistos(const TH1* histo, std::array<std::unique_ptr<TH2F>, 4>& histosB, std::array<std::unique_ptr<TH2F>, 4>& histosNB) const
{
  fillMapHistos(histo, histosB, histosNB, mStripIdxToStripMap);
}

void DigitsHelper::fillBoardMapHistosFromStrips(const TH1* histo, std::array<std::unique_ptr<TH2F>, 4>& histosB, std::array<std::unique_ptr<TH2F>, 4>& histosNB) const
{
  fillMapHistos(histo, histosB, histosNB, mStripIdxToBoardMap);
}

void DigitsHelper::FillBin(TH1* histo, int ibin, double wgt) const
{
  histo->AddBinContent(ibin, wgt);
  histo->SetEntries(histo->GetEntries() + wgt);
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
        FillBin(histo, found->second + 1);
      }
    }
  }
  int nStrips = mColumnInfo[idx].nStripsNB;
  for (int istrip = 0; istrip < nStrips; ++istrip) {
    if (col.isNBPStripFired(istrip)) {
      auto found = mStripsMap.find(o2::mid::getStripId(col.deId, col.columnId, firstLine, istrip, 1));
      // The histogram bin corresponds to the strip index + 1
      // Since we know the bin, we can use AddBinContent instead of Fill to save time
      FillBin(histo, found->second + 1);
    }
  }
}

} // namespace o2::quality_control_modules::mid