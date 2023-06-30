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
/// \file   RawDigits.h
/// \author Jens Wiechula
/// \author Thomas Klemenz
///

#ifndef QC_MODULE_TPC_RAWDIGITS_H
#define QC_MODULE_TPC_RAWDIGITS_H

// O2 includes
#include "TPCQC/CalPadWrapper.h"
#include "TPCReconstruction/RawReaderCRU.h"
#include "TPCWorkflow/CalibProcessingHelper.h"

// QC includes
#include "QualityControl/TaskInterface.h"
#include "TPC/ClustersData.h"

class TCanvas;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::tpc
{

/// \brief Example Quality Control DPL Task
/// It is final because there is no reason to derive from it. Just remove it if needed.
/// \author Barthelemy von Haller
/// \author Piotr Konopka
class RawDigits /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  RawDigits();
  /// \brief Destructor
  ~RawDigits() = default;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  bool mIsMergeable = true;
  ClustersData mRawDigitQC{ "N_RawDigits" };                    ///< O2 Cluster task to perform actions on cluster objects
  std::vector<o2::tpc::qc::CalPadWrapper> mWrapperVector{};     ///< vector holding CalPad objects wrapped as TObjects; published on QCG; will be non-wrapped CalPad objects in the future
  std::vector<std::unique_ptr<TCanvas>> mNRawDigitsCanvasVec{}; ///< summary canvases of the NRawDigits object
  std::vector<std::unique_ptr<TCanvas>> mQMaxCanvasVec{};       ///< summary canvases of the QMax object
  std::vector<std::unique_ptr<TCanvas>> mQTotCanvasVec{};       ///< summary canvases of the QTot object
  std::vector<std::unique_ptr<TCanvas>> mSigmaTimeCanvasVec{};  ///< summary canvases of the SigmaTime object
  std::vector<std::unique_ptr<TCanvas>> mSigmaPadCanvasVec{};   ///< summary canvases of the SigmaPad object
  std::vector<std::unique_ptr<TCanvas>> mTimeBinCanvasVec{};    ///< summary canvases of the TimeBin object
  o2::tpc::rawreader::RawReaderCRUManager mRawReader;
};

} // namespace o2::quality_control_modules::tpc

#endif // QC_MODULE_TPC_RawDigits_H
