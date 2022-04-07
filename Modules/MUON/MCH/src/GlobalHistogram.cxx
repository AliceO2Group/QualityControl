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
/// \file   PhysicsTask.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#include <iostream>
#include <TLine.h>
#include <TList.h>

#include <fmt/format.h>

#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingInterface/CathodeSegmentation.h"
#ifdef MCH_HAS_MAPPING_FACTORY
#include "MCHMappingFactory/CreateSegmentation.h"
#endif
#include "MCHMappingSegContour/CathodeSegmentationContours.h"

#include "MCH/GlobalHistogram.h"

#define GLOBAL_HIST_SCALE 5

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

std::string getHistoPath(int deId)
{
  return fmt::format("ST{}/DE{}/", (deId - 100) / 200 + 1, deId);
}

int getDEindex(int deId)
{
  // CH 1 - 4 DE
  int DEmin = 100;
  int offset = 0;
  if (deId < (DEmin + 100)) {
    return (deId - DEmin);
  }
  offset += 5;

  // CH 2 - 4 DE
  DEmin = 200;
  if (deId < (DEmin + 100)) {
    return (deId - DEmin + offset);
  }
  offset += 5;

  // CH 3 - 4 DE
  DEmin = 300;
  if (deId < (DEmin + 100)) {
    return (deId - DEmin + offset);
  }
  offset += 5;

  // CH 4 - 4 DE
  DEmin = 400;
  if (deId < (DEmin + 100)) {
    return (deId - DEmin + offset);
  }
  offset += 5;

  // CH 5 - 18 DE
  DEmin = 500;
  if (deId < (DEmin + 100)) {
    return (deId - DEmin + offset);
  }
  offset += 19;

  // CH 6 - 18 DE
  DEmin = 600;
  if (deId < (DEmin + 100)) {
    return (deId - DEmin + offset);
  }
  offset += 19;

  // CH 7 - 26 DE
  DEmin = 700;
  if (deId < (DEmin + 100)) {
    return (deId - DEmin + offset);
  }
  offset += 27;

  // CH 8 - 26 DE
  DEmin = 800;
  if (deId < (DEmin + 100)) {
    return (deId - DEmin + offset);
  }
  offset += 27;

  // CH 9 - 26 DE
  DEmin = 900;
  if (deId < (DEmin + 100)) {
    return (deId - DEmin + offset);
  }
  offset += 27;

  // CH 10 - 26 DE
  DEmin = 1000;
  if (deId < (DEmin + 100)) {
    return (deId - DEmin + offset);
  }

  return -1;
}

int getDEindexMax()
{
  return getDEindex(1025);
}

static int getLR(int de)
{
  int lr = -1;
  int deId = de % 100;
  if ((de >= 100) && (de < 500)) {
    if ((deId >= 1) && (deId <= 2)) {
      // ST12 left
      lr = 0;
    } else {
      lr = 1;
    }
  }
  if ((de >= 500) && (de < 700)) {
    if (((deId >= 0) && (deId <= 4)) || ((deId >= 14) && (deId <= 17))) {
      // ST3 right
      lr = 1;
    }
    if (((deId >= 5) && (deId <= 13))) {
      // ST3 left
      lr = 0;
    }
  }
  if ((de >= 700) && (de < 1100)) {
    if (((deId >= 0) && (deId <= 6)) || ((deId >= 20) && (deId <= 25))) {
      // ST4/5 right
      lr = 1;
    }
    if (((deId >= 7) && (deId <= 19))) {
      // ST4/5 left
      lr = 0;
    }
  }
  return lr;
}

static int getBT(int de)
{
  int bt = -1;
  int deId = de % 100;
  if ((de >= 100) && (de < 500)) {
    if ((deId == 0) || (deId == 1)) {
      // ST12 top
      bt = 1;
    } else {
      bt = 0;
    }
  }
  return bt;
}

static int getDetectorHistWidth(int deId)
{
  if (deId >= 500) {
    return (40 * 6 + 20);
  } else if (deId >= 300) {
    return 130;
  } else {
    return 100;
  }
  return 10;
}

static int getDetectorHistXbins(int deId)
{
  return (getDetectorHistWidth(deId) * 2);
}

static int getDetectorHistHeight(int deId)
{
  if (deId >= 500) {
    return (50);
  } else if (deId >= 300) {
    return 120;
  } else {
    return 100;
  }
  return 10;
}

