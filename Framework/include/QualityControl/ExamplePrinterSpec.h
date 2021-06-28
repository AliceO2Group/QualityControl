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
/// \file   ExamplePrinterSpec.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_EXAMPLEPRINTERSPEC_H
#define QUALITYCONTROL_EXAMPLEPRINTERSPEC_H

#include <string>
#include <memory>

#include <TH1F.h>
#include <TObjArray.h>

#include <Framework/Task.h>
#include <Framework/DataRefUtils.h>

#include "QualityControl/MonitorObject.h"
#include "QualityControl/QualityObject.h"

namespace o2::quality_control::example
{

/**
 * \brief Example DPL task to be plugged after a QC task.
 *
 * This example DPL task takes a TObjArray of MonitorObjects as input (corresponding to the output of a checker)
 * and prints the bins of the first element. The element needs to be a TH1 otherwise it is ignored.
 */
class ExamplePrinterSpec : public framework::Task
{
 public:
  void run(ProcessingContext& processingContext) final
  {
    LOG(INFO) << "Received data";
    std::shared_ptr<TObjArray> moArray{ DataRefUtils::as<TObjArray>(*processingContext.inputs().begin()) };

    if (moArray->IsEmpty()) {
      LOG(INFO) << "Array is empty";
      return;
    }

    // get the object
    auto* mo = dynamic_cast<MonitorObject*>(moArray->At(0));
    if (mo == nullptr) {
      LOG(INFO) << "First element is not a MonitorObject";
      return;
    }
    auto* histo = dynamic_cast<TH1F*>(mo->getObject());
    if (histo == nullptr) {
      LOG(INFO) << "MonitorObject does not contain a TH1";
      return;
    }

    std::string bins = "BINS:";
    for (int i = 0; i < histo->GetNbinsX(); i++) {
      bins += " " + std::to_string((int)histo->GetBinContent(i));
    }
    LOG(INFO) << bins;
  }
};

/**
 * \brief Example DPL task to be plugged after a QC check.
 *
 * This example DPL task takes a TObjArray of MonitorObjects as input (corresponding to the output of a checker)
 * and prints the bins of the first element. The element needs to be a TH1 otherwise it is ignored.
 */
class ExampleQualityPrinterSpec : public framework::Task
{
 public:
  void run(ProcessingContext& processingContext) final
  {
    auto qo = processingContext.inputs().get<QualityObject*>("checked-mo");

    LOG(INFO) << "Received Quality: " << qo->getQuality();
  }
};

} // namespace o2::quality_control::example

#endif //QUALITYCONTROL_EXAMPLEPRINTERSPEC_H
