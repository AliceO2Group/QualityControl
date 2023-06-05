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
/// \file   DataCompressionQcTask.h
/// \author Thomas Klemenz
///

#ifndef QC_MODULE_GLO_DATACOMPRESSIONQCTASK_H
#define QC_MODULE_GLO_DATACOMPRESSIONQCTASK_H

// O2 includes
#include "DetectorsCommonDataFormats/CTFIOSize.h"

// QC includes
#include "QualityControl/TaskInterface.h"

// root includes
#include "TCanvas.h"
#include "TH1.h"

#include <unordered_map>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::glo
{

/// \brief DataCompression QC Task
/// \author Thomas Klemenz
class DataCompressionQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  DataCompressionQcTask() = default;
  /// Destructor
  ~DataCompressionQcTask() = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

  /// process the message for a single detector
  void processMessage(const o2::ctf::CTFIOSize& ctfEncRep, const std::string detector);

 private:
  bool mIsMergeable = false;                                                            // switch for canvas output: true->no canvas, false->canvas
  std::unordered_map<std::string, std::vector<std::unique_ptr<TH1>>> mCompressionHists; // contains two histograms for all active detector (as specified in the config)
  std::unique_ptr<TCanvas> mCompressionCanvas;                                          // displays the compression histograms for all active detectors
  std::unique_ptr<TCanvas> mEntropyCompressionCanvas;                                   // displays the entropy compression histograms for all active detectors
};

} // namespace o2::quality_control_modules::glo

#endif // QC_MODULE_GLO_DATACOMPRESSION_H