static int getDetectorHistYbins(int deId)
{
  return (getDetectorHistHeight(deId) * 2);
}

static bool getDetectorFlipX(int deId)
{
  int lr = getLR(deId);
  bool flip = (lr == 1);

  if (deId >= 700) {
    int mod = (deId % 100);
    if (mod == 0 || mod == 2 || mod == 3 || mod == 4 || mod == 9 || mod == 10 || mod == 11 || mod == 13 || mod == 15 || mod == 16 || mod == 17 || mod == 21 || mod == 22 || mod == 23 || mod == 24) {
      flip = !flip;
    }
  } else if (deId >= 500) {
    int mod = (deId % 100);
    if (mod == 2 || mod == 7 || mod == 11 || mod == 16) {
      flip = !flip;
    }
  } else {
    flip = (lr == 0);
  }

  return flip;
}

static bool getDetectorFlipY(int deId)
{
  if (deId >= 100 && deId < 500) {
    int bt = getBT(deId);
    if (bt == 0) {
      return true;
    }
  }
  if (deId == 510 || deId == 517 || deId == 610 || deId == 617) {
    return true;
  }
  if (deId >= 700) {
    if ((deId % 100) == 14) {
      return true;
    }
    if ((deId % 100) == 25) {
      return true;
    }
  }
  return false;
}

static float getDetectorShiftX(int deId)
{
  if (deId < 500) {
    return 5;
  }

  return 0;
}

static float getDetectorShiftY(int deId)
{
  if (deId < 500) {
    return 5;
  }

  return 0;
}

DetectorHistogram::DetectorHistogram(TString name, TString title, int deId, int cathode)
  : mName(name), mTitle(title), mDeId(deId), mCathode(cathode), mFlipX(getDetectorFlipX(deId)), mFlipY(getDetectorFlipY(deId)), mShiftX(getDetectorShiftX(deId)), mShiftY(getDetectorShiftY(deId))
{
  mHist = std::make_pair(new TH2F(name, title, getNbinsX(), getXmin(), getXmax(), getNbinsY(), getYmin(), getYmax()), true);
  addContour();
  mHist.first->SetOption("colz");
}

DetectorHistogram::DetectorHistogram(TString name, TString title, int deId, int cathode, TH2F* hist)
  : mName(name), mTitle(title), mDeId(deId), mCathode(cathode), mFlipX(getDetectorFlipX(deId)), mFlipY(getDetectorFlipY(deId)), mShiftX(getDetectorShiftX(deId)), mShiftY(getDetectorShiftY(deId))
{
  mHist = std::make_pair(hist, false);
  init();
  addContour();
  mHist.first->SetOption("colz");
}

DetectorHistogram::~DetectorHistogram()
{
  if (mHist.first && mHist.second) {
    delete mHist.first;
  }
}

int DetectorHistogram::getNbinsX()
{
  return getDetectorHistXbins(mDeId);
}

int DetectorHistogram::getNbinsY()
{
  return getDetectorHistYbins(mDeId);
}

float DetectorHistogram::getXmin()
{
  if (mDeId < 500) {
    if (getDetectorFlipX(mDeId)) {
      return -1.0 * getDetectorHistWidth(mDeId);
    } else {
      return 0;
    }
  }

  return -1.0 * getDetectorHistWidth(mDeId) / 2;
}

float DetectorHistogram::getXmax()
{
  if (mDeId < 500) {
    if (getDetectorFlipX(mDeId)) {
      return 0;
    } else {
      return getDetectorHistWidth(mDeId);
    }
  }

  return getDetectorHistWidth(mDeId) / 2;
}

float DetectorHistogram::getYmin()
{
  if (mDeId < 500) {
    if (getDetectorFlipY(mDeId)) {
      return -1.0 * getDetectorHistHeight(mDeId);
    } else {
      return 0;
    }
  }

  return -1.0 * getDetectorHistHeight(mDeId) / 2;
}

float DetectorHistogram::getYmax()
{
  if (mDeId < 500) {
    if (getDetectorFlipY(mDeId)) {
      return 0;
    } else {
      return getDetectorHistHeight(mDeId);
    }
  }

  return getDetectorHistHeight(mDeId) / 2;
}

void DetectorHistogram::init()
{
  mHist.first->Reset();

  mHist.first->SetNameTitle(mName, mTitle);

  mHist.first->GetXaxis()->Set(getNbinsX(), getXmin(), getXmax());
  mHist.first->GetYaxis()->Set(getNbinsY(), getYmin(), getYmax());
  mHist.first->SetBinsLength();
}

