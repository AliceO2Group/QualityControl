///
/// \file   DigitsQcTask.cxx
/// \author Markus Fasel, Cristina Terrevoli
///

#include <TCanvas.h>
#include <TH2.h>

#include <DataFormatsEMCAL/EMCALBlockHeader.h>
#include <DataFormatsEMCAL/Digit.h>
#include "QualityControl/QcInfoLogger.h"
#include "EMCAL/DigitsQcTask.h"
#include "EMCALBase/Geometry.h"

namespace o2
{
namespace quality_control_modules
{
namespace emcal
{

DigitsQcTask::~DigitsQcTask()
{
  for (auto h : mDigitAmplitude) {
    delete h;
  }
  for (auto h : mDigitTime) {
    delete h;
    if (mDigitTime[0]) {
      delete mDigitTime[0];
    }
    if (mDigitTime[1]) {
      delete mDigitTime[1];
    }
  }
  if (mDigitAmplitudeEMCAL)
    delete mDigitAmplitudeEMCAL;
  if (mDigitAmplitudeDCAL)
    delete mDigitAmplitudeDCAL;
}

void DigitsQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize DigitsQcTask" << AliceO2::InfoLogger::InfoLogger::endm;
  //define histograms
  mDigitAmplitude[0] = new TH2F("digitAmplitudeHG", "Digit Amplitude (High gain)", 100, 0, 100, 20000, 0., 20000.);
  mDigitAmplitude[1] = new TH2F("digitAmplitudeLG", "Digit Amplitude (Low gain)", 100, 0, 100, 20000, 0., 20000.);
  mDigitTime[0] = new TH2F("digitTimeHG", "Digit Time (High gain)", 1000, 0, 1000, 20000, 0., 20000.);
  mDigitTime[1] = new TH2F("digitTimeLG", "Digit Time (Low gain)", 1000, 0, 1000, 20000, 0., 20000.);
  // 1D histograms for showing the integrated spectrum
  mDigitAmplitudeEMCAL = new TH1F("digitAmplitudeEMCAL", "Digit amplitude in EMCAL", 100, 0., 100.);
  mDigitAmplitudeDCAL = new TH1F("digitAmplitudeDCAL", "Digit amplitude in DCAL", 100, 0., 100.);
  //Puglishing histograms
  for (auto h : mDigitAmplitude)
    getObjectsManager()->startPublishing(h);
  for (auto h : mDigitTime)
    getObjectsManager()->startPublishing(h);

  getObjectsManager()->startPublishing(mDigitAmplitudeEMCAL);
  getObjectsManager()->startPublishing(mDigitAmplitudeDCAL);

  // initialize geometry
  if (!mGeometry)
    mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
}

void DigitsQcTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  reset();
}

void DigitsQcTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void DigitsQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  //  QcInfoLogger::GetInstance() << "Start monitor data" << AliceO2::InfoLogger::InfoLogger::endm;

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
  //  QcInfoLogger::GetInstance() << "Received " << digitcontainer.size() << " digits " << AliceO2::InfoLogger::InfoLogger::endm;
  for (auto digit : digitcontainer) {
    int index = digit.getHighGain() ? 0 : (digit.getLowGain() ? 1 : -1);
    if (index < 0)
      continue;
    mDigitAmplitude[index]->Fill(digit.getEnergy(), digit.getTower());
    mDigitTime[index]->Fill(digit.getTimeStamp(), digit.getTower());

    // get the supermodule for filling EMCAL/DCAL spectra
    try {
      auto cellindices = mGeometry->GetCellIndex(digit.getTower());
      if (std::get<0>(cellindices) < 12)
        mDigitAmplitudeEMCAL->Fill(digit.getEnergy());
      else
        mDigitAmplitudeDCAL->Fill(digit.getEnergy());
    } catch (o2::emcal::InvalidCellIDException& e) {
      QcInfoLogger::GetInstance() << "Invalid cell ID: " << e.getCellID() << AliceO2::InfoLogger::InfoLogger::endm;
    };
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
  for (auto h : mDigitAmplitude)
    h->Reset();
  for (auto h : mDigitTime)
    h->Reset();
  mDigitAmplitudeEMCAL->Reset();
  mDigitAmplitudeDCAL->Reset();
}

} // namespace emcal
} // namespace quality_control_modules
} // namespace o2
