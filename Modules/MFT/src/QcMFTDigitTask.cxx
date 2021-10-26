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
/// \file   QcMFTDigitTask.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova
///

// ROOT
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
// O2
#include <DataFormatsITSMFT/Digit.h>
#include <Framework/InputRecord.h>
#include <ITSMFTReconstruction/ChipMappingMFT.h>
// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/QcMFTDigitTask.h"
#include "MFT/QcMFTUtilTables.h"
// C++
#include <fstream>

namespace o2::quality_control_modules::mft
{

QcMFTDigitTask::~QcMFTDigitTask()
{
  /*
    not needed for unique pointers
  */
}

void QcMFTDigitTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize QcMFTDigitTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("FLP"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - FLP: " << param->second << ENDM;
    mCurrentFLP = stoi(param->second);
  }
  if (auto param = mCustomParameters.find("TaskLevel"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - TaskLevel: " << param->second << ENDM;
    mTaskLevel = stoi(param->second);
  }

  getChipMapData();

  //  reset arrays of vector and chip IDs
  resetArrays(mVectorIndexOfChips, mOccupancyMapIndexOfChips, mVectorIndexOfOccupancyMaps);

  // Defining histograms
  //==============================================
  mChipOccupancy = std::make_unique<TH1F>(
    "mMFTChipOccupancy",
    "Chip Occupancy;Chip ID;#Entries",
    936, -0.5, 935.5);
  getObjectsManager()->startPublishing(mChipOccupancy.get());

  mChipOccupancyStdDev = std::make_unique<TH1F>(
    "mMFTChipOccupancyStdDev",
    "Chip Occupancy Std Dev;Chip ID;Chip std dev",
    936, -0.5, 935.5);
  getObjectsManager()->startPublishing(mChipOccupancyStdDev.get());

  // --Chip hit maps
  //==============================================
  QcMFTUtilTables MFTTable;
  for (int iVectorOccupancyMapIndex = 0; iVectorOccupancyMapIndex < 4; iVectorOccupancyMapIndex++) {
    // create only hit maps corresponding to the FLP
    int iOccupancyMapIndex = getIndexChipOccupancyMap(iVectorOccupancyMapIndex);

    //  generate folder and histogram name using the mapping table
    TString folderName = "";
    TString histogramName = "";
    getNameOfChipOccupancyMap(folderName, histogramName, iOccupancyMapIndex);

    auto chiphitmap = std::make_unique<TH2F>(
      folderName, histogramName,
      MFTTable.mNumberOfBinsInOccupancyMaps[iOccupancyMapIndex][0],
      MFTTable.mNumberOfBinsInOccupancyMaps[iOccupancyMapIndex][1],
      MFTTable.mNumberOfBinsInOccupancyMaps[iOccupancyMapIndex][2],
      MFTTable.mNumberOfBinsInOccupancyMaps[iOccupancyMapIndex][3],
      MFTTable.mNumberOfBinsInOccupancyMaps[iOccupancyMapIndex][4],
      MFTTable.mNumberOfBinsInOccupancyMaps[iOccupancyMapIndex][5]);
    chiphitmap->SetStats(0);
    chiphitmap->SetOption("colz");
    mChipOccupancyMap.push_back(std::move(chiphitmap));
    getObjectsManager()->startPublishing(mChipOccupancyMap[iVectorOccupancyMapIndex].get());
  }

  // --Pixel hit maps
  //==============================================
  int maxVectorIndex = mNumberOfPixelMapsPerFLP[mCurrentFLP] + mNumberOfPixelMapsPerFLP[4 - mCurrentFLP];
  for (int iVectorIndex = 0; iVectorIndex < maxVectorIndex; iVectorIndex++) {
    // create only hit maps corresponding to the FLP
    int iChipIndex = getChipIndexPixelOccupancyMap(iVectorIndex);

    //  generate folder and histogram name using the mapping table
    TString folderName = "";
    TString histogramName = "";
    getNameOfPixelOccupancyMap(folderName, histogramName, iChipIndex);

    auto pixelhitmap = std::make_unique<TH2F>(
      folderName, histogramName,
      maxBinXPixelOccupancyMap / binWidthPixelOccupancyMap,
      minBinPixelOccupancyMap - shiftPixelOccupancyMap,
      maxBinXPixelOccupancyMap - shiftPixelOccupancyMap,
      maxBinYPixelOccupancyMap / binWidthPixelOccupancyMap,
      minBinPixelOccupancyMap - shiftPixelOccupancyMap,
      maxBinYPixelOccupancyMap - shiftPixelOccupancyMap);
    pixelhitmap->SetStats(0);
    pixelhitmap->SetOption("colz");
    mPixelOccupancyMap.push_back(std::move(pixelhitmap));
    if (mTaskLevel == 1)
      getObjectsManager()->startPublishing(mPixelOccupancyMap[iVectorIndex].get());
  }
}

void QcMFTDigitTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;

