///
/// \file   HistoMerger.cxx
/// \author Piotr Konopka
///

#include "QualityControl/HistoMerger.h"

#include <Framework/DataRefUtils.h>
#include <TObjArray.h>

namespace o2
{
using header::DataDescription;
using header::DataOrigin;
using SubSpecificationType = header::DataHeader::SubSpecificationType;
using namespace framework;
namespace quality_control
{
namespace core
{

HistoMerger::HistoMerger(std::string mergerName, double publicationPeriodSeconds)
  : mMergerName(mergerName), mOutputSpec{ header::gDataOriginInvalid, header::gDataDescriptionInvalid }
{
  mPublicationTimer.reset(static_cast<int>(publicationPeriodSeconds * 1000000));
  mMergedArray.SetOwner(true);
}

HistoMerger::~HistoMerger() {}

void HistoMerger::init(framework::InitContext&) { mMergedArray.Clear(); }

void HistoMerger::run(framework::ProcessingContext& ctx)
{
  for (const auto& input : ctx.inputs()) {
    if (input.header != nullptr && input.spec != nullptr) {
      std::unique_ptr<TObjArray> moArray = DataRefUtils::as<TObjArray>(input);

      if (mMergedArray.IsEmpty()) {
        mMergedArray = *moArray.release();
      } else {
        if (mMergedArray.GetSize() != moArray->GetSize()) {
          LOG(ERROR) << "array don't match in size, " << mMergedArray.GetSize() << " vs " << moArray->GetSize();
          return;
        }

        for (int i = 0; i < mMergedArray.GetEntries(); i++) {
          MonitorObject* mo = dynamic_cast<MonitorObject*>((*moArray)[i]);
          if (mo && std::strstr(mo->getObject()->ClassName(), "TH1") != nullptr) {
            TH1* h = dynamic_cast<TH1*>(dynamic_cast<MonitorObject*>(mMergedArray[i])->getObject());
            const TH1* hUpdate = dynamic_cast<TH1*>(mo->getObject());
            h->Add(hUpdate);
          }
        }
      }
    }
  }
  if (mPublicationTimer.isTimeout()) {
    if (!mMergedArray.IsEmpty()) {
      ctx.outputs().snapshot(Output{ mOutputSpec.origin, mOutputSpec.description, mOutputSpec.subSpec }, mMergedArray);
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