void DetectorHistogram::addContour()
{
  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(mDeId);
  const o2::mch::mapping::CathodeSegmentation& csegment = (mCathode == 0) ? segment.bending() : segment.nonBending();
  o2::mch::contour::Contour<double> envelop = o2::mch::mapping::getEnvelop(csegment);
  std::vector<o2::mch::contour::Vertex<double>> vertices = envelop.getVertices();

  TLine* line{ nullptr };

  for (unsigned int vi = 0; vi < vertices.size(); vi++) {
    o2::mch::contour::Vertex<double> v1 = vertices[vi];
    o2::mch::contour::Vertex<double> v2 = (vi < (vertices.size() - 1)) ? vertices[vi + 1] : vertices[0];

    v1.x += mShiftX;
    v2.x += mShiftX;
    v1.y += mShiftY;
    v2.y += mShiftY;

    if (mFlipX) {
      v1.x *= -1;
      v2.x *= -1;
    }
    if (mFlipY) {
      v1.y *= -1;
      v2.y *= -1;
    }

    line = new TLine(v1.x, v1.y, v2.x, v2.y);
    mHist.first->GetListOfFunctions()->Add(line);
    line = new TLine(v1.x, v1.y, v2.x, v2.y);
    mHist.first->GetListOfFunctions()->Add(line);
  }
}

void DetectorHistogram::Fill(double padX, double padY, double padSizeX, double padSizeY, double val)
{
  if (!mHist.first) {
    return;
  }

  padX += mShiftX;
  padY += mShiftY;

  if (mFlipX) {
    padX *= -1.0;
  }
  if (mFlipY) {
    padY *= -1.0;
  }

  int binx_min = mHist.first->GetXaxis()->FindBin(padX - padSizeX / 2 + 0.1);
  int binx_max = mHist.first->GetXaxis()->FindBin(padX + padSizeX / 2 - 0.1);
  int biny_min = mHist.first->GetYaxis()->FindBin(padY - padSizeY / 2 + 0.1);
  int biny_max = mHist.first->GetYaxis()->FindBin(padY + padSizeY / 2 - 0.1);
  for (int by = biny_min; by <= biny_max; by++) {
    float y = mHist.first->GetYaxis()->GetBinCenter(by);
    for (int bx = binx_min; bx <= binx_max; bx++) {
      float x = mHist.first->GetXaxis()->GetBinCenter(bx);
      mHist.first->Fill(x, y, val);
    }
  }
}

void DetectorHistogram::Set(double padX, double padY, double padSizeX, double padSizeY, double val)
{
  if (!mHist.first) {
    return;
  }

  padX += mShiftX;
  padY += mShiftY;

  if (mFlipX) {
    padX *= -1.0;
  }
  if (mFlipY) {
    padY *= -1.0;
  }

  int binx_min = mHist.first->GetXaxis()->FindBin(padX - padSizeX / 2 + 0.1);
  int binx_max = mHist.first->GetXaxis()->FindBin(padX + padSizeX / 2 - 0.1);
  int biny_min = mHist.first->GetYaxis()->FindBin(padY - padSizeY / 2 + 0.1);
  int biny_max = mHist.first->GetYaxis()->FindBin(padY + padSizeY / 2 - 0.1);
  for (int by = biny_min; by <= biny_max; by++) {
    for (int bx = binx_min; bx <= binx_max; bx++) {
      mHist.first->SetBinContent(bx, by, val);
    }
  }
}

static float getGlobalHistDeWidth(int id)
{
  float result{ 0 };

  switch (id) {
    case 0:
      result = 130;
      break;
    case 1:
      result = 250;
      break;
  }

  return result;
}

static float getGlobalHistDeHeight(int id)
{
  float result{ 0 };

  switch (id) {
    case 0:
      result = 130;
      break;
    case 1:
      result = 60;
      break;
  }

  return result;
}

static int getNHistPerChamberX(int /* id */)
{
  return 2;
}

static int getNHistPerChamberY(int id)
{
  return (id == 0) ? 2 : 13;
}

static int getNStations(int id)
{
  int nStations = (id == 0) ? 2 : 3;
  return nStations;
}

static float getGlobalHistWidth(int id)
{
  constexpr int chambersPerStation = 2;

  return (getGlobalHistDeWidth(id) * getNHistPerChamberX(id) * chambersPerStation * getNStations(id));
}

