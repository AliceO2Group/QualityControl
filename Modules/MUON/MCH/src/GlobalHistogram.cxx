///
/// \file   PhysicsTask.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#include <iostream>
#include <TLine.h>
#include <TList.h>

#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingInterface/CathodeSegmentation.h"
#ifdef MCH_HAS_MAPPING_FACTORY
#include "MCHMappingFactory/CreateSegmentation.h"
#endif
#include "MCHMappingSegContour/CathodeSegmentationContours.h"

#include "MCH/GlobalHistogram.h"

#define DE_HEIGHT 60
#define DE_WIDTH 250
#define NXHIST_PER_CHAMBER 2
#define NYHIST_PER_CHAMBER 13
#define NCHAMBERS_PER_STATION 2
#define NXHIST_PER_STATION NXHIST_PER_CHAMBER* NCHAMBERS_PER_STATION
#define NSTATIONS 3

#define HIST_WIDTH (NSTATIONS * NXHIST_PER_CHAMBER * NCHAMBERS_PER_STATION * DE_WIDTH)
#define HIST_HEIGHT (NYHIST_PER_CHAMBER * DE_HEIGHT * 2)
#define HIST_SCALE 2

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

GlobalHistogram::GlobalHistogram(std::string name, std::string title) : TH2F(name.c_str(), title.c_str(), HIST_WIDTH / HIST_SCALE, 0, HIST_WIDTH, HIST_HEIGHT / HIST_SCALE, 0, HIST_HEIGHT)
{
}

void GlobalHistogram::init()
{
  TLine* line;

  line = new TLine(0, HIST_HEIGHT / 2, HIST_WIDTH, HIST_HEIGHT / 2);
  GetListOfFunctions()->Add(line);

  line = new TLine(NXHIST_PER_STATION * DE_WIDTH, 0, NXHIST_PER_STATION * DE_WIDTH, HIST_HEIGHT);
  GetListOfFunctions()->Add(line);

  line = new TLine(NXHIST_PER_STATION * DE_WIDTH * 2, 0, NXHIST_PER_STATION * DE_WIDTH * 2, HIST_HEIGHT);
  GetListOfFunctions()->Add(line);

  for (int de = 500; de < 1100; de++) {
    const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);
    if ((&segment) == nullptr) {
      continue;
    }
    const o2::mch::mapping::CathodeSegmentation& csegment = segment.bending();
    o2::mch::contour::Contour<double> envelop = o2::mch::mapping::getEnvelop(csegment);
    std::vector<o2::mch::contour::Vertex<double>> vertices = envelop.getVertices();

    float xB0, yB0, xNB0, yNB0;
    getDeCenter(de, xB0, yB0, xNB0, yNB0);
    bool isR = getLR(de) == 1;

    for (unsigned int vi = 0; vi < vertices.size(); vi++) {
      const o2::mch::contour::Vertex<double> v1 = vertices[vi];
      const o2::mch::contour::Vertex<double> v2 = (vi < (vertices.size() - 1)) ? vertices[vi + 1] : vertices[0];

      if (isR) {
        line = new TLine(-v1.x + xB0, v1.y + yB0, -v2.x + xB0, v2.y + yB0);
        GetListOfFunctions()->Add(line);
        line = new TLine(-v1.x + xNB0, v1.y + yNB0, -v2.x + xNB0, v2.y + yNB0);
        GetListOfFunctions()->Add(line);
      } else {
        line = new TLine(v1.x + xB0, v1.y + yB0, v2.x + xB0, v2.y + yB0);
        GetListOfFunctions()->Add(line);
        line = new TLine(v1.x + xNB0, v1.y + yNB0, v2.x + xNB0, v2.y + yNB0);
        GetListOfFunctions()->Add(line);
      }
    }
  }
}

