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
/// \file   OrbitsPlotter.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_ORBITSPLOTTER_H
#define QC_MODULE_ORBITSPLOTTER_H

#include "MUONCommon/HistPlotter.h"
#include "MCHRawElecMap/Mapper.h"
#include <TH2F.h>

using namespace o2::quality_control_modules::muon;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

class OrbitsPlotter : public HistPlotter
{
 public:
  OrbitsPlotter(std::string path);

  void update(TH2F* h);

 private:
  void addHisto(TH1* h, bool statBox, const char* drawOptions, const char* displayHints)
  {
    h->SetOption(drawOptions);
    if (!statBox) {
      h->SetStats(0);
    }
    histograms().emplace_back(HistInfo{ h, drawOptions, displayHints });
  }

  o2::mch::raw::Elec2DetMapper mElec2DetMapper;
  o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper;

  std::unique_ptr<TH2F> mHistogramOrbits;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_ORBITSPLOTTER_H
