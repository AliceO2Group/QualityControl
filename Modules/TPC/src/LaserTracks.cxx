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
/// \file   LaserTracks.cxx
/// \author Thomas Klemenz
///

// O2 includes
#if __has_include("TPCBase/Painter.h")
#include "TPCBase/Painter.h"
#include "TPCBase/CDBInterface.h"
#else
#include "TPCBaseRecSim/Painter.h"
#include "TPCBaseRecSim/CDBInterface.h"
#endif

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/LaserTracks.h"
#include "TPC/Utility.h"

// root includes
#include "TCanvas.h"
#include <boost/property_tree/ptree.hpp>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::tpc
{

void LaserTracks::configure(const boost::property_tree::ptree& config)
{
  o2::tpc::CDBInterface::instance().setURL(config.get<std::string>("qc.postprocessing." + getID() + ".dataSourceURL"));
}

void LaserTracks::initialize(Trigger, framework::ServiceRegistryRef)
{
  addAndPublish(getObjectsManager(),
                mLaserTracksCanvasVec,
                { "Ltr_Coverage", "Calib_Values", "Ltr_dEdx" },
                mStoreMap);
}

void LaserTracks::update(Trigger t, framework::ServiceRegistryRef)
{
  ILOG(Info, Support) << "Trigger type is: " << t.triggerType << ", the timestamp is " << t.timestamp << ENDM;

  const auto& calibData = o2::tpc::CDBInterface::instance().getSpecificObjectFromCDB<o2::tpc::LtrCalibData>("TPC/Calib/LaserTracks", mTimestamp, mLookupMap);

  auto vecPtr = toVector(mLaserTracksCanvasVec);
  o2::tpc::painter::makeSummaryCanvases(calibData, &vecPtr);
}

void LaserTracks::finalize(Trigger t, framework::ServiceRegistryRef)
{
  for (const auto& canvas : mLaserTracksCanvasVec) {
    getObjectsManager()->stopPublishing(canvas.get());
  }
  mLaserTracksCanvasVec.clear();
}

} // namespace o2::quality_control_modules::tpc
