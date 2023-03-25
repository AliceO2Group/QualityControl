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
/// \file   PhysicsTask.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_GLOBALHISTOGRAM_H
#define QC_MODULE_MUONCHAMBERS_GLOBALHISTOGRAM_H

#include <map>
#include <TH2.h>

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

class DetectorHistogram
{
 public:
  DetectorHistogram(TString name, TString title, int deId, int cathode);
  DetectorHistogram(TString name, TString title, int deId, int cathode, TH2F* hist);
  ~DetectorHistogram();

  void Fill(double padX, double padY, double padSizeX, double padSizeY, double val = 1);
  void Set(double padX, double padY, double padSizeX, double padSizeY, double val);

  int getNbinsX();
  int getNbinsY();
  float getXmin();
  float getXmax();
  float getYmin();
  float getYmax();

  TH2F* getHist() { return mHist.first; }

 private:
  void init();
  void addContour();

  int mDeId{ 0 };
  int mCathode{ 0 };

  TString mName;
  TString mTitle;
  std::pair<TH2F*, bool> mHist;

  bool mFlipX{ false };
  bool mFlipY{ false };
  float mShiftX{ 0 };
  float mShiftY{ 0 };

  float mHistWidth{ 0 };
  float mHistHeight{ 0 };
};

class GlobalHistogram
{
 public:
  GlobalHistogram(std::string name, std::string title, int id, float rescale);
  GlobalHistogram(std::string name, std::string title, int id, float rescale, TH2F* hist);
  ~GlobalHistogram();

  void init();

  // add the histograms of the individual detection elements
  void add(std::map<int, std::shared_ptr<DetectorHistogram>>& histB, std::map<int, std::shared_ptr<DetectorHistogram>>& histNB);

  // replace the contents with the histograms of the individual detection elements, including null bins
  void set_includeNull(std::map<int, std::shared_ptr<DetectorHistogram>>& histB, std::map<int, std::shared_ptr<DetectorHistogram>>& histNB);

  // replace the contents with the histograms of the individual detection elements
  void set(std::map<int, std::shared_ptr<DetectorHistogram>>& histB, std::map<int, std::shared_ptr<DetectorHistogram>>& histNB, bool doAverage = true, bool includeNullBins = false);

  TH2F* getHist() { return mHist.first; }

 private:
  void initST345();
  void initST12();
  void getDeCenter(int de, float& xB0, float& yB0, float& xNB0, float& yNB0);
  void getDeCenterST12(int de, float& xB0, float& yB0, float& xNB0, float& yNB0);
  void getDeCenterST3(int de, float& xB0, float& yB0, float& xNB0, float& yNB0);
  void getDeCenterST4(int de, float& xB0, float& yB0, float& xNB0, float& yNB0);
  void getDeCenterST5(int de, float& xB0, float& yB0, float& xNB0, float& yNB0);

  TString mName;
  TString mTitle;
  int mId;
  float mScaleFactor;
  std::pair<TH2F*, bool> mHist;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_GLOBALHISTOGRAM_H
