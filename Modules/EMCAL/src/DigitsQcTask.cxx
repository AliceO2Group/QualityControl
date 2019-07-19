///
/// \file   DigitsQcTask.cxx
/// \author Markus Fasel
///

#include <TCanvas.h>
#include <TH2.h>

#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsEMCAL/EMCALBlockHeader.h"
#include "EMCAL/DigitsQcTask.h"
#include "DataFormatsEMCAL/Digit.h"

namespace o2
{
namespace quality_control_modules
{
namespace emcal
{

DigitsQcTask::~DigitsQcTask()
{
  if (mDigitAmplitude) {
    delete mDigitAmplitude;
  }
  if (mDigitTime) {
    delete mDigitTime;
  }
}

void DigitsQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize DigitsQcTask" << AliceO2::InfoLogger::InfoLogger::endm;

  mDigitAmplitude = new TH2F("digitAmplitude", "Digit Amplitude", 100, 0, 100, 20000, 0., 20000.);
  mDigitTime = new TH2F("digitTime", "Digit Time", 1000, 0, 1000, 20000, 0., 20000.);
  getObjectsManager()->startPublishing(mDigitAmplitude);
  getObjectsManager()->startPublishing(mDigitTime);
  //getObjectsManager()->addCheck(mDigitAmplitude, "checkFromEMCAL", "o2::quality_control_modules::emcal::EMCALCheck",
  //                              "QcEMCAL");
}

void DigitsQcTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  mDigitAmplitude->Reset();
  mDigitTime->Reset();
}

void DigitsQcTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DigitsQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  QcInfoLogger::GetInstance() << "Start monitor data" << AliceO2::InfoLogger::InfoLogger::endm;

  // check if we have payoad
  auto dataref = ctx.inputs().get("emcal-digits");
  auto const* emcheader = o2::framework::DataRefUtils::getHeader<o2::emcal::EMCALBlockHeader*>(dataref);
  if (!emcheader->mHasPayload) {
    QcInfoLogger::GetInstance() << "No more digits" << AliceO2::InfoLogger::InfoLogger::endm;
    //ctx.services().get<o2::framework::ControlService>().readyToQuit(false);
    return;
  }
  // Get payload and loop over digits
  auto digitcontainer = ctx.inputs().get<std::vector<o2::emcal::Digit>>("emcal-digits");
  QcInfoLogger::GetInstance() << "Received " << digitcontainer.size() << " digits " << AliceO2::InfoLogger::InfoLogger::endm;
  for (auto digit : digitcontainer) {
    mDigitAmplitude->Fill(digit.getEnergy(), digit.getTower());
    mDigitTime->Fill(digit.getTimeStamp(), digit.getTower());
  }
}

void DigitsQcTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DigitsQcTask::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DigitsQcTask::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  mDigitAmplitude->Reset();
  mDigitTime->Reset();
}

} // namespace emcal
} // namespace quality_control_modules
} // namespace o2