static float getGlobalHistHeight(int id)
{
  return (getGlobalHistDeHeight(id) * getNHistPerChamberY(id) * 2);
}

GlobalHistogram::GlobalHistogram(std::string name, std::string title, int id) : mName(name), mTitle(title), mId(id)
{
  auto hist = new TH2F(name.c_str(), title.c_str(), getGlobalHistWidth(id) / GLOBAL_HIST_SCALE, 0, getGlobalHistWidth(id),
                       getGlobalHistHeight(id) / GLOBAL_HIST_SCALE, 0, getGlobalHistHeight(id));
  mHist = std::make_pair(hist, true);
  mHist.first->SetOption("colz");
}

GlobalHistogram::GlobalHistogram(std::string name, std::string title, int id, TH2F* hist) : mName(name), mTitle(title), mId(id)
{
  mHist = std::make_pair(hist, false);
  mHist.first->SetOption("colz");
}

GlobalHistogram::~GlobalHistogram()
{
  if (mHist.first && mHist.second) {
    delete mHist.first;
  }
}

void GlobalHistogram::init()
{
  mHist.first->Reset();

  mHist.first->SetNameTitle(mName, mTitle);

  mHist.first->GetXaxis()->Set(getGlobalHistWidth(mId) / GLOBAL_HIST_SCALE, 0, getGlobalHistWidth(mId));
  mHist.first->GetYaxis()->Set(getGlobalHistHeight(mId) / GLOBAL_HIST_SCALE, 0, getGlobalHistHeight(mId));
  mHist.first->SetBinsLength();

  switch (mId) {
    case 0:
      initST12();
      break;
    case 1:
      initST345();
      break;
  }
}

void GlobalHistogram::initST12()
{
  std::vector<int> allDE;

  auto addDE = [&allDE](int detElemId) {
    allDE.push_back(detElemId);
  };
  o2::mch::mapping::forEachDetectionElement(addDE);

  TLine* line;

  float histWidth = getHist()->GetXaxis()->GetXmax();
  float histHeight = getHist()->GetYaxis()->GetXmax();
  float stationWidth = histWidth / 2;

  line = new TLine(0, histHeight / 2, histWidth, histHeight / 2);
  getHist()->GetListOfFunctions()->Add(line);

  line = new TLine(stationWidth, 0, stationWidth, histHeight);
  getHist()->GetListOfFunctions()->Add(line);

  for (auto& de : allDE) {
    if (de >= 500) {
      continue;
    }
    const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);

    std::vector<o2::mch::contour::Vertex<double>> vertices[2];
    const o2::mch::mapping::CathodeSegmentation& csegmentB = segment.bending();
    o2::mch::contour::Contour<double> envelopB = o2::mch::mapping::getEnvelop(csegmentB);
    vertices[0] = envelopB.getVertices();

    const o2::mch::mapping::CathodeSegmentation& csegmentNB = segment.nonBending();
    o2::mch::contour::Contour<double> envelopNB = o2::mch::mapping::getEnvelop(csegmentNB);
    vertices[1] = envelopNB.getVertices();

    float x0[2], y0[2], xNB0, yNB0;
    getDeCenter(de, x0[0], y0[0], x0[1], y0[1]);
    bool flipX = getDetectorFlipX(de);
    bool flipY = getDetectorFlipY(de);

    float shiftX = getDetectorShiftX(de);
    float shiftY = getDetectorShiftY(de);

    for (unsigned int ci = 0; ci < 2; ci++) {
      for (unsigned int vi = 0; vi < vertices[ci].size(); vi++) {
        o2::mch::contour::Vertex<double> v1 = vertices[ci][vi];
        o2::mch::contour::Vertex<double> v2 = (vi < (vertices[ci].size() - 1)) ? vertices[ci][vi + 1] : vertices[ci][0];

        v1.x += shiftX;
        v1.y += shiftY;
        v2.x += shiftX;
        v2.y += shiftY;

        if (flipX) {
          v1.x *= -1;
          v2.x *= -1;
        }

        if (flipY) {
          v1.y *= -1;
          v2.y *= -1;
        }

        line = new TLine(v1.x + x0[ci], v1.y + y0[ci], v2.x + x0[ci], v2.y + y0[ci]);
        getHist()->GetListOfFunctions()->Add(line);
      }
    }
  }
}

