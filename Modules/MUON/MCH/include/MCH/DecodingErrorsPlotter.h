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
/// \file   DecodingErrorsPlotter.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_DECODINGERRORSPLOTTER_H
#define QC_MODULE_DECODINGERRORSPLOTTER_H

#include "MUONCommon/HistPlotter.h"
#include "MCHRawElecMap/Mapper.h"
#include <TH1F.h>
#include <TH2F.h>

using namespace o2::quality_control_modules::muon;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

class DecodingErrorsPlotter : public HistPlotter
{
 public:
  DecodingErrorsPlotter(std::string path);

  void update(TH2F* h2);

 private:
  void addHisto(TH1* h, bool statBox, const char* drawOptions, const char* displayHints)
  {
    h->SetOption(drawOptions);
    if (!statBox) {
      h->SetStats(0);
    }
    histograms().emplace_back(HistInfo{ h, drawOptions, displayHints });
  }

  o2::mch::raw::Det2ElecMapper mDet2ElecMapper;
  o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper;

  std::string mPath;

  std::unique_ptr<TH1F> mHistogramGoodBoardsPerDE;  ///< fraction of boards with errors per DE
  std::unique_ptr<TH2F> mHistogramErrorsPerDE;      ///< error codes per DE
  std::unique_ptr<TH2F> mHistogramErrorsPerChamber; ///< error codes per chamber
  std::unique_ptr<TH2F> mHistogramErrorsPerFeeId;   ///< error codes per FEE ID
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_DECODINGERRORSPLOTTER_H
