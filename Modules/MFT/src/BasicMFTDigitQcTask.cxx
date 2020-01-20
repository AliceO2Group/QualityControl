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
/// \file   BasicMFTDigitQcTask.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include <TCanvas.h>
#include <TH1.h>

#include "ITSMFTBase/Digit.h"
#include "QualityControl/QcInfoLogger.h"
#include "MFT/BasicMFTDigitQcTask.h"

namespace o2::quality_control_modules::mft
{

BasicMFTDigitQcTask::~BasicMFTDigitQcTask()
{
  /*
    not needed for unique pointers
  */
}

void BasicMFTDigitQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize BasicMFTDigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  MFT_chip_index_H = std::make_unique<TH1F>("MFT_chip_index", "MFT_chip_index", 936,-0.5,935.5);
  getObjectsManager()->startPublishing(MFT_chip_index_H.get());
  getObjectsManager()->addMetadata(MFT_chip_index_H->GetName(), "custom", "34");
  getObjectsManager()->addCheck(MFT_chip_index_H.get(), "BasicMFTDigitQcCheck", "o2::quality_control_modules::mft::BasicMFTDigitQcCheck", "QcMFT");
}

void BasicMFTDigitQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
  MFT_chip_index_H->Reset();
}

void BasicMFTDigitQcTask::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void BasicMFTDigitQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the digits
  auto digits = ctx.inputs().get<const std::vector<o2::itsmft::Digit>>("randomdigit");
  if (digits.size() < 1) return;
  /// fill the histograms
  for ( auto &one_digit : digits ) {
    MFT_chip_index_H->Fill(one_digit.getChipIndex());
  }
}

void BasicMFTDigitQcTask::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
}

void BasicMFTDigitQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void BasicMFTDigitQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info) << "Resetting the histogram" << ENDM;
  MFT_chip_index_H->Reset();
}

} // namespace o2::quality_control_modules::mft
