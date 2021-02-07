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
/// \file   BasicDigitQcTask.h
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
///

#ifndef QC_MODULE_MFT_MFTBasicDIGITQCTASK_H
#define QC_MODULE_MFT_MFTBasicDIGITQCTASK_H

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
/// \author Katarina Krizkova Gajdosova
/// \author Tomas Herman
/// \author Guillermo Contreras
class BasicDigitQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  BasicDigitQcTask() = default;
  /// Destructor
  ~BasicDigitQcTask() override;

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
  const double gPixelHitMapsMaxBinX = 1023.5;
  const double gPixelHitMapsMaxBinY = 511.5;
  const double gPixelHitMapsMinBin = -0.5;
  const int gPixelHitMapsBinWidth = 1;

  const int nHitmaps = 20;
  const int nChip = 936;

  int FLP;
  int TaskLevel;
  int nMapsH0[5] = { 66, 66, 82, 118, 136 };
  int nMapsH1[5] = { 136, 118, 82, 66, 66 };
  int nMaps[5] = { 66, 66, 82, 118, 136 };

  // int half[936] = { 0 };
  // int disk[936] = { 0 };
  // int face[936] = { 0 };
  // int zone[936] = { 0 };
  // int ladder[936] = { 0 };
  // int sensor[936] = { 0 };
  // int transID[936] = { 0 };
  // int layer[936] = { 0 };
  // double x[936] = { 0 };
  // double y[936] = { 0 };
  // double z[936] = { 0 };
  // double binx[936] = { 0 };
  // double biny[936] = { 0 };

  //  bin numbers for chip hit maps
  double binsChipHitMaps[20][6] = {
    // half0
    { 12, -10, 10, 4, -12, 0 }, // disk0, face 0
    { 12, -10, 10, 4, -12, 0 }, // disk0, face 1

    { 12, -10, 10, 4, -12, 0 },
    { 12, -10, 10, 4, -12, 0 },

    { 13, -11, 10, 4, -12, 0 },
    { 13, -10, 11, 4, -12, 0 },

    { 16, -13, 14, 5, -15, 0 },
    { 16, -14, 13, 5, -15, 0 },

    { 17, -14, 14, 5, -15, 0 },
    { 17, -14, 14, 5, -15, 0 },

    // half1
    { 12, -10, 10, 4, 0, 12 },
    { 12, -10, 10, 4, 0, 12 },

    { 12, -10, 10, 4, 0, 12 },
    { 12, -10, 10, 4, 0, 12 },

    { 13, -10, 11, 4, 0, 12 },
    { 13, -11, 10, 4, 0, 12 },

    { 16, -14, 13, 5, 0, 15 },
    { 16, -13, 14, 5, 0, 15 },

    { 17, -14, 14, 5, 0, 15 },
    { 17, -14, 14, 5, 0, 15 },
  };

  std::unique_ptr<TH1F> mMFT_chip_index_H = nullptr;
  std::unique_ptr<TH1F> mMFT_chip_std_dev_H = nullptr;

  std::vector<std::unique_ptr<TH2F>> mMFTChipHitMap;
  std::vector<std::unique_ptr<TH2F>> mMFTPixelHitMap;

  //  functions
  void readTable();
  int getVectorIndex(int chipID);
  int getChipIndex(int vectorID);
  void getChipName(TString& FolderName, TString& HistogramName, int iHitMap);
  void getPixelName(TString& FolderName, TString& HistogramName, int iVectorID);
};

} // namespace o2::quality_control_modules::mft

#endif // QC_MODULE_MFT_MFTBasicDIGITQCTASK_H