void GlobalHistogram::initST345()
{
  std::vector<int> allDE;

  auto addDE = [&allDE](int detElemId) {
    allDE.push_back(detElemId);
  };
  o2::mch::mapping::forEachDetectionElement(addDE);

  TLine* line;

  float histWidth = getHist()->GetXaxis()->GetXmax();
  float histHeight = getHist()->GetYaxis()->GetXmax();
  float stationWidth = histWidth / 3;

  line = new TLine(0, histHeight / 2, histWidth, histHeight / 2);
  getHist()->GetListOfFunctions()->Add(line);

  line = new TLine(stationWidth, 0, stationWidth, histHeight);
  getHist()->GetListOfFunctions()->Add(line);

  line = new TLine(stationWidth * 2, 0, stationWidth * 2, histHeight);
  getHist()->GetListOfFunctions()->Add(line);

  for (auto& de : allDE) {
    if (de < 500) {
      continue;
    }
    const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);
    const o2::mch::mapping::CathodeSegmentation& csegment = segment.bending();
    o2::mch::contour::Contour<double> envelop = o2::mch::mapping::getEnvelop(csegment);
    std::vector<o2::mch::contour::Vertex<double>> vertices = envelop.getVertices();

    float xB0, yB0, xNB0, yNB0;
    getDeCenter(de, xB0, yB0, xNB0, yNB0);
    bool isR = getLR(de) == 1;
    bool flipY = getDetectorFlipY(de);

    for (unsigned int vi = 0; vi < vertices.size(); vi++) {
      o2::mch::contour::Vertex<double> v1 = vertices[vi];
      o2::mch::contour::Vertex<double> v2 = (vi < (vertices.size() - 1)) ? vertices[vi + 1] : vertices[0];

      if (flipY) {
        v1.y *= -1;
        v2.y *= -1;
      }

      if (isR) {
        line = new TLine(-v1.x + xB0, v1.y + yB0, -v2.x + xB0, v2.y + yB0);
        getHist()->GetListOfFunctions()->Add(line);
        line = new TLine(-v1.x + xNB0, v1.y + yNB0, -v2.x + xNB0, v2.y + yNB0);
        getHist()->GetListOfFunctions()->Add(line);
      } else {
        line = new TLine(v1.x + xB0, v1.y + yB0, v2.x + xB0, v2.y + yB0);
        getHist()->GetListOfFunctions()->Add(line);
        line = new TLine(v1.x + xNB0, v1.y + yNB0, v2.x + xNB0, v2.y + yNB0);
        getHist()->GetListOfFunctions()->Add(line);
      }
    }
  }
}

void GlobalHistogram::getDeCenter(int de, float& xB0, float& yB0, float& xNB0, float& yNB0)
{
  xB0 = 0;
  yB0 = 0;
  xNB0 = 0;
  yNB0 = 0;

  if ((de >= 100) && (de < 500)) {
    getDeCenterST12(de, xB0, yB0, xNB0, yNB0);
  }

  if ((de >= 500) && (de < 700)) {
    getDeCenterST3(de, xB0, yB0, xNB0, yNB0);
  }

  if ((de >= 700) && (de < 900)) {
    getDeCenterST4(de, xB0, yB0, xNB0, yNB0);
  }

  if ((de >= 900) && (de < 1100)) {
    getDeCenterST5(de, xB0, yB0, xNB0, yNB0);
  }
}

void GlobalHistogram::getDeCenterST12(int de, float& xB0, float& yB0, float& xNB0, float& yNB0)
{
  float deWidth = getGlobalHistDeWidth(mId);
  float deHeight = getGlobalHistDeHeight(mId);

  int yOffset = 0;
  // DE index within the chamber
  int deId = de % 100;

  int xId = getLR(de), yId = -1;

  switch (deId) {
    case 0:
    case 1:
      yId = 1;
      break;
    case 2:
    case 3:
      yId = 0;
      break;
  }

  yId += yOffset;

  xB0 = xNB0 = deWidth;

  yB0 = yNB0 = deHeight;
  yB0 += getNHistPerChamberY(mId) * deHeight;

  int chamber = de / 100;
  if ((chamber % 2) == 0) {
    xB0 += getNHistPerChamberX(mId) * deWidth;
    xNB0 += getNHistPerChamberX(mId) * deWidth;
  }

  int station = (de - 100) / 200 + 1;
  if (de >= 300) {
    xB0 += getNHistPerChamberX(mId) * deWidth * 2;
    xNB0 += getNHistPerChamberX(mId) * deWidth * 2;
  }

  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);
  if ((&segment) == nullptr) {
    return;
  }
  const o2::mch::mapping::CathodeSegmentation& csegment = segment.bending();
  o2::mch::contour::Contour<double> envelop = o2::mch::mapping::getEnvelop(csegment);
  std::vector<o2::mch::contour::Vertex<double>> vertices = envelop.getVertices();
  o2::mch::contour::BBox<double> bbox = o2::mch::mapping::getBBox(csegment);
}

