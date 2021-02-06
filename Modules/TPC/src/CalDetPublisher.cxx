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
/// \file   CalDetPublisher.cxx
/// \author Thomas Klemenz
///

// O2 includes
#include "TPC/CalDetPublisher.h"
#include "TPCBase/Painter.h"
#include "TPCBase/CDBInterface.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/Utility.h"

//root includes
#include "TCanvas.h"

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::tpc
{

void CalDetPublisher::initialize(Trigger, framework::ServiceRegistry&)
{
  addAndPublish(getObjectsManager(), mPedestalCanvasVec, { "c_Sides_Pedestals", "c_ROCs_Pedestals_1D", "c_ROCs_Pedestals_2D" });
  addAndPublish(getObjectsManager(), mNoiseCanvasVec, { "c_Sides_Noise", "c_ROCs_Noise_1D", "c_ROCs_Noise_2D" });
}

void CalDetPublisher::update(Trigger t, framework::ServiceRegistry&)
{
  ILOG(Info, Support) << "Trigger type is: " << t.triggerType << ", the timestamp is " << t.timestamp << ENDM;

  //========Pedestals==========
  auto& pedestal = o2::tpc::CDBInterface::instance().getPedestals();
  auto vecPtrPed = toVector(mPedestalCanvasVec);
  o2::tpc::painter::makeSummaryCanvases(pedestal, 300, 0, 0, true, &vecPtrPed);

  //==========Noise===========
  auto& noise = o2::tpc::CDBInterface::instance().getNoise();
  auto vecPtrNoise = toVector(mNoiseCanvasVec);
  o2::tpc::painter::makeSummaryCanvases(noise, 300, 0, 0, true, &vecPtrNoise);
}

void CalDetPublisher::finalize(Trigger, framework::ServiceRegistry&)
{
  for (auto& canvas : mPedestalCanvasVec) {
    getObjectsManager()->stopPublishing(canvas.get());
  }

  for (auto& canvas : mNoiseCanvasVec) {
    getObjectsManager()->stopPublishing(canvas.get());
  }
}

} // namespace o2::quality_control_modules::tpc
