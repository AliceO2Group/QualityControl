// Copyright CERN and copyright holders of ALICE O2. This software is
// // distributed under the terms of the GNU General Public License v3 (GPL
// // Version 3), copied verbatim in the file "COPYING".
// //
// // See http://alice-o2.web.cern.ch/license for full licensing information.
// //
// // In applying this license CERN does not waive the privileges and immunities
// // granted to it by virtue of its status as an Intergovernmental Organization
// // or submit itself to any jurisdiction.
//

///
/// ITSNoisyPixelTask.h
///
///

#ifndef QC_MODULE_ITS_ITSNOISYPIXELTASK_H
#define QC_MODULE_ITS_ITSNOISYPIXELTASK_H

#include "QualityControl/TaskInterface.h"
#include <TH1.h>
#include <TH2.h>
#include <THnSparse.h>
#include <DataFormatsITSMFT/TopologyDictionary.h>
#include <ITSBase/GeometryTGeo.h>

#include <algorithm>
#include <set>
#include <map>
#include <string>
#include <unordered_set>
#include <unordered_map>

class TH1D;
class TH2D;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::its
{

class ITSNoisyPixelTask : public TaskInterface
{

 public:
  ITSNoisyPixelTask();
  ~ITSNoisyPixelTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  void publishHistos();
  void formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset = 1., float yOffset = 1.);
  void addObject(TObject* aObject);
  void getJsonParameters();
  void createAllHistos();
  void NormalizeOccupancyPlots(int n_cycle);
  std::vector<int> MapOverHIC(int col, int row, int chip);

  static constexpr int NLayer = 7;
  static constexpr int NLayerIB = 3;

  int mROFcounter = 0;
  int mROFcycle = 0;
  std::unordered_map<uint64_t, int> hashtable;

  std::vector<TObject*> mPublishedObjects;

  int nmostnoisy = 25; // number of bins for the following three histograms. It can be configured from config file.
  TH1D* hOrderedHitsAddressIB;
  TH1D* hOrderedHitsAddressML;
  TH1D* hOrderedHitsAddressOL;

  TH2D* hOccupancyIB[3];
  TH2D* hOccupancyOB[4];

  THnSparseD* hNoisyPixelMapIB[3][20];
  THnSparseD* hNoisyPixelMapOB[4][48];

  enum QueryType { kUndefined, kCluster, kDigit};
  QueryType mQueryOption = kUndefined;

  int mOccUpdateFrequency;
  bool mEnableOrderedHitsObject;
  int mTotalTimeInQCTask;
  int ChipIDprev = 0;
  std::string mDictPath;
  std::string mGeomPath;

  const int mNStaves[7] = { 12, 16, 20, 24, 30, 42, 48 };
  const int mNHicPerStave[7] = { 1, 1, 1, 8, 8, 14, 14 };
  const int mNChipsPerHic[7] = { 9, 9, 9, 14, 14, 14, 14 };
  int mEnableLayers[7];
  o2::itsmft::TopologyDictionary mDict;
  o2::its::GeometryTGeo* mGeom;
};
} //  namespace o2::quality_control_modules::its

#endif