void GlobalHistogram::getDeCenterST3(int de, float& xB0, float& yB0, float& xNB0, float& yNB0)
{
  float deWidth = getGlobalHistDeWidth(mId);
  float deHeight = getGlobalHistDeHeight(mId);

  int yOffset = 2;
  // DE index within the chamber
  int deId = de % 100;

  int xId = getLR(de), yId = -1;

  switch (deId) {
    case 13:
    case 14:
      yId = 0;
      break;
    case 12:
    case 15:
      yId = 1;
      break;
    case 11:
    case 16:
      yId = 2;
      break;
    case 10:
    case 17:
      yId = 3;
      break;
    case 9:
    case 0:
      yId = 4;
      break;
    case 8:
    case 1:
      yId = 5;
      break;
    case 7:
    case 2:
      yId = 6;
      break;
    case 6:
    case 3:
      yId = 7;
      break;
    case 5:
    case 4:
      yId = 8;
      break;
  }

  yId += yOffset;

  xB0 = xNB0 = deWidth * xId + deWidth / 2;

  yB0 = yNB0 = deHeight * yId + deHeight / 2;
  yB0 += getNHistPerChamberY(mId) * deHeight;

  int chamber = de / 100;
  if ((chamber % 2) == 0) {
    xB0 += getNHistPerChamberX(mId) * deWidth;
    xNB0 += getNHistPerChamberX(mId) * deWidth;
  }

  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);
  if ((&segment) == nullptr) {
    return;
  }
  const o2::mch::mapping::CathodeSegmentation& csegment = segment.bending();
  o2::mch::contour::Contour<double> envelop = o2::mch::mapping::getEnvelop(csegment);
  std::vector<o2::mch::contour::Vertex<double>> vertices = envelop.getVertices();
  o2::mch::contour::BBox<double> bbox = o2::mch::mapping::getBBox(csegment);

  double xmax = bbox.xmax();
  if (xId == 0) {
    // ST3 left, shift all detectors to the right
    xB0 += 120 - xmax;
    xNB0 += 120 - xmax;
    if (deId == 9) {
      xB0 -= 22.5;
      xNB0 -= 22.5;
    }
  }
  if (xId == 1) {
    // ST3 right, shift all detectors to the left
    xB0 -= 120 - xmax;
    xNB0 -= 120 - xmax;
    if (deId == 0) {
      xB0 += 22.5;
      xNB0 += 22.5;
    }
  }
}

void GlobalHistogram::getDeCenterST4(int de, float& xB0, float& yB0, float& xNB0, float& yNB0)
{
  float deWidth = getGlobalHistDeWidth(mId);
  float deHeight = getGlobalHistDeHeight(mId);

  int yOffset = 0;
  // DE index within the chamber
  int deId = de % 100;

  int xId = getLR(de), yId = -1;

  switch (deId) {
    case 19:
    case 20:
      yId = 0;
      break;
    case 18:
    case 21:
      yId = 1;
      break;
    case 17:
    case 22:
      yId = 2;
      break;
    case 16:
    case 23:
      yId = 3;
      break;
    case 15:
    case 24:
      yId = 4;
      break;
    case 14:
    case 25:
      yId = 5;
      break;
    case 13:
    case 0:
      yId = 6;
      break;
    case 12:
    case 1:
      yId = 7;
      break;
    case 11:
    case 2:
      yId = 8;
      break;
    case 10:
    case 3:
      yId = 9;
      break;
    case 9:
    case 4:
      yId = 10;
      break;
    case 8:
    case 5:
      yId = 11;
      break;
    case 7:
    case 6:
      yId = 12;
      break;
  }

  yId += yOffset;

  xB0 = xNB0 = deWidth * xId + deWidth / 2 + getNHistPerChamberX(mId) * 2 * deWidth;

  yB0 = yNB0 = deHeight * yId + deHeight / 2;
  yB0 += getNHistPerChamberY(mId) * deHeight;

  int chamber = de / 100;
  if ((chamber % 2) == 0) {
    xB0 += getNHistPerChamberX(mId) * deWidth;
    xNB0 += getNHistPerChamberX(mId) * deWidth;
  }

  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);
  if ((&segment) == nullptr) {
    return;
  }
  const o2::mch::mapping::CathodeSegmentation& csegment = segment.bending();
  o2::mch::contour::Contour<double> envelop = o2::mch::mapping::getEnvelop(csegment);
  std::vector<o2::mch::contour::Vertex<double>> vertices = envelop.getVertices();
  o2::mch::contour::BBox<double> bbox = o2::mch::mapping::getBBox(csegment);

  double xmax = bbox.xmax();
  if (xId == 0) {
    // ST3 left, shift all detectors to the right
    xB0 += 120 - xmax;
    xNB0 += 120 - xmax;
    if (deId == 13) {
      xB0 -= 40;
      xNB0 -= 40;
    }
  }
  if (xId == 1) {
    // ST3 right, shift all detectors to the left
    xB0 -= 120 - xmax;
    xNB0 -= 120 - xmax;
    if (deId == 0) {
      xB0 += 40;
      xNB0 += 40;
    }
  }
}

