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
/// \file   TrendingTaskTPC.h
/// \author Marcel Lesch
/// \author Cindy Mordasini
/// \author Based on the work from Piotr Konopka
///

#ifndef QUALITYCONTROL_SLICEINFO_H
#define QUALITYCONTROL_SLICEINFO_H

#include <Rtypes.h>

namespace o2::quality_control_modules::tpc
{
/// \brief  A post-processing task tuned for the needs of the trending of the TPC.
///
/// A post-processing task which trends TPC related objects inside QC database (QCDB).
/// It extracts some values of one or multiple objects using the Reductor classes,
/// then stores them inside a TTree. The class exposes the TTree::Draw interface
/// to the user tp generate the plots out of the TTree.
/// This class is specific to the TPC: a subrange slicer is available in the json,
/// and input/output canvas can be dealt with alongside normal histograms.
///
/// \author Marcel Lesch
/// \author Cindy Mordasini
/// \author Based on the work from Piotr Konopka

struct SliceInfo {
  double entries;
  double meanX;
  double stddevX;
  double errMeanX;
  double meanY;
  double stddevY;
  double errMeanY;

  ClassDefNV(SliceInfo,1);
};

} // namespace o2::quality_control_modules::tpc

#endif //QUALITYCONTROL_SLICEINFO_H