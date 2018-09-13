///
/// \file   HistoMerger.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_HISTOMERGER_H
#define QUALITYCONTROL_HISTOMERGER_H

#include <vector>
#include <memory>

#include <TH1.h>
#include <Framework/Task.h>
#include <Headers/DataHeader.h>
#include "Common/Timer.h"

#include "QualityControl/MonitorObject.h"

namespace o2 {
namespace quality_control {
namespace core {


/// \brief A crude histogram merger for development purposes.
class HistoMerger : public framework::Task {
public:
  /// Constructor
  HistoMerger(std::string mergerName, double publicationPeriodSeconds = 10);

  /// Destructor
  ~HistoMerger() override;

  /// \brief Checker init callback
  void init(framework::InitContext& ctx) override;

  /// \brief Checker process callback
  void run(framework::ProcessingContext& ctx) override;

  void configureEdges(
    o2::header::DataOrigin origin, o2::header::DataDescription description,
    std::pair<o2::header::DataHeader::SubSpecificationType, o2::header::DataHeader::SubSpecificationType> subSpecRange);

  std::string getName() { return mMergerName; };
  std::vector<o2::framework::InputSpec>  getInputSpecs() { return mInputSpecs; };
  framework::OutputSpec getOutputSpec() { return mOutputSpec; };

private:
  // General state
  std::string mMergerName;
  std::shared_ptr<MonitorObject> mMonitorObject;
  AliceO2::Common::Timer mPublicationTimer;

  // DPL
  std::vector<o2::framework::InputSpec> mInputSpecs;
  o2::framework::OutputSpec mOutputSpec;
};

}
}
}

#endif // QUALITYCONTROL_HISTOMERGER_H
