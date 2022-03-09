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
/// \file   QcMFTDigitTask.h
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova
///

#ifndef QC_MFT_DIGIT_TASK_H
#define QC_MFT_DIGIT_TASK_H

// ROOT
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
// Quality Control
#include "QualityControl/TaskInterface.h"

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::mft
{

/// \brief MFT Digit QC task
///
class QcMFTDigitTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  QcMFTDigitTask() = default;
  /// Destructor
  ~QcMFTDigitTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  //  variables
  const double maxBinXPixelOccupancyMap = 1024;
  const double maxBinYPixelOccupancyMap = 512;
  const double minBinPixelOccupancyMap = 0;
  const double shiftPixelOccupancyMap = 0.5;
  const int binWidthPixelOccupancyMap = 1;

  const int numberOfOccupancyMaps = 20;
  const int numberOfChips = 936;

  int mCurrentFLP;
  int mTaskLevel;
  int mNumberOfPixelMapsPerFLP[5] = { 66, 66, 82, 118, 136 };

  int mVectorIndexOfChips[936] = { 0 };
  int mOccupancyMapIndexOfChips[936] = { 0 };
  int mVectorIndexOfOccupancyMaps[20] = { 0 };

  int mHalf[936] = { 0 };
  int mDisk[936] = { 0 };
  int mFace[936] = { 0 };
  int mZone[936] = { 0 };
  int mSensor[936] = { 0 };
  int mTransID[936] = { 0 };
  int mLayer[936] = { 0 };
  int mLadder[936] = { 0 };
  float mX[936] = { 0 };
  float mY[936] = { 0 };

  std::unique_ptr<TH1F> mDigitChipOccupancy = nullptr;
  std::unique_ptr<TH1F> mDigitChipStdDev = nullptr;
  std::unique_ptr<TH2F> mDigitOccupancySummary = nullptr;

  std::vector<std::unique_ptr<TH2F>> mDigitChipOccupancyMap;
  std::vector<std::unique_ptr<TH2F>> mDigitPixelOccupancyMap;

  //  functions
  int getVectorIndexChipOccupancyMap(int chipIndex);
  int getIndexChipOccupancyMap(int vectorChipOccupancyMapIndex);
  int getVectorIndexPixelOccupancyMap(int chipIndex);
  int getChipIndexPixelOccupancyMap(int vectorIndex);
  void getNameOfChipOccupancyMap(TString& folderName, TString& histogramName, int iOccupancyMapIndex);
  void getNameOfPixelOccupancyMap(TString& folderName, TString& histogramName, int iChipIndex);
  void resetArrays(int* array1, int* array2, int* array3);
  void getChipMapData();
};

} // namespace o2::quality_control_modules::mft

#endif // QC_MFT_DIGIT_TASK_H
