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
// file  CalPadClusterReductor.cxx
// author Marcel Lesch
//

#include "TPC/CalPadClusterReductor.h"
#include "TPC/ClustersData.h"
#include "QualityControl/QcInfoLogger.h"
#include <TMath.h>
#include <vector>
#include <algorithm>
#include <cmath>

namespace o2::quality_control_modules::tpc
{

void* CalPadClusterReductor::getBranchAddress()
{
  return &mCalPad;
} // void* CalPadClusterReductor::getBranchAddress()

const char* CalPadClusterReductor::getBranchLeafList()
{
  return "NClusters[4][72]/F:QMax[4][72]:QTot[4][72]:SigmaTime[4][72]:SigmaPad[4][72]:TimeBin[4][72]";
} // const char* CalPadClusterReductor::getBranchLeafList()

void CalPadClusterReductor::update(TObject* obj)
{
  if (obj) {
    auto qcClusters = dynamic_cast<ClustersData*>(obj);

    if (qcClusters) {
      auto& clusters = qcClusters->getClusters();

      for (int iType = 0; iType < 6; iType++) {
        auto& calDet = GetCalPad(clusters, iType);

        for (size_t iROC = 0; iROC < calDet.getData().size(); ++iROC) {
          auto& calArray = calDet.getCalArray(iROC);
          auto& data = calArray.getData();

          // Remove pads which are empty from any calculations
          data.erase(std::remove_if(data.begin(), data.end(), [](const auto& value) { return (std::isnan(value) || value <= 0); }), data.end());

          Float_t(*Quantity)[72] = getArrayPointer(iType);
          if ((*Quantity)[0]) {
            Quantity[0][iROC] = static_cast<float>(data.size());
            Quantity[1][iROC] = TMath::Mean(data.begin(), data.end());
            Quantity[2][iROC] = TMath::StdDev(data.begin(), data.end());
            Quantity[3][iROC] = TMath::Median(data.size(), data.data());
          }
        } // for (size_t iROC = 0; iROC < calDet.getData().size(); ++iROC)
      }   // for(int iType = 0; iType < 6; iType++)
    }     // if (qcClusters)
  }       // if(obj)
} // void CalPadClusterReductor::update(TObject* obj)

o2::tpc::CalPad& CalPadClusterReductor::GetCalPad(o2::tpc::qc::Clusters& clusters, int dataType)
{
  if (dataType == 0) {
    return clusters.getNClusters();
  } else if (dataType == 1) {
    return clusters.getQMax();
  } else if (dataType == 2) {
    return clusters.getQTot();
  } else if (dataType == 3) {
    return clusters.getSigmaTime();
  } else if (dataType == 4) {
    return clusters.getSigmaPad();
  } else if (dataType == 5) {
    return clusters.getTimeBin();
  } else {
    ILOG(Error, Support) << "Error: Datatype not supported in CalPadClusterReductor" << ENDM;
    o2::tpc::CalPad Dummy(0);
    return Dummy;
  }
} // o2::tpc::CalPad& CalPadClusterReductor::GetCalPad(o2::tpc::qc::Clusters& clusters, int dataType)

CalPadClusterReductor::pointer_to_arrays CalPadClusterReductor::getArrayPointer(int dataType)
{
  if (dataType == 0) {
    return mCalPad.NClusters;
  } else if (dataType == 1) {
    return mCalPad.QMax;
  } else if (dataType == 2) {
    return mCalPad.QTot;
  } else if (dataType == 3) {
    return mCalPad.SigmaTime;
  } else if (dataType == 4) {
    return mCalPad.SigmaPad;
  } else if (dataType == 5) {
    return mCalPad.TimeBin;
  } else {
    ILOG(Error, Support) << "Error: Datatype not supported in CalPadClusterReductor" << ENDM;
    return NULL;
  }
} // CalPadClusterReductor::pointer_to_arrays CalPadClusterReductor::getArrayPointer(int dataType)

} // namespace o2::quality_control_modules::tpc