  mChipOccupancy->Reset();
  mChipOccupancyStdDev->Reset();

  for (int iVectorOccupancyMapIndex = 0; iVectorOccupancyMapIndex < 4; iVectorOccupancyMapIndex++) {
    mChipOccupancyMap[iVectorOccupancyMapIndex]->Reset();
  }

  int maxVectorIndex = mNumberOfPixelMapsPerFLP[mCurrentFLP] + mNumberOfPixelMapsPerFLP[4 - mCurrentFLP];
  for (int iVectorIndex = 0; iVectorIndex < maxVectorIndex; iVectorIndex++) {
    mPixelOccupancyMap[iVectorIndex]->Reset();
  }
}

void QcMFTDigitTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void QcMFTDigitTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the digits
  const auto digits = ctx.inputs().get<gsl::span<o2::itsmft::Digit>>("randomdigit");
  if (digits.size() < 1)
    return;

  // fill the pixel hit maps and overview histograms
  for (auto& oneDigit : digits) {

    int chipIndex = oneDigit.getChipIndex();

    int vectorIndex = getVectorIndexPixelOccupancyMap(chipIndex);
    if (vectorIndex < 0) // if the chip is not from wanted FLP, the array will give -1
      continue;

    // fill pixel hit maps
    mPixelOccupancyMap[vectorIndex]->Fill(oneDigit.getColumn(), oneDigit.getRow());

    // fill overview histograms
    mChipOccupancy->SetBinContent(chipIndex + 1, mPixelOccupancyMap[vectorIndex]->GetEntries());
    mChipOccupancyStdDev->SetBinContent(chipIndex + 1, mPixelOccupancyMap[vectorIndex]->GetStdDev(1));

    // fill integrated chip hit maps
    int vectorOccupancyMapIndex = getVectorIndexChipOccupancyMap(chipIndex);
    if (vectorOccupancyMapIndex < 0)
      continue;
    mChipOccupancyMap[vectorOccupancyMapIndex]->Fill(mX[chipIndex], mY[chipIndex]);
  }
}

void QcMFTDigitTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void QcMFTDigitTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void QcMFTDigitTask::reset()
{
  // clean all the monitor objects here
  ILOG(Info, Support) << "Resetting the histogram" << ENDM;

  mChipOccupancy->Reset();
  mChipOccupancyStdDev->Reset();

  for (int iVectorOccupancyMapIndex = 0; iVectorOccupancyMapIndex < 4; iVectorOccupancyMapIndex++) {
    mChipOccupancyMap[iVectorOccupancyMapIndex]->Reset();
  }

  int maxVectorIndex = mNumberOfPixelMapsPerFLP[mCurrentFLP] + mNumberOfPixelMapsPerFLP[4 - mCurrentFLP];
  for (int iVectorIndex = 0; iVectorIndex < maxVectorIndex; iVectorIndex++) {
    mPixelOccupancyMap[iVectorIndex]->Reset();
  }
}

void QcMFTDigitTask::getNameOfChipOccupancyMap(TString& folderName, TString& histogramName, int iOccupancyMapIndex)
{
  folderName = Form("ChipOccupancyMaps/Half_%d/Disk_%d/Face_%d/mMFTChipOccupancyMap",
                    int(iOccupancyMapIndex / 10), int((iOccupancyMapIndex % 10) / 2), (iOccupancyMapIndex % 10) % 2);

  histogramName = Form("h%d-d%d-f%d;x (cm);y (cm)",
                       int(iOccupancyMapIndex / 10), int((iOccupancyMapIndex % 10) / 2), (iOccupancyMapIndex % 10) % 2);
}

