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
/// \file    ReferenceComparatorPlot.h
/// \author  Andrea Ferrero
/// \brief   Utility class for the combined drawing of the current and reference plots, and their ratio
///

#ifndef QUALITYCONTROL_REFERENCECOMPARATORPLOT_H
#define QUALITYCONTROL_REFERENCECOMPARATORPLOT_H

#include <TH1.h>
#include <string>
#include <memory>

namespace o2::quality_control_modules::common
{

class ReferenceComparatorPlotImpl;

class ReferenceComparatorPlot
{
 public:
  ReferenceComparatorPlot(TH1* refHist, std::string outputPath, bool scaleRef, bool drawRatioOnly, std::string drawOption1D, std::string drawOption2D);
  virtual ~ReferenceComparatorPlot() = default;

  TObject* getObject();
  void update(TH1* hist, TH1* histRef);

 private:
  std::shared_ptr<ReferenceComparatorPlotImpl> mImplementation;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_REFERENCECOMPARATORTASK_H
