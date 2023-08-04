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
/// \file   RootFileSource.cxx
/// \author Piotr Konopka
///

#include "QualityControl/RootFileSource.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObjectCollection.h"
#include "QualityControl/RootFileStorage.h"

#include <Framework/ControlService.h>
#include <Framework/DeviceSpec.h>
#include <TFile.h>
#include <TKey.h>

using namespace o2::framework;

namespace o2::quality_control::core
{
RootFileSource::RootFileSource(std::string filePath)
  : mFilePath(std::move(filePath))
{
}

void RootFileSource::init(framework::InitContext& ctx)
{
  auto const& deviceSpec = ctx.services().get<DeviceSpec const>();
  mAllowedOutputs.clear();
  mAllowedOutputs.reserve(deviceSpec.outputs.size());
  for (const auto& outputRoute : deviceSpec.outputs) {
    mAllowedOutputs.push_back(outputRoute.matcher.binding);
  }

  mRootFileManager = std::make_shared<RootFileStorage>(mFilePath, RootFileStorage::ReadMode::Read);
  if (mRootFileManager == nullptr) {
    ILOG(Fatal, Ops) << "Could not open file '" << mFilePath << "'" << ENDM;
    ctx.services().get<ControlService>().endOfStream();
    ctx.services().get<ControlService>().readyToQuit(false);
    return;
  }
  ILOG(Info) << "Input file '" << mFilePath << "' successfully open." << ENDM;

  auto fileStructure = mRootFileManager->readStructure(false);

  mIntegralMocWalker = std::make_shared<IntegralMocWalker>(fileStructure);
  mMovingWindowMocWalker = std::make_shared<MovingWindowMocWalker>(fileStructure);
}

void RootFileSource::run(framework::ProcessingContext& ctx)
{
  if (mIntegralMocWalker->hasNextPath()) {
    const auto& path = mIntegralMocWalker->nextPath();
    auto moc = mRootFileManager->readMonitorObjectCollection(path);
    auto binding = outputBinding(moc->getDetector(), moc->getTaskName(), false);

    if (std::find_if(mAllowedOutputs.begin(), mAllowedOutputs.end(),
                     [binding](const auto& other) { return other.value == binding.value; }) == mAllowedOutputs.end()) {
      ILOG(Error) << "The MonitorObjectCollection '" << binding.value << "' is not among declared output bindings: ";
      for (const auto& output : mAllowedOutputs) {
        ILOG(Error) << output.value << " ";
      }
      ILOG(Error) << ", skipping." << ENDM;
      return;
    }
    // snapshot does a shallow copy, so we cannot let it delete elements in MOC when it deletes the MOC
    moc->SetOwner(false);
    ctx.outputs().snapshot(OutputRef{ binding.value, 0 }, *moc);
    moc->postDeserialization();
    ILOG(Info) << "Read and published object '" << path << "'" << ENDM;
    delete moc;

    return;
  }

  if (mMovingWindowMocWalker->hasNextPath()) {
    const auto& path = mMovingWindowMocWalker->nextPath();
    auto moc = mRootFileManager->readMonitorObjectCollection(path);
    auto binding = outputBinding(moc->getDetector(), moc->getTaskName(), true);

    if (std::find_if(mAllowedOutputs.begin(), mAllowedOutputs.end(),
                     [binding](const auto& other) { return other.value == binding.value; }) == mAllowedOutputs.end()) {
      ILOG(Error) << "The MonitorObjectCollection '" << binding.value << "' is not among declared output bindings: ";
      for (const auto& output : mAllowedOutputs) {
        ILOG(Error) << output.value << " ";
      }
      ILOG(Error) << ", skipping." << ENDM;
      return;
    }
    // snapshot does a shallow copy, so we cannot let it delete elements in MOC when it deletes the MOC
    moc->SetOwner(false);
    ctx.outputs().snapshot(OutputRef{ binding.value, 0 }, *moc);
    moc->postDeserialization();
    ILOG(Info) << "Read and published object '" << path << "'" << ENDM;
    delete moc;

    return;
  }

  mRootFileManager.reset();

  ctx.services().get<ControlService>().endOfStream();
  ctx.services().get<ControlService>().readyToQuit(QuitRequest::Me);
}

framework::OutputLabel
  RootFileSource::outputBinding(const std::string& detectorCode, const std::string& taskName, bool movingWindow)
{
  return movingWindow ? framework::OutputLabel{ detectorCode + "-MW-" + taskName } : framework::OutputLabel{ detectorCode + "-" + taskName };
}

} // namespace o2::quality_control::core
