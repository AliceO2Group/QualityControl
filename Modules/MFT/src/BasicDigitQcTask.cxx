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
    ILOG(Info, Support) << "Custom parameter - myOwnKey: " << param->second << ENDM;
    FLP = stoi(param->second);
    minChipID = (FLP - 1) * nchip / 4;
    maxChipID = FLP * nchip / 4;
  }

  //  -------------------
  mMFT_chip_index_H = std::make_unique<TH1F>("mMFT_chip_index_H", "mMFT_chip_index_H", 936, -0.5, 935.5);
  getObjectsManager()->startPublishing(mMFT_chip_index_H.get());
  getObjectsManager()->addMetadata(mMFT_chip_index_H->GetName(), "custom", "34");

  mMFT_chip_std_dev_H = std::make_unique<TH1F>("mMFT_chip_std_dev_H", "mMFT_chip_std_dev_H", 936, -0.5, 935.5);
  getObjectsManager()->startPublishing(mMFT_chip_std_dev_H.get());
  getObjectsManager()->addMetadata(mMFT_chip_std_dev_H->GetName(), "custom", "34");

  //==============================================
  //  chip hit maps
  readTable();
  for (int iHitMap = 0; iHitMap < nhitmaps; iHitMap++) {
    //  generate folder and histogram name using the mapping table
    TString FolderName = "";
    TString HistogramName = "";
    getChipName(FolderName, HistogramName, iHitMap);

    auto chiphitmap = std::make_unique<TH2F>(
      FolderName, HistogramName,
      binsChipHitMaps[iHitMap][0], binsChipHitMaps[iHitMap][1], binsChipHitMaps[iHitMap][2],
      binsChipHitMaps[iHitMap][3], binsChipHitMaps[iHitMap][4], binsChipHitMaps[iHitMap][5]);
    chiphitmap->SetStats(0);
    mMFTChipHitMap.push_back(std::move(chiphitmap));
    getObjectsManager()->startPublishing(mMFTChipHitMap[iHitMap].get());
    getObjectsManager()->addMetadata(mMFTChipHitMap[iHitMap]->GetName(), "custom", "34");
  }

  //==============================================
  //  pixel hit maps
  for (int iChipID = 0; iChipID < nchip; iChipID++) {
    //  generate folder and histogram name using the mapping table
    TString FolderName = "";
    TString HistogramName = "";
    getPixelName(FolderName, HistogramName, iChipID);

    //  create pixel hit map
    auto pxlhitmap = std::make_unique<TH2F>(
      FolderName, HistogramName,
      gPixelHitMapsMaxBinX / gPixelHitMapsBinWidth, gPixelHitMapsMinBin, gPixelHitMapsMaxBinX,
      gPixelHitMapsMaxBinY / gPixelHitMapsBinWidth, gPixelHitMapsMinBin, gPixelHitMapsMaxBinY);
    pxlhitmap->SetStats(0);
    mMFTPixelHitMap.push_back(std::move(pxlhitmap));

    if ((iChipID >= minChipID) && (iChipID < maxChipID)) {
      getObjectsManager()->startPublishing(mMFTPixelHitMap[iChipID].get());
      getObjectsManager()->addMetadata(mMFTPixelHitMap[iChipID]->GetName(), "custom", "34");
    }
  }
}

void BasicDigitQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;

  mMFT_chip_index_H->Reset();
  mMFT_chip_std_dev_H->Reset();

  for (int iHitMap = 0; iHitMap < nhitmaps; iHitMap++) {
    mMFTChipHitMap[iHitMap]->Reset();
  }

  for (int iChipID = 0; iChipID < nchip; iChipID++) {
    mMFTPixelHitMap[iChipID]->Reset();
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

    //  fill pixel hit maps
    mMFTPixelHitMap[chipIndex]->Fill(one_digit.getColumn(), one_digit.getRow());
    // fill number of entries and standard dev for all chips
    mMFT_chip_index_H->SetBinContent(chipIndex, mMFTPixelHitMap[chipIndex]->GetEntries());
    mMFT_chip_std_dev_H->SetBinContent(chipIndex, mMFTPixelHitMap[chipIndex]->GetStdDev(1));
  }

  //  fill the chip hit maps
  for (int iChipID = 0; iChipID < nchip; iChipID++) {
    int nEntries = mMFTPixelHitMap[iChipID]->GetEntries();
    mMFTChipHitMap[layer[iChipID] + half[iChipID] * 10]->SetBinContent(binx[iChipID], biny[iChipID], nEntries);
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

  for (int iHitMap = 0; iHitMap < nhitmaps; iHitMap++) {
    mMFTChipHitMap[iHitMap]->Reset();
  }

  for (int iChipID = 0; iChipID < nchip; iChipID++) {
    mMFTPixelHitMap[iChipID]->Reset();
  }
}

void BasicDigitQcTask::getChipName(TString& FolderName, TString& HistogramName, int iHitMap)
{
  FolderName = Form("ChipHitMaps/Half_%d/Disk_%d/Face_%d/mMFTChipHitMap",
                    int(iHitMap / 10), int((iHitMap % 10) / 2), (iHitMap % 10) % 2);

  HistogramName = Form("h%d-d%d-f%d;x (cm);y (cm)",
                       int(iHitMap / 10), int((iHitMap % 10) / 2), (iHitMap % 10) % 2);
}

void BasicDigitQcTask::getPixelName(TString& FolderName, TString& HistogramName, int iChipID)
{
  FolderName = Form("PixelHitMaps/Half_%d/Disk_%d/Face_%d/mMFTPixelHitMap-z%d-l%d-s%d-tr%d",
                    half[iChipID], disk[iChipID], face[iChipID], zone[iChipID], ladder[iChipID], sensor[iChipID], transID[iChipID]);

  HistogramName = Form("h%d-d%d-f%d-z%d-l%d-s%d-tr%d",
                       half[iChipID], disk[iChipID], face[iChipID], zone[iChipID], ladder[iChipID], sensor[iChipID], transID[iChipID]);
}

void BasicDigitQcTask::readTable()
{
  //const int nchip = 936;

  //  reset arrays
  for (int i = 0; i < nchip; i++) {
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
  for (int i = 0; i < nchip; ++i) {
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

} // namespace o2::quality_control_modules::mft
