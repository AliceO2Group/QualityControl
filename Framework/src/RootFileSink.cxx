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
/// \file   RootFileSink.cxx
/// \author Piotr Konopka
///

#include "QualityControl/RootFileSink.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObjectCollection.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/RootFileStorage.h"
#include <Framework/DeviceSpec.h>
#include <Framework/CompletionPolicyHelpers.h>
#include <Framework/CompletionPolicy.h>
#include <Framework/InputRecordWalker.h>

#if defined(__linux__) && __has_include(<malloc.h>)
#include <malloc.h>
#endif

using namespace o2::framework;

namespace o2::quality_control::core
{

RootFileSink::RootFileSink(std::string filePath)
  : mFilePath(std::move(filePath))
{
}

void RootFileSink::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  auto matcher = [label = RootFileSink::getLabel()](framework::DeviceSpec const& device) {
    return std::find(device.labels.begin(), device.labels.end(), label) != device.labels.end();
  };

  policies.emplace_back(CompletionPolicyHelpers::consumeWhenAny("qcRootFileSinkCompletionPolicy", matcher));
}

void RootFileSink::init(framework::InitContext& ictx)
{
}

void RootFileSink::run(framework::ProcessingContext& pctx)
{
  try {
    RootFileStorage mStorage{ mFilePath, RootFileStorage::ReadMode::Update };
    for (const auto& input : InputRecordWalker(pctx.inputs())) {
      auto moc = DataRefUtils::as<MonitorObjectCollection>(input);
      if (moc == nullptr) {
        ILOG(Error) << "Could not cast the input object to MonitorObjectCollection, skipping." << ENDM;
        continue;
      }
      ILOG(Info, Support) << "Received MonitorObjectCollection '" << moc->GetName() << "'" << ENDM;
      moc->postDeserialization();
      auto mwMOC = dynamic_cast<MonitorObjectCollection*>(moc->cloneMovingWindow());

      if (moc->GetEntries() > 0) {
        mStorage.storeIntegralMOC(moc.get());
      }
      if (mwMOC->GetEntries() > 0) {
        mStorage.storeMovingWindowMOC(mwMOC);
      }
    }
  } catch (const std::bad_alloc& ex) {
    ILOG(Error, Ops) << "Caught a bad_alloc exception, there is probably a huge file or object present, but I will try to survive" << ENDM;
    ILOG(Error, Support) << "Details: " << ex.what() << ENDM;
  } catch (...) {
    throw;
  }

#if defined(__linux__) && __has_include(<malloc.h>)
  // Once we write object to TFile, the OS does not actually release the array memory from the heap,
  // despite deleting the pointers. This function encourages the system to release it.
  // Unfortunately there is no platform-independent method for this, while we see a similar
  // (or even worse) behaviour on MacOS.
  // See the ROOT forum issues for additional details:
  // https://root-forum.cern.ch/t/should-the-result-of-tdirectory-getdirectory-be-deleted/53427
  malloc_trim(0);
#endif
}

} // namespace o2::quality_control::core
