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

#include <Framework/ControlService.h>
#include <TFile.h>
#include <TKey.h>

using namespace o2::framework;

namespace o2::quality_control::core
{
RootFileSource::RootFileSource(std::string filePath)
  : mFilePath(std::move(filePath))
{
}

void RootFileSource::init(framework::InitContext&)
{
}

void RootFileSource::run(framework::ProcessingContext& ctx)
{
  auto* file = new TFile(mFilePath.c_str(), "READ");
  if (file->IsZombie()) {
    throw std::runtime_error("File '" + mFilePath + "' is zombie.");
  }
  if (!file->IsOpen()) {
    throw std::runtime_error("Failed to open the file: " + mFilePath);
  }
  ILOG(Info) << "Input file '" << mFilePath << "' successfully open." << ENDM;

  TIter next(file->GetListOfKeys());
  TKey* key;
  while ((key = (TKey*)next())) {
    auto storedTObj = file->Get(key->GetName());
    if (storedTObj != nullptr) {
      auto storedMOC = dynamic_cast<MonitorObjectCollection*>(storedTObj);
      if (storedMOC == nullptr) {
        ILOG(Error) << "Could not cast the stored object to MonitorObjectCollection, skipping." << ENDM;
        delete storedTObj;
        continue;
      }

      // snapshot does a shallow copy, so we cannot let it delete elements in MOC when it deletes the MOC
      storedMOC->SetOwner(false);
      ctx.outputs().snapshot(OutputRef{ storedMOC->GetName(), 0 }, *storedMOC);
      storedMOC->SetOwner(true);
      ILOG(Info) << "Read and published object '" << storedMOC->GetName() << "'" << ENDM;
    }
    delete storedTObj;
  }
  file->Close();
  delete file;

  ctx.services().get<ControlService>().endOfStream();
  ctx.services().get<ControlService>().readyToQuit(QuitRequest::Me);
}

} // namespace o2::quality_control::core