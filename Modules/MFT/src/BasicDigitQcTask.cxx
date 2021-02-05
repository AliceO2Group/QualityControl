// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   BasicDigitQcTask.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
///

// ROOT
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
// O2
#include <DataFormatsITSMFT/Digit.h>
#include <Framework/InputRecord.h>
// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/BasicDigitQcTask.h"
// C++
#include <fstream>

namespace o2::quality_control_modules::mft
{

BasicDigitQcTask::~BasicDigitQcTask()
{
  /*
    not needed for unique pointers
  */
}

void BasicDigitQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize BasicDigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("FLP"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Custom parameter - FLP: " << param->second << ENDM;
    FLP = stoi(param->second);
  }

  // Read table with variables for hit maps - temporary solution
  readTable();
  //  -------------------
  mMFT_chip_index_H = std::make_unique<TH1F>("ChipHitMaps/mMFT_chip_index_H", "mMFT_chip_index_H;Chip ID;#Entries", 936, -0.5, 935.5);
  getObjectsManager()->startPublishing(mMFT_chip_index_H.get());

  mMFT_chip_std_dev_H = std::make_unique<TH1F>("ChipHitMaps/mMFT_chip_std_dev_H", "mMFT_chip_std_dev_H;Chip ID;#Entries", 936, -0.5, 935.5);
  getObjectsManager()->startPublishing(mMFT_chip_std_dev_H.get());

  //==============================================
  //  chip hit maps

  for (int iVectorHitMap = 0; iVectorHitMap < 4; iVectorHitMap++) {
    //  generate folder and histogram name using the mapping table
    int iHitMap = (iVectorHitMap % 2) + int(iVectorHitMap / 2) * 10 + FLP * 2;

    TString FolderName = "";
    TString HistogramName = "";
    getChipName(FolderName, HistogramName, iHitMap);

    auto chiphitmap = std::make_unique<TH2F>(
      FolderName, HistogramName,
      binsChipHitMaps[iHitMap][0], binsChipHitMaps[iHitMap][1], binsChipHitMaps[iHitMap][2],
      binsChipHitMaps[iHitMap][3], binsChipHitMaps[iHitMap][4], binsChipHitMaps[iHitMap][5]);
    chiphitmap->SetStats(0);
    mMFTChipHitMap.push_back(std::move(chiphitmap));
    getObjectsManager()->startPublishing(mMFTChipHitMap[iVectorHitMap].get());
  }

  //==============================================
  //  pixel hit maps

  for (int iVectorID = 0; iVectorID < nMaps[FLP]; iVectorID++) {
    //  generate folder and histogram name using the mapping table
    TString FolderName = "";
    TString HistogramName = "";
    getPixelName(FolderName, HistogramName, iVectorID);

    //  create pixel hit map
    auto pxlhitmap = std::make_unique<TH2F>(
      FolderName, HistogramName,
      gPixelHitMapsMaxBinX / gPixelHitMapsBinWidth, gPixelHitMapsMinBin, gPixelHitMapsMaxBinX,
      gPixelHitMapsMaxBinY / gPixelHitMapsBinWidth, gPixelHitMapsMinBin, gPixelHitMapsMaxBinY);
    pxlhitmap->SetStats(0);
    mMFTPixelHitMap.push_back(std::move(pxlhitmap));
    // getObjectsManager()->startPublishing(mMFTPixelHitMap[iVectorID].get());
  }
}

void BasicDigitQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;

  mMFT_chip_index_H->Reset();
  mMFT_chip_std_dev_H->Reset();

  for (int iVectorHitMap = 0; iVectorHitMap < 4; iVectorHitMap++) {
    mMFTChipHitMap[iVectorHitMap]->Reset();
  }

  for (int iVectorID = 0; iVectorID < nMaps[FLP]; iVectorID++) {
    mMFTPixelHitMap[iVectorID]->Reset();
  }
}

void BasicDigitQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void BasicDigitQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the digits
  const auto digits = ctx.inputs().get<gsl::span<o2::itsmft::Digit>>("randomdigit");
  if (digits.size() < 1)
    return;

  // fill the histograms
  for (auto& one_digit : digits) {
    int chipIndex = one_digit.getChipIndex();

    // Simulate QC task on specific FLP (i.e. only digits from disk X will arrive); this can be removed once the code is on FLP
    if (disk[chipIndex] != FLP)
      continue;

    int vectorIndex = getVectorIndex(chipIndex);
    //  fill pixel hit maps
    mMFTPixelHitMap[vectorIndex]->Fill(one_digit.getColumn(), one_digit.getRow());
    // fill number of entries and standard dev for all chips
    mMFT_chip_index_H->SetBinContent(chipIndex, mMFTPixelHitMap[vectorIndex]->GetEntries());
    mMFT_chip_std_dev_H->SetBinContent(chipIndex, mMFTPixelHitMap[vectorIndex]->GetStdDev(1));
  }

  //  fill the chip hit maps
  for (int iVectorID = 0; iVectorID < nMaps[FLP]; iVectorID++) {
    int nEntries = mMFTPixelHitMap[iVectorID]->GetEntries();
    int chipID = getChipIndex(iVectorID);

    int HitMap = layer[chipID] + half[chipID] * 10;
    int VectorHitMap = (HitMap % 10) - FLP * 2 + half[chipID] * 2;

    mMFTChipHitMap[VectorHitMap]->SetBinContent(binx[chipID], biny[chipID], nEntries);
  }
}

void BasicDigitQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void BasicDigitQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void BasicDigitQcTask::reset()
{
  // clean all the monitor objects here
  ILOG(Info, Support) << "Resetting the histogram" << ENDM;

  mMFT_chip_index_H->Reset();
  mMFT_chip_std_dev_H->Reset();

  for (int iVectorHitMap = 0; iVectorHitMap < 4; iVectorHitMap++) {
    mMFTChipHitMap[iVectorHitMap]->Reset();
  }

  for (int iVectorID = 0; iVectorID < nMaps[FLP]; iVectorID++) {
    mMFTPixelHitMap[iVectorID]->Reset();
  }
}

void BasicDigitQcTask::getChipName(TString& FolderName, TString& HistogramName, int iHitMap)
{
  FolderName = Form("ChipHitMaps/Half_%d/Disk_%d/Face_%d/mMFTChipHitMap",
                    int(iHitMap / 10), int((iHitMap % 10) / 2), (iHitMap % 10) % 2);

  HistogramName = Form("h%d-d%d-f%d;x (cm);y (cm)",
                       int(iHitMap / 10), int((iHitMap % 10) / 2), (iHitMap % 10) % 2);
}

void BasicDigitQcTask::getPixelName(TString& FolderName, TString& HistogramName, int iVectorID)
{
  int iChipID = getChipIndex(iVectorID);

  FolderName = Form("PixelHitMaps/Half_%d/Disk_%d/Face_%d/mMFTPixelHitMap-z%d-l%d-s%d-tr%d",
                    half[iChipID], disk[iChipID], face[iChipID], zone[iChipID], ladder[iChipID], sensor[iChipID], transID[iChipID]);

  HistogramName = Form("h%d-d%d-f%d-z%d-l%d-s%d-tr%d",
                       half[iChipID], disk[iChipID], face[iChipID], zone[iChipID], ladder[iChipID], sensor[iChipID], transID[iChipID]);
}

void BasicDigitQcTask::readTable()
{
  //  reset arrays
  for (int i = 0; i < nChip; i++) {
    half[i] = 0;
    disk[i] = 0;
    face[i] = 0;
    zone[i] = 0;
    ladder[i] = 0;
    sensor[i] = 0;
    transID[i] = 0;
    layer[i] = 0;
    x[i] = 0;
    y[i] = 0;
    z[i] = 0;
    binx[i] = 0;
    biny[i] = 0;
  }

  // read file
  std::ifstream read_table;
  read_table.open("./table_file_binidx.txt");
  for (int i = 0; i < nChip; ++i) {
    read_table >> half[i];
    read_table >> disk[i];
    read_table >> face[i];
    read_table >> zone[i];
    read_table >> ladder[i];
    read_table >> sensor[i];
    read_table >> transID[i];
    read_table >> layer[i];
    read_table >> x[i];
    read_table >> y[i];
    read_table >> z[i];
    read_table >> binx[i];
    read_table >> biny[i];
  }
  read_table.close();
}

int BasicDigitQcTask::getVectorIndex(int chipID)
{
  int vectorID = chipID + half[chipID] * (-nChip / 2 + nMaps[disk[chipID]] / 2);

  for (int idisk = 0; idisk < disk[chipID]; idisk++)
    vectorID = vectorID - nMaps[idisk] / 2;

  return vectorID;
}

int BasicDigitQcTask::getChipIndex(int vectorID)
{
  int half = int(vectorID / (nMaps[FLP] / 2));
  int chipID = vectorID + half * (-nMaps[FLP] / 2 + nChip / 2);

  for (int idisk = 0; idisk < FLP; idisk++)
    chipID = chipID + nMaps[idisk] / 2;

  return chipID;
}

} // namespace o2::quality_control_modules::mft