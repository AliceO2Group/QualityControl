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
#include <Framework/DeviceSpec.h>
#include <Framework/CompletionPolicyHelpers.h>
#include <Framework/CompletionPolicy.h>
#include <Framework/InputRecordWalker.h>
#include <TFile.h>

using namespace o2::framework;

namespace o2::quality_control::core
{

RootFileSink::RootFileSink(std::string filePath)
  : mFilePath(std::move(filePath))
{
}

TFile* openSinkFile(const std::string& name)
{
  auto file = new TFile(name.c_str(), "UPDATE");
  if (file->IsZombie()) {
    throw std::runtime_error("File '" + name + "' is zombie.");
  }
  if (!file->IsOpen()) {
    throw std::runtime_error("Failed to open the file: " + name);
  }
  if (!file->IsWritable()) {
    throw std::runtime_error("File '" + name + "' is not writable.");
  }
  ILOG(Info) << "Output file '" << name << "' successfully open." << ENDM;
  return file;
}

void closeSinkFile(TFile* file)
{
  if (file != nullptr) {
    if (file->IsOpen()) {
      ILOG(Info) << "Closing file '" << file->GetName() << "'." << ENDM;
      file->Close();
    }
    delete file;
  }
}

RootFileSink::~RootFileSink()
{
}

void RootFileSink::customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies)
{
  auto matcher = [label = RootFileSink::getLabel()](framework::DeviceSpec const& device) {
    return std::find(device.labels.begin(), device.labels.end(), label) != device.labels.end();
  };
  auto callback = CompletionPolicyHelpers::consumeWhenAny().callback;

  policies.emplace_back("qcRootFileSinkCompletionPolicy", matcher, callback);
}

void RootFileSink::init(framework::InitContext& ictx)
{
}

void RootFileSink::run(framework::ProcessingContext& pctx)
{
  TFile* sinkFile = nullptr;
  try {
    sinkFile = openSinkFile(mFilePath);
    for (const auto& input : InputRecordWalker(pctx.inputs())) {
      auto moc = DataRefUtils::as<MonitorObjectCollection>(input).release();
      if (moc == nullptr) {
        ILOG(Error) << "Could not cast the input object to MonitorObjectCollection, skipping." << ENDM;
        continue;
      }
      ILOG(Info, Support) << "Received MonitorObjectCollection '" << moc->GetName() << "'" << ENDM;
      moc->postDeserialization();

      auto mocName = moc->GetName();
      if (*mocName == '\0') {
        ILOG(Error, Support) << "MonitorObjectCollection does not have a name, skipping." << ENDM;
        continue;
      }

      ILOG(Info, Support) << "Checking for existing objects in the file." << ENDM;
      auto storedTObj = sinkFile->Get(mocName);
      if (storedTObj != nullptr) {
        auto storedMOC = dynamic_cast<MonitorObjectCollection*>(storedTObj);
        if (storedMOC == nullptr) {
          ILOG(Error, Ops) << "Could not cast the stored object to MonitorObjectCollection, skipping." << ENDM;
          delete storedTObj;
          continue;
        }
        storedMOC->postDeserialization();
        ILOG(Info, Support) << "Merging object '" << moc->GetName() << "' with the existing one in the file." << ENDM;
        moc->merge(storedMOC);
      }
      delete storedTObj;

      auto nbytes = sinkFile->WriteObject(moc, moc->GetName(), "Overwrite");
      ILOG(Info, Support) << "Object '" << moc->GetName() << "' has been stored in the file (" << nbytes << " bytes)." << ENDM;
      delete moc;
    }
    closeSinkFile(sinkFile);
  } catch (const std::bad_alloc& ex) {
    ILOG(Error, Ops) << "Caught a bad_alloc exception, there is probably a huge file or object present, but I will try to survive" << ENDM;
    ILOG(Error, Support) << "Details: " << ex.what() << ENDM;
    closeSinkFile(sinkFile);
  } catch (...) {
    closeSinkFile(sinkFile);
    throw;
  }
}

} // namespace o2::quality_control::core
