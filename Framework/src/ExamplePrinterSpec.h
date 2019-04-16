//
// Created by bvonhall on 16/04/19.
//

#ifndef QUALITYCONTROL_EXAMPLEPRINTERSPEC_H
#define QUALITYCONTROL_EXAMPLEPRINTERSPEC_H

namespace o2::quality_control::core
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

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_EXAMPLEPRINTERSPEC_H
