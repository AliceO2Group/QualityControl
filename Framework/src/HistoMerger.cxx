///
/// \file   HistoMerger.cxx
/// \author Piotr Konopka
///

#include "QualityControl/HistoMerger.h"

#include <Framework/DataRefUtils.h>

namespace o2
{
using header::DataOrigin;
using header::DataDescription;
using SubSpecificationType = header::DataHeader::SubSpecificationType;
using namespace framework;
namespace quality_control
{
namespace core
{

HistoMerger::HistoMerger(std::string mergerName, double publicationPeriodSeconds)
  : mMergerName(mergerName),
    mOutputSpec{ header::gDataOriginInvalid, header::gDataDescriptionInvalid }
{
  mPublicationTimer.reset(static_cast<int>(publicationPeriodSeconds * 1000000));
}

HistoMerger::~HistoMerger() {}

void HistoMerger::init(framework::InitContext& ctx)
{
  mMonitorObject.reset();
}

void HistoMerger::run(framework::ProcessingContext& ctx)
{
  for (const auto& input : ctx.inputs()) {
    if (input.header != nullptr && input.spec != nullptr) {

      if (!mMonitorObject) {
        mMonitorObject.reset(DataRefUtils::as<MonitorObject>(input).release());
      } else {
        TH1* h = dynamic_cast<TH1*>(mMonitorObject->getObject());
        const TH1* hUpdate = dynamic_cast<TH1*>(DataRefUtils::as<MonitorObject>(input)->getObject());
        h->Add(hUpdate);
      }
    }
  }
  if (mPublicationTimer.isTimeout()) {
    if (mMonitorObject) {
      ctx.outputs().snapshot<MonitorObject>(Output{ mOutputSpec.origin, mOutputSpec.description, mOutputSpec.subSpec },
                                            *mMonitorObject);
    }

    // avoid publishing mo many times consecutively because of too long initial waiting time
    do {
      mPublicationTimer.increment();
    } while (mPublicationTimer.isTimeout());
  }
}

void HistoMerger::configureInputsOutputs(DataOrigin origin, DataDescription description,
                                         std::pair<SubSpecificationType, SubSpecificationType> subSpecRange)
{
  mInputSpecs.clear();

  for (SubSpecificationType s = subSpecRange.first; s <= subSpecRange.second; s++) {
    mInputSpecs.push_back({ "mo", origin, description, s });
  }
  mOutputSpec = OutputSpec{ origin, description, 0 };
}

} // namespace core
} // namespace quality_control
} // namespace o2