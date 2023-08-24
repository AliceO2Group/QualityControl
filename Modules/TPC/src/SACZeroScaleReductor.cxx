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
// file  SACZeroScaleReductor.cxx
// author Marcel Lesch
//

#include "TPC/SACZeroScaleReductor.h"
#include "QualityControl/QcInfoLogger.h"
#include <TCanvas.h>
#include <TGraph.h>

namespace o2::quality_control_modules::tpc
{

void* SACZeroScaleReductor::getBranchAddress()
{
  return &mSACZero;
} // void* SACZeroScaleReductor::getBranchAddress()

const char* SACZeroScaleReductor::getBranchLeafList()
{
  return "SACZeroScaleFactorASide/F:SACZeroScaleFactorCSide";
} // const char* SACZeroScaleReductor::getBranchLeafList()

void SACZeroScaleReductor::update(TObject* obj)
{
  if (obj) {
    auto canvas = dynamic_cast<TCanvas*>(obj);

    if (canvas) {
      auto g = dynamic_cast<TGraph*>(canvas->GetListOfPrimitives()->FindObject("g_SAC0ScaleFactor"));

      if (g) {
        mSACZero.ScaleFactorASide = g->GetPointY(0);
        mSACZero.ScaleFactorCSide = g->GetPointY(1);

      } // if (g)
    }   // if (canvas)
  }     // if(obj)
} // void SACZeroScaleReductor::update(TObject* obj)

} // namespace o2::quality_control_modules::tpc
