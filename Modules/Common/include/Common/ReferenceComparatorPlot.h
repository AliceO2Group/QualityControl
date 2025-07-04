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
  /// ReferenceComparatorPlot constructor
  /// \param referenceHistogram pointer to the reference histogram object, used to initialize the internal plots
  /// \param outputPath QCDB path were the output canvas is stored
  /// \param scaleReference if true the reference plot is sclaled such that its integral matches the one of the current histogram
  /// \param drawRatioOnly if true only  the ratio between current and reference plot is draw, otherwise also the individual plots are drawn in addition
  /// \param drawOption1D ROOT draw option to be used for 1-D plots
  /// \param drawOption2D ROOT draw option to be used for 2-D plots
  ReferenceComparatorPlot(TH1* referenceHistogram,
                          int referenceRun,
                          const std::string& outputPath,
                          bool scaleReference,
                          bool drawRatioOnly,
                          double legendHeight,
                          const std::string& drawOption1D,
                          const std::string& drawOption2D);
  virtual ~ReferenceComparatorPlot() = default;

  TObject* getMainCanvas();
  void update(TH1* histogram);

 private:
  std::shared_ptr<ReferenceComparatorPlotImpl> mImplementation;
};

} // namespace o2::quality_control_modules::common

#endif // QUALITYCONTROL_REFERENCECOMPARATORTASK_H
