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
/// \author David Grund
///

// C++
#include <gsl/span>
#include <string>
#include <vector>
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TAxis.h>
#include <TString.h>
// O2
#include <DataFormatsITSMFT/Digit.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <Framework/InputRecord.h>
#include <Framework/TimingInfo.h>
#include <Framework/ProcessingContext.h>
#include <Framework/ServiceRegistryRef.h>
#include <Framework/ProcessingContext.h>
#include <ITSMFTReconstruction/ChipMappingMFT.h>
#include <CommonConstants/LHCConstants.h>
// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/QcMFTDigitTask.h"
#include "MFT/QcMFTUtilTables.h"
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"
#include "DetectorsBase/GRPGeomHelper.h"

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
  ILOG(Debug, Devel) << "initialize QcMFTDigitTask" << ENDM;

  // loading custom parameters
  if (auto param = mCustomParameters.find("FLP"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - FLP: " << param->second << ENDM;
    mCurrentFLP = stoi(param->second);
  }

  if (auto param = mCustomParameters.find("NoiseScan"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - NoiseScan: " << param->second << ENDM;
    mNoiseScan = stoi(param->second);
  }

  auto maxDigitROFSize = 5000;
  if (auto param = mCustomParameters.find("maxDigitROFSize"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - maxDigitROFSize: " << param->second << ENDM;
    maxDigitROFSize = stoi(param->second);
  }

  auto maxDuration = 60.f;
  if (auto param = mCustomParameters.find("maxDuration"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - maxDuration: " << param->second << ENDM;
    maxDuration = stof(param->second);
  }

  auto timeBinSize = 0.01f;
  if (auto param = mCustomParameters.find("timeBinSize"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - timeBinSize: " << param->second << ENDM;
    timeBinSize = stof(param->second);
  }

  auto NofTimeBins = static_cast<int>(maxDuration / timeBinSize);

  auto ROFLengthInBC = 198;
  if (auto param = mCustomParameters.find("ROFLengthInBC"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - ROFLengthInBC: " << param->second << ENDM;
    ROFLengthInBC = stoi(param->second);
  }
  auto ROFsPerOrbit = o2::constants::lhc::LHCMaxBunches / ROFLengthInBC;

  getChipMapData();

  //  reset arrays of vector and chip IDs
  resetArrays(mVectorIndexOfChips, mOccupancyMapIndexOfChips, mVectorIndexOfOccupancyMaps);

  // Defining histograms

  mMergerTest = std::make_unique<TH1F>(
    "mMergerTest", "Merger testing from different FLPs;FLP ID;# entries", 5, -0.5, 4.5);
  mMergerTest->SetStats(0);
  mMergerTest->GetXaxis()->SetBinLabel(1, "FLP 182");
  mMergerTest->GetXaxis()->SetBinLabel(2, "FLP 183");
  mMergerTest->GetXaxis()->SetBinLabel(3, "FLP 184");
  mMergerTest->GetXaxis()->SetBinLabel(4, "FLP 185");
  mMergerTest->GetXaxis()->SetBinLabel(5, "FLP 186");
  getObjectsManager()->startPublishing(mMergerTest.get());

  mDigitChipOccupancy = std::make_unique<TH1FRatio>(
    "mDigitChipOccupancy", "Digit Chip Occupancy;Chip ID;# entries per orbit", 936, -0.5, 935.5, true);
  mDigitChipOccupancy->SetStats(0);
  getObjectsManager()->startPublishing(mDigitChipOccupancy.get());
  getObjectsManager()->setDisplayHint(mDigitChipOccupancy.get(), "hist");

  mDigitDoubleColumnSensorIndices = std::make_unique<TH2FRatio>(
    "mDigitDoubleColumnSensorIndices", "Double Column vs Chip ID;Double Column;Chip ID",
    512, -0.5, 511.5, 936, -0.5, 935.5, true);
  mDigitDoubleColumnSensorIndices->SetStats(0);
  getObjectsManager()->startPublishing(mDigitDoubleColumnSensorIndices.get());
  getObjectsManager()->setDisplayHint(mDigitDoubleColumnSensorIndices.get(), "colz");

  if (mNoiseScan == 1) { // to be executed only for special runs
    mDigitChipStdDev = std::make_unique<TH1F>(
      "mDigitChipStdDev", "Digit Chip Std Dev;Chip ID;Chip std dev", 936, -0.5, 935.5);
    mDigitChipStdDev->SetStats(0);
    getObjectsManager()->startPublishing(mDigitChipStdDev.get());
  }

  mDigitOccupancySummary = std::make_unique<TH2FRatio>(
    "mDigitOccupancySummary", "Digit Occupancy Summary;;", 10, -0.5, 9.5, 8, -0.5, 7.5, true);
  mDigitOccupancySummary->GetXaxis()->SetBinLabel(1, "d0-f0");
  mDigitOccupancySummary->GetXaxis()->SetBinLabel(2, "d0-f1");
  mDigitOccupancySummary->GetXaxis()->SetBinLabel(3, "d1-f0");
  mDigitOccupancySummary->GetXaxis()->SetBinLabel(4, "d1-f1");
  mDigitOccupancySummary->GetXaxis()->SetBinLabel(5, "d2-f0");
  mDigitOccupancySummary->GetXaxis()->SetBinLabel(6, "d2-f1");
  mDigitOccupancySummary->GetXaxis()->SetBinLabel(7, "d3-f0");
  mDigitOccupancySummary->GetXaxis()->SetBinLabel(8, "d3-f1");
  mDigitOccupancySummary->GetXaxis()->SetBinLabel(9, "d4-f0");
  mDigitOccupancySummary->GetXaxis()->SetBinLabel(10, "d4-f1");
  mDigitOccupancySummary->GetYaxis()->SetBinLabel(1, "h0-z0");
  mDigitOccupancySummary->GetYaxis()->SetBinLabel(2, "h0-z1");
  mDigitOccupancySummary->GetYaxis()->SetBinLabel(3, "h0-z2");
  mDigitOccupancySummary->GetYaxis()->SetBinLabel(4, "h0-z3");
  mDigitOccupancySummary->GetYaxis()->SetBinLabel(5, "h1-z0");
  mDigitOccupancySummary->GetYaxis()->SetBinLabel(6, "h1-z1");
  mDigitOccupancySummary->GetYaxis()->SetBinLabel(7, "h1-z2");
  mDigitOccupancySummary->GetYaxis()->SetBinLabel(8, "h1-z3");
  mDigitOccupancySummary->SetStats(0);
  getObjectsManager()->startPublishing(mDigitOccupancySummary.get());
  getObjectsManager()->setDisplayHint(mDigitOccupancySummary.get(), "colz");

  mDigitsROFSize = std::make_unique<TH1FRatio>("mDigitsROFSize",
                                               "Distribution of the #digits per ROF; # digits per ROF; # entries per orbit",
                                               QcMFTUtilTables::nROFBins, const_cast<float*>(QcMFTUtilTables::mROFBins), true);
  mDigitsROFSize->SetStats(0);
  getObjectsManager()->startPublishing(mDigitsROFSize.get());
  getObjectsManager()->setDisplayHint(mDigitsROFSize.get(), "hist logx logy");

  mDigitsBC = std::make_unique<TH1FRatio>("mDigitsBC",
                                          "Digits per BC; BCid; # entries per orbit",
                                          o2::constants::lhc::LHCMaxBunches, 0, o2::constants::lhc::LHCMaxBunches, true);
  mDigitsBC->SetMinimum(0.1);
  getObjectsManager()->startPublishing(mDigitsBC.get());
  getObjectsManager()->setDisplayHint(mDigitsBC.get(), "hist");

  // Chip hit maps

  QcMFTUtilTables MFTTable;
  for (int iVectorOccupancyMapIndex = 0; iVectorOccupancyMapIndex < 4; iVectorOccupancyMapIndex++) {
    // create only hit maps corresponding to the FLP
    int iOccupancyMapIndex = getIndexChipOccupancyMap(iVectorOccupancyMapIndex);

    //  generate folder and histogram name using the mapping table
    TString folderName = "";
    TString histogramName = "";
    getNameOfChipOccupancyMap(folderName, histogramName, iOccupancyMapIndex);

    auto chiphitmap = std::make_unique<TH2FRatio>(
      folderName, histogramName,
      MFTTable.mNumberOfBinsInOccupancyMaps[iOccupancyMapIndex][0],
      MFTTable.mNumberOfBinsInOccupancyMaps[iOccupancyMapIndex][1],
      MFTTable.mNumberOfBinsInOccupancyMaps[iOccupancyMapIndex][2],
      MFTTable.mNumberOfBinsInOccupancyMaps[iOccupancyMapIndex][3],
      MFTTable.mNumberOfBinsInOccupancyMaps[iOccupancyMapIndex][4],
      MFTTable.mNumberOfBinsInOccupancyMaps[iOccupancyMapIndex][5], true);
    chiphitmap->SetStats(0);
    mDigitChipOccupancyMap.push_back(std::move(chiphitmap));
    getObjectsManager()->startPublishing(mDigitChipOccupancyMap[iVectorOccupancyMapIndex].get());
    getObjectsManager()->setDefaultDrawOptions(mDigitChipOccupancyMap[iVectorOccupancyMapIndex].get(), "colz");
  }

  // Pixel hit maps
  int maxVectorIndex = mNumberOfPixelMapsPerFLP[mCurrentFLP] + mNumberOfPixelMapsPerFLP[4 - mCurrentFLP];
  for (int iVectorIndex = 0; iVectorIndex < maxVectorIndex; iVectorIndex++) {
    // create only hit maps corresponding to the FLP
    int iChipIndex = getChipIndexPixelOccupancyMap(iVectorIndex);
  }

  if (mNoiseScan == 1) { // to be executed only for special runs
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
      mDigitPixelOccupancyMap.push_back(std::move(pixelhitmap));
      getObjectsManager()->startPublishing(mDigitPixelOccupancyMap[iVectorIndex].get());
      getObjectsManager()->setDefaultDrawOptions(mDigitPixelOccupancyMap[iVectorIndex].get(), "colz");
    }
  }
}

void QcMFTDigitTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;

  // reset histograms
  reset();
}

void QcMFTDigitTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void QcMFTDigitTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto mNOrbitsPerTF = o2::base::GRPGeomHelper::instance().getNHBFPerTF();

  mMergerTest->Fill(mCurrentFLP);
  mMergerTest->Fill(-1); // To test what happenes with the normalisation when merged.
  // get the digits
  const auto digits = ctx.inputs().get<gsl::span<o2::itsmft::Digit>>("randomdigit");
  if (digits.empty()) {
    return;
  }

  // get the rofs
  const auto rofs = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("digitsrof");
  if (rofs.empty()) {
    return;
  }

  // get correct timing info of the first TF orbit
  if (mRefOrbit == -1) {
    mRefOrbit = ctx.services().get<o2::framework::TimingInfo>().firstTForbit;
  }

  // fill the digits time histograms
  for (const auto& rof : rofs) {
    mDigitsROFSize->getNum()->Fill(rof.getNEntries());
    float seconds = orbitToSeconds(rof.getBCData().orbit, mRefOrbit) + rof.getBCData().bc * o2::constants::lhc::LHCBunchSpacingNS * 1e-9;
    mDigitsBC->getNum()->Fill(rof.getBCData().bc, rof.getNEntries());
  }

  // fill the pixel hit maps and overview histograms
  for (auto& oneDigit : digits) {

    int chipIndex = oneDigit.getChipIndex();

    int vectorIndex = getVectorIndexPixelOccupancyMap(chipIndex);
    if (vectorIndex < 0) // if the chip is not from wanted FLP, the array will give -1
      continue;

    // fill double column histogram
    mDigitDoubleColumnSensorIndices->getNum()->Fill(oneDigit.getColumn() >> 1, oneDigit.getChipIndex());

    // fill info into the summary histo
    int xBin = mDisk[chipIndex] * 2 + mFace[chipIndex];
    int yBin = mZone[chipIndex] + mHalf[chipIndex] * 4;
    mDigitOccupancySummary->getNum()->Fill(xBin, yBin);

    // fill pixel hit maps
    if (mNoiseScan == 1)
      mDigitPixelOccupancyMap[vectorIndex]->Fill(oneDigit.getColumn(), oneDigit.getRow());

    // fill overview histograms
    mDigitChipOccupancy->getNum()->Fill(chipIndex);
    if (mNoiseScan == 1)
      mDigitChipStdDev->SetBinContent(chipIndex + 1, mDigitPixelOccupancyMap[vectorIndex]->GetStdDev(1));

    // fill integrated chip hit maps
    int vectorOccupancyMapIndex = getVectorIndexChipOccupancyMap(chipIndex);
    if (vectorOccupancyMapIndex < 0)
      continue;
    mDigitChipOccupancyMap[vectorOccupancyMapIndex]->getNum()->Fill(mX[chipIndex], mY[chipIndex]);
  }

  // fill the denominators
  mDigitChipOccupancy->getDen()->SetBinContent(1, mDigitChipOccupancy->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mDigitOccupancySummary->getDen()->SetBinContent(1, 1, mDigitOccupancySummary->getDen()->GetBinContent(1, 1) + mNOrbitsPerTF);
  mDigitDoubleColumnSensorIndices->getDen()->SetBinContent(1, 1, mDigitDoubleColumnSensorIndices->getDen()->GetBinContent(1, 1) + mNOrbitsPerTF);
  mDigitsROFSize->getDen()->SetBinContent(1, mDigitsROFSize->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  mDigitsBC->getDen()->SetBinContent(1, mDigitsBC->getDen()->GetBinContent(1) + mNOrbitsPerTF);
  for (int i = 0; i < 4; i++)
    mDigitChipOccupancyMap[i]->getDen()->SetBinContent(1, 1, mDigitChipOccupancyMap[i]->getDen()->GetBinContent(1, 1) + mNOrbitsPerTF);
}

void QcMFTDigitTask::endOfCycle()
{
  // update all THRatios
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;

  mDigitChipOccupancy->update();
  mDigitOccupancySummary->update();
  mDigitDoubleColumnSensorIndices->update();
  mDigitsROFSize->update();
  mDigitsBC->update();
  for (int i = 0; i < 4; i++)
    mDigitChipOccupancyMap[i]->update();
}

void QcMFTDigitTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void QcMFTDigitTask::reset()
{
  // clean all the monitor objects here
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;

  mMergerTest->Reset();
  mDigitChipOccupancy->Reset();
  mDigitDoubleColumnSensorIndices->Reset();
  if (mNoiseScan == 1)
    mDigitChipStdDev->Reset();
  mDigitOccupancySummary->Reset();
  mDigitsROFSize->Reset();
  mDigitsBC->Reset();
  for (int i = 0; i < 4; i++)
    mDigitChipOccupancyMap[i]->Reset();
  if (mNoiseScan == 1) {
    int maxVectorIndex = mNumberOfPixelMapsPerFLP[mCurrentFLP] + mNumberOfPixelMapsPerFLP[4 - mCurrentFLP];
    for (int j = 0; j < maxVectorIndex; j++)
      mDigitPixelOccupancyMap[j]->Reset();
  }
}

void QcMFTDigitTask::getNameOfChipOccupancyMap(TString& folderName, TString& histogramName, int iOccupancyMapIndex)
{
  folderName = Form("ChipOccupancyMaps/Half_%d/Disk_%d/Face_%d/mDigitChipOccupancyMap",
                    int(iOccupancyMapIndex / 10), int((iOccupancyMapIndex % 10) / 2), (iOccupancyMapIndex % 10) % 2);

  histogramName = Form("Digit Chip Map h%d-d%d-f%d;x (cm);y (cm)",
                       int(iOccupancyMapIndex / 10), int((iOccupancyMapIndex % 10) / 2), (iOccupancyMapIndex % 10) % 2);
}

void QcMFTDigitTask::getNameOfPixelOccupancyMap(TString& folderName, TString& histogramName, int iChipIndex)
{
  folderName = Form("PixelOccupancyMaps/Half_%d/Disk_%d/Face_%d/mDigitPixelOccupancyMap-z%d-l%d-s%d-tr%d",
                    mHalf[iChipIndex], mDisk[iChipIndex], mFace[iChipIndex], mZone[iChipIndex],
                    mLadder[iChipIndex], mSensor[iChipIndex], mTransID[iChipIndex]);

  histogramName = Form("Pixel Map h%d-d%d-f%d-z%d-l%d-s%d-tr%d",
                       mHalf[iChipIndex], mDisk[iChipIndex], mFace[iChipIndex], mZone[iChipIndex],
                       mLadder[iChipIndex], mSensor[iChipIndex], mTransID[iChipIndex]);
}

void QcMFTDigitTask::getChipMapData()
{
  const o2::itsmft::ChipMappingMFT mapMFT;
  auto chipMapData = mapMFT.getChipMappingData();
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
  if (vectorOccupancyMapHalf == 0) {
    occupancyMapIndex = vectorChipOccupancyMapIndex + mCurrentFLP * 2;
  } else {
    occupancyMapIndex = (vectorChipOccupancyMapIndex % 2) + (4 - mCurrentFLP) * 2 + numberOfOccupancyMaps / 2;
  }
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
  if (int(vectorIndex / mNumberOfPixelMapsPerFLP[mCurrentFLP]) < 1) {
    vectorHalf = 0;
  } else {
    vectorHalf = 1;
  }

  int chipIndex = vectorIndex + vectorHalf * (-mNumberOfPixelMapsPerFLP[mCurrentFLP] + numberOfChips / 2);

  int maxDisk = 0;
  if (vectorHalf == 0) {
    maxDisk = mCurrentFLP;
  } else {
    maxDisk = 4 - mCurrentFLP;
  }

  for (int idisk = 0; idisk < maxDisk; idisk++)
    chipIndex = chipIndex + mNumberOfPixelMapsPerFLP[idisk];

  //  fill the array of vector index for corresponding chipIndex
  //  (opposite matching)
  mVectorIndexOfChips[chipIndex] = vectorIndex;
  //  fill the array of hit map ID for corresponding chipIndex
  mOccupancyMapIndexOfChips[chipIndex] = mLayer[chipIndex] + mHalf[chipIndex] * numberOfOccupancyMaps / 2;

  return chipIndex;
}

void QcMFTDigitTask::resetArrays(int* array1, int* array2, int* array3)
{

  for (int iChip = 0; iChip < numberOfChips; iChip++) {
    array1[iChip] = -1;
    array2[iChip] = -1;
  }

  for (int iMap = 0; iMap < numberOfOccupancyMaps; iMap++)
    array3[iMap] = -1;
}

} // namespace o2::quality_control_modules::mft