void QcMFTDigitTask::getNameOfPixelOccupancyMap(TString& folderName, TString& histogramName, int iChipIndex)
{
  folderName = Form("PixelOccupancyMaps/Half_%d/Disk_%d/Face_%d/mMFTPixelOccupancyMap-z%d-l%d-s%d-tr%d",
                    mHalf[iChipIndex], mDisk[iChipIndex], mFace[iChipIndex], mZone[iChipIndex],
                    mLadder[iChipIndex], mSensor[iChipIndex], mTransID[iChipIndex]);

  histogramName = Form("h%d-d%d-f%d-z%d-l%d-s%d-tr%d",
                       mHalf[iChipIndex], mDisk[iChipIndex], mFace[iChipIndex], mZone[iChipIndex],
                       mLadder[iChipIndex], mSensor[iChipIndex], mTransID[iChipIndex]);
}

void QcMFTDigitTask::getChipMapData()
{
  const o2::itsmft::ChipMappingMFT map;
  auto chipMapData = map.getChipMappingData();
  QcMFTUtilTables MFTTable;

  for (int i = 0; i < 936; i++) {
    mHalf[i] = chipMapData[i].half;
    mDisk[i] = chipMapData[i].disk;
    mLayer[i] = chipMapData[i].layer;
    mFace[i] = mLayer[i] % 2;
    mZone[i] = chipMapData[i].zone;
    mSensor[i] = chipMapData[i].localChipSWID;
    mTransID[i] = chipMapData[i].cable;
    mLadder[i] = MFTTable.mLadder[i];
    mX[i] = MFTTable.mX[i];
    mY[i] = MFTTable.mY[i];
  }
}

int QcMFTDigitTask::getVectorIndexChipOccupancyMap(int chipIndex)
{

  int occupancyMapIndex = mOccupancyMapIndexOfChips[chipIndex];

  int vectorOccupancyMapIndex = mVectorIndexOfOccupancyMaps[occupancyMapIndex];

  return vectorOccupancyMapIndex;
}

int QcMFTDigitTask::getIndexChipOccupancyMap(int vectorChipOccupancyMapIndex)
{
  int vectorOccupancyMapHalf = int(vectorChipOccupancyMapIndex / 2);

  int occupancyMapIndex;
  if (vectorOccupancyMapHalf == 0)
    occupancyMapIndex = vectorChipOccupancyMapIndex + mCurrentFLP * 2;
  else
    occupancyMapIndex = (vectorChipOccupancyMapIndex % 2) + (4 - mCurrentFLP) * 2 + numberOfOccupancyMaps / 2;

  //  fill the array of vector ID for corresponding hit map
  //  (opposite matching)
  mVectorIndexOfOccupancyMaps[occupancyMapIndex] = vectorChipOccupancyMapIndex;

  return occupancyMapIndex;
}

int QcMFTDigitTask::getVectorIndexPixelOccupancyMap(int chipIndex)
{

  int vectorIndex = mVectorIndexOfChips[chipIndex];

  return vectorIndex;
}

int QcMFTDigitTask::getChipIndexPixelOccupancyMap(int vectorIndex)
{
  int vectorHalf = 0;
  if (int(vectorIndex / mNumberOfPixelMapsPerFLP[mCurrentFLP]) < 1)
    vectorHalf = 0;
  else
    vectorHalf = 1;

  int chipIndex = vectorIndex + vectorHalf * (-mNumberOfPixelMapsPerFLP[mCurrentFLP] + numberOfChips / 2);

  int maxDisk = 0;
  if (vectorHalf == 0)
    maxDisk = mCurrentFLP;
  else
    maxDisk = 4 - mCurrentFLP;

  for (int idisk = 0; idisk < maxDisk; idisk++)
    chipIndex = chipIndex + mNumberOfPixelMapsPerFLP[idisk];

  //  fill the array of vector index for corresponding chipIndex
  //  (opposite matching)
  mVectorIndexOfChips[chipIndex] = vectorIndex;
  //  fill the array of hit map ID for corresponding chipIndex
  mOccupancyMapIndexOfChips[chipIndex] = mLayer[chipIndex] + mHalf[chipIndex] * numberOfOccupancyMaps / 2;

  return chipIndex;
}

void QcMFTDigitTask::resetArrays(int array[], int array2[], int array3[])
{

  for (int iChip = 0; iChip < numberOfChips; iChip++) {
    array[iChip] = -1;
    array2[iChip] = -1;
  }

  for (int iMap = 0; iMap < numberOfOccupancyMaps; iMap++)
    array3[iMap] = -1;
}

} // namespace o2::quality_control_modules::mft
