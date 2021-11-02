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

//
// \file   LtrCalibReductor.cxx
// \author Cindy Mordasini
// \author Marcel Lesch
//
#include "TPC/LtrCalibReductor.h"
#include "QualityControl/QcInfoLogger.h"
#include <boost/algorithm/string.hpp>
#include <string>
#include <sstream>
#include <TCanvas.h>
#include <TPaveText.h>

namespace o2::quality_control_modules::tpc
{

void* LtrCalibReductor::getBranchAddress()
{
  return &mLtrCalib;
}

const char* LtrCalibReductor::getBranchLeafList()
{
  //return "processedTFs/g:dvCorrectionA/F:dvCorrectionC:dvOffsetA:dvOffsetC:nTracksA/s:nTracksC";
  return "processedTFs/D:dvCorrectionA:dvCorrectionC:dvCorrection:dvOffsetA:dvOffsetC:nTracksA:nTracksC";
}

void LtrCalibReductor::update(TObject* obj)
{
  // The 'Calib_Values' for the Laser Calibration are saved in a TPaveText inside a TCanvas 'obj'.
  if (obj) {
    //ILOG(Info, Support) << "'obj' has been passed to the reductor." << ENDM;
    if (auto canvas = static_cast<TCanvas*>(obj)) {
      if (auto blocText = static_cast<TPaveText*>(canvas->GetPrimitive("TPave"))) {

        mLtrCalib.processedTFs = getValue((TText*)blocText->GetLineWith("processedTFs:"));
        mLtrCalib.dvCorrectionA = getValue((TText*)blocText->GetLineWith("dvCorrectionA:"));
        mLtrCalib.dvCorrectionC = getValue((TText*)blocText->GetLineWith("dvCorrectionC:"));
        mLtrCalib.dvCorrection = getValue((TText*)blocText->GetLineWith("dvCorrection:"));
        mLtrCalib.dvOffsetA = getValue((TText*)blocText->GetLineWith("dvOffsetA:"));
        mLtrCalib.dvOffsetC = getValue((TText*)blocText->GetLineWith("dvOffsetC:"));
        mLtrCalib.nTracksA = getValue((TText*)blocText->GetLineWith("nTracksA:"));
        mLtrCalib.nTracksC = getValue((TText*)blocText->GetLineWith("nTracksC:"));
      }
    }
  } else {
    ILOG(Error, Support) << "No 'obj' found." << ENDM;
  }
}

double LtrCalibReductor::getValue(TText* line)
{
  std::string text = static_cast<std::string>(line->GetTitle());

  std::string title;
  double value;
  std::stringstream sStream(text);
  sStream >> title >> value;
  return value;
}

}   // namespace o2::quality_control_modules::tpc