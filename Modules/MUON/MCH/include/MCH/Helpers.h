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
/// \file   Helpers.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_HELPERS_H
#define QC_MODULE_MUONCHAMBERS_HELPERS_H

#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/Quality.h"
#include "MCHConstants/DetectionElements.h"
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TLegend.h>
#include <TText.h>
#include <gsl/span>
#include <utility>
#include <optional>

namespace o2::quality_control::core
{
class Activity;
}

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

int getDEindex(int de);
constexpr int getNumDE() { return (4 * 4 + 18 * 2 + 26 * 4); }
int getNumDEinChamber(int chIndex);
std::pair<int, int> getDEindexInChamber(int deId);

o2::quality_control::core::Quality checkDetectorQuality(gsl::span<o2::quality_control::core::Quality>& deQuality);

void addChamberDelimiters(TH1F* h, float xmin = 0, float xmax = 0);
void addChamberDelimiters(TH2F* h);

std::string getHistoPath(int deId);
bool matchHistName(std::string hist, std::string name);

void splitDataSourceName(std::string s, std::string& type, std::string& name);

// check the overall spectrometer quality based on the individual detectors
struct QualityChecker {
  QualityChecker();

  void reset();
  void addCheckResult(gsl::span<o2::quality_control::core::Quality> quality);
  o2::quality_control::core::Quality checkST12(int i);
  o2::quality_control::core::Quality checkST345(int i);
  o2::quality_control::core::Quality getQuality();

  int mMaxBadST12{ 2 };
  int mMaxBadST345{ 5 };

  std::array<int, getNumDE()> mChamberMap;
  std::array<std::pair<int, int>, getNumDE()> mDeMap;
  std::array<o2::quality_control::core::Quality, getNumDE()> mQuality;
};

// build a unique FEC ID from a FEE/LINK/DSADDR triple
struct FecId {
  FecId(int feeId, int linkId, int dsAddr)
  {
    mFecId = feeId * sLinkNum * sDsNum + linkId * sDsNum + dsAddr;
  }

  FecId(int fecId) : mFecId(fecId) {}

  int getFeeId() { return (mFecId / (sLinkNum * sDsNum)); }
  int getLinkId() { return ((mFecId % (sLinkNum * sDsNum)) / sDsNum); }
  int getDsAddr() { return (mFecId % sDsNum); }

  static constexpr int max() { return ((sFeeNum * sLinkNum * sDsNum) - 1); }

  static constexpr int sFeeNum = 64;
  static constexpr int sLinkNum = 12;
  static constexpr int sDsNum = 40;

  int mFecId{ 0 };
};

//_________________________________________________________________________________________

struct CcdbObjectHelper {
  CcdbObjectHelper();
  CcdbObjectHelper(std::string p, std::string n);

  bool update(o2::quality_control::repository::DatabaseInterface* qcdb,
              long timeStamp = -1,
              const o2::quality_control::core::Activity& activity = {});
  void setStartIme();
  long getTimeStamp() { return mTimeStamp; }
  template <typename T>
  T* get()
  {
    if (!mObject) {
      return nullptr;
    }
    if (!mObject->getObject()) {
      return nullptr;
    }

    // Get histogram object
    T* h = dynamic_cast<T*>(mObject->getObject());
    return h;
  }

  std::shared_ptr<o2::quality_control::core::MonitorObject> mObject;
  std::string mPath;
  std::string mName;
  uint64_t mTimeStart{ 0 };
  uint64_t mTimeStamp{ 0 };
};

//_________________________________________________________________________________________

struct QualityObjectHelper {
  QualityObjectHelper();
  QualityObjectHelper(std::string p, std::string n);

  bool update(o2::quality_control::repository::DatabaseInterface* qcdb,
              long timeStamp = -1,
              const o2::quality_control::core::Activity& activity = {});
  void setStartIme();
  long getTimeStamp() { return mTimeStamp; }

  std::shared_ptr<o2::quality_control::core::QualityObject> mObject;
  bool mUpdated{ false };
  std::string mPath;
  std::string mName;
  uint64_t mTimeStart{ 0 };
  uint64_t mTimeStamp{ 0 };
};

//_________________________________________________________________________________________

class TrendGraph : public TCanvas
{
 public:
  TrendGraph(std::string name, std::string title, std::string label, std::optional<float> refValue = {});

  void update(uint64_t time, float val);

 private:
  std::optional<float> mRefValue;
  std::string mAxisLabel;
  std::unique_ptr<TGraph> mGraph;
  std::unique_ptr<TGraph> mGraphRef;
  std::unique_ptr<TGraph> mGraphHist;
  std::array<std::unique_ptr<TLegend>, 2> mLegends;
};

//_________________________________________________________________________________________

class QualityTrendGraph : public TCanvas
{
 public:
  QualityTrendGraph(std::string name, std::string title);

  void update(uint64_t time, o2::quality_control::core::Quality q);

 private:
  std::unique_ptr<TGraph> mGraph;
  std::unique_ptr<TGraph> mGraphHist;
  std::array<std::unique_ptr<TText>, 4> mLabels;
};

//_________________________________________________________________________________________

class TrendMultiGraph : public TCanvas
{
 public:
  TrendMultiGraph(std::string name, std::string title, std::string label);

  void addGraph(std::string name, std::string title, std::optional<float> refValue = {});
  void addLegends();
  void update(long time, gsl::span<double> values);

  void setRange(float min, float max)
  {
    mYmin = min;
    mYmax = max;
  }

 private:
  std::string mAxisLabel;

  float mYmin{ 0 };
  float mYmax{ 0 };

  int mNGraphs{ 0 };
  std::unique_ptr<TGraph> mGraphHist;
  std::array<std::optional<float>, 10> mRefValues;
  std::array<std::unique_ptr<TGraph>, 10> mGraphs;
  std::array<std::unique_ptr<TGraph>, 10> mGraphsRef;
  std::array<std::unique_ptr<TLegend>, 5> mLegends;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_HELPERS_H
