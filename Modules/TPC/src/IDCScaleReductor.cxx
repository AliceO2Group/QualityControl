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
// file  IDCScaleReductor.cxx
// author Marcel Lesch
//

#include "TPC/IDCScaleReductor.h"
#include "QualityControl/QcInfoLogger.h"
#include <TCanvas.h>
#include <TGraph.h>

namespace o2::quality_control_modules::tpc
{

void* IDCScaleReductor::getBranchAddress()
{
  return &mIDC;
} // void* IDCScaleReductor::getBranchAddress()

const char* IDCScaleReductor::getBranchLeafList()
{
  return "IDCScaleFactorASide/F:IDCScaleFactorCSide";
} // const char* IDCScaleReductor::getBranchLeafList()

void IDCScaleReductor::update(TObject* obj)
{
  if (obj) {
    auto canvas = dynamic_cast<TCanvas*>(obj);

    if (canvas) {
      TGraph* g = (TGraph*)canvas->GetListOfPrimitives()->FindObject("g_IDC0ScaleFactor");

      if (g) {
        mIDC.ScaleFactorASide = g->GetPointY(0);
        mIDC.ScaleFactorCSide = g->GetPointY(1);

      } // if (g)
    }   // if (canvas)
  }     // if(obj)
} // void IDCScaleReductor::update(TObject* obj)

} // namespace o2::quality_control_modules::tpc
