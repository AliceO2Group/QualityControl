///
/// \file   HistoMerger.cxx
/// \author Piotr Konopka
///

#include "QualityControl/HistoMerger.h"

#include <Framework/DataRefUtils.h>

namespace o2 {
using header::DataOrigin;
using header::DataDescription;
using SubSpecificationType = header::DataHeader::SubSpecificationType;
using namespace framework;
namespace quality_control {
namespace core {

HistoMerger::HistoMerger(std::string mergerName)
  : mMergerName(mergerName),
    mPublicationRate(10000000),
    mOutputSpec{ header::gDataOriginInvalid, header::gDataDescriptionInvalid }
{
}

HistoMerger::~HistoMerger() {}

void HistoMerger::init(framework::InitContext& ctx)
{
  mMonitorObject.reset();
  mPublicationTimer.reset(mPublicationRate); // 10 s.
}

void HistoMerger::run(framework::ProcessingContext& ctx)
{
  for (const auto& input : ctx.inputs()) {
    if (input.header != nullptr && input.spec != nullptr) {

      if (!mMonitorObject) {
        mMonitorObject.reset(DataRefUtils::as<MonitorObject>(input).release());
      }
      else {
        TH1* h = dynamic_cast<TH1*>(mMonitorObject->getObject());
        const TH1* hUpdate = dynamic_cast<TH1*>(DataRefUtils::as<MonitorObject>(input)->getObject());
        h->Add(hUpdate);
      }
    }
  }
  if (mPublicationTimer.isTimeout()) {
    mPublicationTimer.increment();

    ctx.outputs().snapshot<MonitorObject>(Output{ mOutputSpec.origin, mOutputSpec.description, mOutputSpec.subSpec },
                                          *mMonitorObject);
  }
}

void HistoMerger::configureEdges(DataOrigin origin, DataDescription description,
                                 std::pair<SubSpecificationType, SubSpecificationType> subSpecRange)
{
  mInputSpecs.clear();

  for (SubSpecificationType s = subSpecRange.first; s <= subSpecRange.second; s++) {
    mInputSpecs.push_back({ "mo", origin, description, s });
  }
  mOutputSpec = OutputSpec{ origin, description, 0 };
}

void HistoMerger::setPublicationRate(int us)
{
  mPublicationRate = us;
  mPublicationTimer.reset(us);
}
}
}
}