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
/// \file   BasicDigitQcTask.cxx
/// \author Tomas Herman
/// \author Guillermo Contreras
///

// ROOT
#include <TCanvas.h>
#include <TH1.h>
// O2
#include <DataFormatsITSMFT/Digit.h>
// Quality Control
#include "QualityControl/QcInfoLogger.h"
#include "MFT/BasicDigitQcTask.h"

namespace o2::quality_control_modules::mft
{

BasicDigitQcTask::~BasicDigitQcTask()
{
  /*
    not needed for unique pointers
  */
}

void BasicDigitQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize BasicDigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  mMFT_chip_index_H = std::make_unique<TH1F>("mMFT_chip_index", "mMFT_chip_index", 936, -0.5, 935.5);
  getObjectsManager()->startPublishing(mMFT_chip_index_H.get());
  getObjectsManager()->addMetadata(mMFT_chip_index_H->GetName(), "custom", "34");
}

void BasicDigitQcTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
  mMFT_chip_index_H->Reset();
}

void BasicDigitQcTask::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

void BasicDigitQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the digits
  const auto digits = ctx.inputs().get<gsl::span<o2::itsmft::Digit>>("randomdigit");
  if (digits.size() < 1)
    return;
  /// fill the histograms
  for (auto& one_digit : digits) {
    mMFT_chip_index_H->Fill(one_digit.getChipIndex());
  }
}

void BasicDigitQcTask::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
}

void BasicDigitQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void BasicDigitQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info) << "Resetting the histogram" << ENDM;
  mMFT_chip_index_H->Reset();
}

} // namespace o2::quality_control_modules::mft