void GlobalHistogram::getDeCenterST5(int de, float& xB0, float& yB0, float& xNB0, float& yNB0)
{
  float deWidth = getGlobalHistDeWidth(mId);

  getDeCenterST4(de, xB0, yB0, xNB0, yNB0);
  xB0 += getNHistPerChamberX(mId) * 2 * deWidth;
  xNB0 += getNHistPerChamberX(mId) * 2 * deWidth;
}

void GlobalHistogram::add(std::map<int, std::shared_ptr<DetectorHistogram>>& histB, std::map<int, std::shared_ptr<DetectorHistogram>>& histNB)
{
  set(histB, histNB, false);
}

void GlobalHistogram::set_includeNull(std::map<int, std::shared_ptr<DetectorHistogram>>& histB, std::map<int, std::shared_ptr<DetectorHistogram>>& histNB)
{
  set(histB, histNB, true, true);
}

void GlobalHistogram::set(std::map<int, std::shared_ptr<DetectorHistogram>>& histB, std::map<int, std::shared_ptr<DetectorHistogram>>& histNB, bool doAverage, bool includeNullBins)
{
  int deMin = (mId == 0) ? 100 : 500;
  int deMax = (mId == 0) ? 403 : 1100;

  for (auto& ih : histB) {
    int de = ih.first;
    if (de < deMin || de > deMax) {
      continue;
    }

    auto hB = ih.second.get();
    if (!hB) {
      continue;
    }
    if (!hB->getHist()) {
      continue;
    }

    DetectorHistogram* hNB = nullptr;
    auto jh = histNB.find(de);
    if (jh != histNB.end()) {
      hNB = jh->second.get();
    }
    if (!hNB) {
      continue;
    }
    if (!hNB->getHist()) {
      continue;
    }

    TH2F* hist[2] = { hB->getHist(), hNB->getHist() };

    float xB0, yB0, xNB0, yNB0;
    getDeCenter(de, xB0, yB0, xNB0, yNB0);

    float x0[2] = { xB0, xNB0 };
    float y0[2] = { yB0, yNB0 };

    const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);
    const o2::mch::mapping::CathodeSegmentation& csegmentB = segment.bending();
    o2::mch::contour::BBox<double> bboxB = o2::mch::mapping::getBBox(csegmentB);
    o2::mch::contour::Contour<double> envelopB = o2::mch::mapping::getEnvelop(csegmentB);

    const o2::mch::mapping::CathodeSegmentation& csegmentNB = segment.nonBending();
    o2::mch::contour::BBox<double> bboxNB = o2::mch::mapping::getBBox(csegmentNB);
    o2::mch::contour::Contour<double> envelopNB = o2::mch::mapping::getEnvelop(csegmentNB);

    float xMin[2] = { static_cast<float>(xB0 - bboxB.width() / 2), static_cast<float>(xNB0 - bboxNB.width() / 2) };
    float xMax[2] = { static_cast<float>(xB0 + bboxB.width() / 2), static_cast<float>(xNB0 + bboxNB.width() / 2) };
    float yMin[2] = { static_cast<float>(yB0 + bboxB.ymin()), static_cast<float>(yNB0 + bboxNB.ymin()) };
    float yMax[2] = { static_cast<float>(yB0 + bboxB.ymax()), static_cast<float>(yNB0 + bboxNB.ymax()) };

    if (mId == 0) {
      bool flipX = getDetectorFlipX(de);
      bool flipY = getDetectorFlipY(de);
      float shiftX = getDetectorShiftX(de);
      float shiftY = getDetectorShiftY(de);
      xMin[0] = flipX ? xB0 - 1.0 * (bboxB.width() + shiftX) : xB0 + shiftX;
      xMin[1] = flipX ? xNB0 - 1.0 * (bboxNB.width() + shiftX) : xNB0 + shiftX;
      xMax[0] = flipX ? xB0 - shiftX : (xB0 + bboxB.width() + shiftX);
      xMax[1] = flipX ? xNB0 - shiftX : (xNB0 + bboxNB.width() + shiftX);

      yMin[0] = flipY ? yB0 - 1.0 * (bboxB.height() + shiftY) : yB0 + shiftY;
      yMin[1] = flipY ? yNB0 - 1.0 * (bboxNB.height() + shiftY) : yNB0 + shiftY;
      yMax[0] = flipY ? yB0 - shiftY : (yB0 + bboxB.height() + shiftY);
      yMax[1] = flipY ? yNB0 - shiftY : (yNB0 + bboxNB.height() + shiftY);
    }

    float binWidthX = getHist()->GetXaxis()->GetBinWidth(1);
    float binWidthY = getHist()->GetYaxis()->GetBinWidth(1);

    // loop on bending and non-bending planes
    for (int i = 0; i < 2; i++) {

      if (hist[i] == nullptr) {
        continue;
      }

      // loop on destination bins
      int binXmin = getHist()->GetXaxis()->FindBin(xMin[i] + binWidthX / 2);
      int binXmax = getHist()->GetXaxis()->FindBin(xMax[i] - binWidthX / 2);
      int binYmin = getHist()->GetYaxis()->FindBin(yMin[i] + binWidthY / 2);
      int binYmax = getHist()->GetYaxis()->FindBin(yMax[i] - binWidthY / 2);

      for (int by = binYmin; by <= binYmax; by++) {
        // vertical boundaries of current bin, in DE coordinates
        float minY = getHist()->GetYaxis()->GetBinLowEdge(by) - y0[i];
        float maxY = getHist()->GetYaxis()->GetBinUpEdge(by) - y0[i];

        // find Y bin range in source histogram
        int srcBinYmin = hist[i]->GetYaxis()->FindBin(minY);
        if (hist[i]->GetYaxis()->GetBinCenter(srcBinYmin) < minY) {
          srcBinYmin += 1;
        }
        int srcBinYmax = hist[i]->GetYaxis()->FindBin(maxY);
        if (hist[i]->GetYaxis()->GetBinCenter(srcBinYmax) > maxY) {
          srcBinYmax -= 1;
        }

        for (int bx = binXmin; bx <= binXmax; bx++) {
          // horizontal boundaries of current bin, in DE coordinates
          float minX = getHist()->GetXaxis()->GetBinLowEdge(bx) - x0[i];
          float maxX = getHist()->GetXaxis()->GetBinUpEdge(bx) - x0[i];

          // find X bin range in source histogram
          int srcBinXmin = hist[i]->GetXaxis()->FindBin(minX);
          if (hist[i]->GetXaxis()->GetBinCenter(srcBinXmin) < minX) {
            srcBinXmin += 1;
          }
          int srcBinXmax = hist[i]->GetXaxis()->FindBin(maxX);
          if (hist[i]->GetXaxis()->GetBinCenter(srcBinXmax) > maxX) {
            srcBinXmax -= 1;
          }

          // loop on source bins, and compute the sum or average
          int nBins = 0;
          float tot = 0;
          for (int sby = srcBinYmin; sby <= srcBinYmax; sby++) {
            for (int sbx = srcBinXmin; sbx <= srcBinXmax; sbx++) {
              float val = hist[i]->GetBinContent(sbx, sby);
              if (val == 0 && !includeNullBins) {
                continue;
              }
              nBins += 1;
              tot += val;
            }
          }

          if (doAverage && (nBins > 0)) {
            tot /= nBins;
          }
          getHist()->SetBinContent(bx, by, tot);
        }
      }
    }
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
