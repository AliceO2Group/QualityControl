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
/// \file   HistoMerger.h
/// \author Piotr Konopka
///

#ifndef QC_CORE_HISTOMERGER_H
#define QC_CORE_HISTOMERGER_H

#include <string>
#include <memory>
#include <vector>

#include <TH1.h>
#include <TObjArray.h>

#include <Common/Timer.h>
#include <Framework/Task.h>
#include <Framework/InputSpec.h>
#include <Framework/OutputSpec.h>
#include <Headers/DataHeader.h>

#include "QualityControl/MonitorObject.h"

namespace o2::quality_control::core
{

/// \brief A crude histogram merger for development purposes.
///
/// A crude histogram merger for development purposes - at some point, it will be substituted with more fine solution.
/// As inputs, it expects updates of MonitorObjects with one TH1 histogram each, which are accumulated into one
/// MonitorObject. The joined MO is published on regular basis with a period specified in the constructor. All inputs
/// should have the same DataOrigin and DataDescription and non-zero SubSpecification. Output has the same origin and
/// description as inputs, but SubSpec is fixed 0.
class HistoMerger : public framework::Task
{
 public:
  /// Constructor
  explicit HistoMerger(std::string mergerName, double publicationPeriodSeconds = 10);

  /// Destructor
  ~HistoMerger() override;

  /// \brief HistoMerger init callback
  void init(framework::InitContext& ctx) override;

  /// \brief HistoMerger process callback
  void run(framework::ProcessingContext& ctx) override;

  void configureInputsOutputs(
    o2::header::DataOrigin origin, o2::header::DataDescription description,
    std::pair<o2::header::DataHeader::SubSpecificationType, o2::header::DataHeader::SubSpecificationType> subSpecRange);

  std::string getName() { return mMergerName; };
  std::vector<o2::framework::InputSpec> getInputSpecs() { return mInputSpecs; };
  framework::OutputSpec getOutputSpec() { return mOutputSpec; };

 private:
  // General state
  std::string mMergerName;
  TObjArray mMergedArray;
  AliceO2::Common::Timer mPublicationTimer;

  // DPL
  std::vector<o2::framework::InputSpec> mInputSpecs;
  o2::framework::OutputSpec mOutputSpec;
};

} // namespace o2::quality_control::core

#endif // QC_CORE_HISTOMERGER_H
