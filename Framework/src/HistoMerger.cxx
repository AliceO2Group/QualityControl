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
/// \file   HistoMerger.cxx
/// \author Piotr Konopka
///

#include "QualityControl/HistoMerger.h"
#include <Framework/DataSpecUtils.h>
#include <Framework/DataRefUtils.h>
#include <TObjArray.h>

using o2::header::DataDescription;
using o2::header::DataOrigin;
using SubSpecificationType = o2::header::DataHeader::SubSpecificationType;
using namespace o2::framework;

namespace o2::quality_control::core
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
      auto concreteOutput = framework::DataSpecUtils::asConcreteDataMatcher(mOutputSpec);
      ctx.outputs().snapshot(Output{ concreteOutput.origin, concreteOutput.description, concreteOutput.subSpec }, mMergedArray);
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

} // namespace o2::quality_control::core