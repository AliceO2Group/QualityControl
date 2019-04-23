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
/// \file   ExamplePrinterSpec.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_EXAMPLEPRINTERSPEC_H
#define QUALITYCONTROL_EXAMPLEPRINTERSPEC_H

namespace o2::quality_control::example
{

/**
 * \brief Example DPL task to be plugged after a QC checker.
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
    std::shared_ptr<TObjArray> moArray{ std::move(DataRefUtils::as<TObjArray>(*processingContext.inputs().begin())) };

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

} // namespace o2::quality_control::example

#endif //QUALITYCONTROL_EXAMPLEPRINTERSPEC_H