void GlobalHistogram::getDeCenter(int de, float& xB0, float& yB0, float& xNB0, float& yNB0)
{
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

int GlobalHistogram::getLR(int de)
{
  int lr = -1;
  int deId = de % 100;
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

void GlobalHistogram::getDeCenterST3(int de, float& xB0, float& yB0, float& xNB0, float& yNB0)
{
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

  xB0 = xNB0 = DE_WIDTH * xId + DE_WIDTH / 2;
  //xNB0 = DE_WIDTH * xId + DE_WIDTH * 2 + DE_WIDTH / 2;

  yB0 = yNB0 = DE_HEIGHT * yId + DE_HEIGHT / 2;
  yB0 += NYHIST_PER_CHAMBER * DE_HEIGHT;

  int chamber = de / 100;
  if ((chamber % 2) == 0) {
    //yB0  += NYHIST_PER_CHAMBER * DE_HEIGHT;
    //yNB0 += NYHIST_PER_CHAMBER * DE_HEIGHT;
    xB0 += NXHIST_PER_CHAMBER * DE_WIDTH;
    xNB0 += NXHIST_PER_CHAMBER * DE_WIDTH;
  }

  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);
  if ((&segment) == nullptr) {
    return;
  }
  const o2::mch::mapping::CathodeSegmentation& csegment = segment.bending();
  o2::mch::contour::Contour<double> envelop = o2::mch::mapping::getEnvelop(csegment);
  std::vector<o2::mch::contour::Vertex<double>> vertices = envelop.getVertices();
  o2::mch::contour::BBox<double> bbox = o2::mch::mapping::getBBox(csegment);
  //std::cout<<"DE "<<de<<"  BBOX "<<bbox<<std::endl;

  double xmin = bbox.xmin();
  double xmax = bbox.xmax();
  if (xId == 0) {
    // ST3 left, shift all detectors to the right
    xB0 += 120 - xmax;
    xNB0 += 120 - xmax;
  }
  if (xId == 1) {
    // ST3 right, shift all detectors to the left
    xB0 -= 120 - xmax;
    xNB0 -= 120 - xmax;
  }
}

void GlobalHistogram::getDeCenterST4(int de, float& xB0, float& yB0, float& xNB0, float& yNB0)
{
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

  xB0 = xNB0 = DE_WIDTH * xId + DE_WIDTH / 2 + NXHIST_PER_STATION * DE_WIDTH;

  //xB0  = DE_WIDTH * xId + DE_WIDTH / 2 + NXHIST_PER_STATION * DE_WIDTH;
  //xNB0 = DE_WIDTH * xId + DE_WIDTH * 2 + DE_WIDTH / 2 + NXHIST_PER_STATION * DE_WIDTH;
  //xB0  = DE_WIDTH * (2 * xId)     + DE_WIDTH / 2 + NXHIST_PER_CHAMBER * DE_WIDTH;
  //xNB0 = DE_WIDTH * (2 * xId + 1) + DE_WIDTH / 2 + NXHIST_PER_CHAMBER * DE_WIDTH;

  yB0 = yNB0 = DE_HEIGHT * yId + DE_HEIGHT / 2;
  yB0 += NYHIST_PER_CHAMBER * DE_HEIGHT;

  int chamber = de / 100;
  if ((chamber % 2) == 0) {
    //yB0  += NYHIST_PER_CHAMBER * DE_HEIGHT;
    //yNB0 += NYHIST_PER_CHAMBER * DE_HEIGHT;
    xB0 += NXHIST_PER_CHAMBER * DE_WIDTH;
    xNB0 += NXHIST_PER_CHAMBER * DE_WIDTH;
  }

  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);
  if ((&segment) == nullptr) {
    return;
  }
  const o2::mch::mapping::CathodeSegmentation& csegment = segment.bending();
  o2::mch::contour::Contour<double> envelop = o2::mch::mapping::getEnvelop(csegment);
  std::vector<o2::mch::contour::Vertex<double>> vertices = envelop.getVertices();
  o2::mch::contour::BBox<double> bbox = o2::mch::mapping::getBBox(csegment);
  //std::cout<<"DE "<<de<<"  BBOX "<<bbox<<std::endl;

  double xmin = bbox.xmin();
  double xmax = bbox.xmax();
  if (xId == 0) {
    // ST3 left, shift all detectors to the right
    xB0 += 120 - xmax;
    xNB0 += 120 - xmax;
  }
  if (xId == 1) {
    // ST3 right, shift all detectors to the left
    xB0 -= 120 - xmax;
    xNB0 -= 120 - xmax;
  }
}

void GlobalHistogram::getDeCenterST5(int de, float& xB0, float& yB0, float& xNB0, float& yNB0)
{
  getDeCenterST4(de, xB0, yB0, xNB0, yNB0);
  xB0 += NXHIST_PER_STATION * DE_WIDTH;
  xNB0 += NXHIST_PER_STATION * DE_WIDTH;
}

void GlobalHistogram::add(std::map<int, TH2F*>& histB, std::map<int, TH2F*>& histNB)
{
  set(histB, histNB, false);
}

void GlobalHistogram::set(std::map<int, TH2F*>& histB, std::map<int, TH2F*>& histNB, bool doAverage)
{
  for (auto& ih : histB) {
    int de = ih.first;
    //if (de != 819) continue;
    if (de < 500)
      continue;
    if (de >= 1100)
      continue;
    auto hB = ih.second;
    if (!hB) {
      continue;
    }

    bool swapX = getLR(de) == 1;

    TH2F* hNB = nullptr;
    auto jh = histNB.find(de);
    if (jh != histNB.end()) {
      hNB = jh->second;
    }

    TH2F* hist[2] = { hB, hNB };

    float xB0, yB0, xNB0, yNB0;
    getDeCenter(de, xB0, yB0, xNB0, yNB0);

    float x0[2] = { xB0, xNB0 };
    float y0[2] = { yB0, yNB0 };

    float xMin[2] = { xB0 - DE_WIDTH / 2, xNB0 - DE_WIDTH / 2 };
    float xMax[2] = { xB0 + DE_WIDTH / 2, xNB0 + DE_WIDTH / 2 };
    float yMin[2] = { yB0 - DE_HEIGHT / 2, yNB0 - DE_HEIGHT / 2 };
    float yMax[2] = { yB0 + DE_HEIGHT / 2, yNB0 + DE_HEIGHT / 2 };

    //std::cout<<"DE "<<de<<"B   LIMITS "<<xMin[0]<<" "<<xMax[0]<<", "<<yMin[0]<<" "<<yMax[0]<<std::endl;
    //std::cout<<"DE "<<de<<"NB  LIMITS "<<xMin[1]<<" "<<xMax[1]<<", "<<yMin[1]<<" "<<yMax[1]<<std::endl;

    float binWidthX = GetXaxis()->GetBinWidth(1);
    float binWidthY = GetYaxis()->GetBinWidth(1);

    // loop on bending and non-bending planes
    for (int i = 0; i < 2; i++) {

      if (hist[i] == nullptr) {
        continue;
      }

      // loop on destination bins
      int binXmin = GetXaxis()->FindBin(xMin[i] + binWidthX / 2);
      int binXmax = GetXaxis()->FindBin(xMax[i] - binWidthX / 2);
      int binYmin = GetYaxis()->FindBin(yMin[i] + binWidthY / 2);
      int binYmax = GetYaxis()->FindBin(yMax[i] - binWidthY / 2);

      //std::cout<<(i==0 ? "B " : "NB")<<"  BIN LIMITS "<<binXmin<<" "<<binXmax<<", "<<binYmin<<" "<<binYmax<<std::endl;

      for (int by = binYmin; by <= binYmax; by++) {
        // vertical boundaries of current bin, in DE coordinates
        float minY = GetYaxis()->GetBinLowEdge(by) - y0[i];
        float maxY = GetYaxis()->GetBinUpEdge(by) - y0[i];

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
          float minX = GetXaxis()->GetBinLowEdge(bx) - x0[i];
          float maxX = GetXaxis()->GetBinUpEdge(bx) - x0[i];

          if (swapX) {
            float tempMax = -minX;
            float tempMin = -maxX;
            minX = tempMin;
            maxX = tempMax;
          }

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
              if (val == 0) {
                continue;
              }
              nBins += 1;
              tot += val;
            }
          }

          if (doAverage && (nBins > 0)) {
            tot /= nBins;
          }
          //if(de==800) std::cout<<"DE "<<de<<"  i "<<i<<"  by "<<by<<"  bx "<<bx<<" --> "<<tot<<std::endl;
          SetBinContent(bx, by, tot);
        }
      }
    }
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
