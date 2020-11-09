///
/// \file   DigitsQcTask.cxx
/// \author Markus Fasel, Cristina Terrevoli
///

#include <TCanvas.h>
#include <TH2.h>
#include <TProfile2D.h>

#include <DataFormatsEMCAL/EMCALBlockHeader.h>
#include <DataFormatsEMCAL/TriggerRecord.h>
#include <DataFormatsEMCAL/Digit.h>
#include "QualityControl/QcInfoLogger.h"
#include "EMCAL/DigitsQcTask.h"
#include "DataFormatsEMCAL/Cell.h"
#include "EMCALBase/Geometry.h"
#include <Framework/InputRecord.h>

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

  mDigitOccupancy = new TH2F("digitOccupancyEMC", "Digit Occupancy EMCAL", 96, -0.5, 95.5, 208, -0.5, 207.5);
  mDigitOccupancyThr = new TH2F("digitOccupancyEMCwThr", "Digit Occupancy EMCAL with E>0.5 GeV/c", 96, -0.5, 95.5, 208, -0.5, 207.5);

  mIntegratedOccupancy = new TProfile2D("digitOccupancyInt", "Digit Occupancy Integrated", 96, -0.5, 95.5, 208, -0.5, 207.5);
  mIntegratedOccupancy->GetXaxis()->SetTitle("col");
  mIntegratedOccupancy->GetYaxis()->SetTitle("row");

  mDigitTime[0] = new TH2F("digitTimeHG", "Digit Time (High gain)", 1000, 0, 1000, 20000, 0., 20000.);
  mDigitTime[1] = new TH2F("digitTimeLG", "Digit Time (Low gain)", 1000, 0, 1000, 20000, 0., 20000.);
  // 1D histograms for showing the integrated spectrum

  mDigitAmplitudeEMCAL = new TH1F("digitAmplitudeEMCAL", "Digit amplitude in EMCAL", 100, 0., 100.);
  mDigitAmplitudeDCAL = new TH1F("digitAmplitudeDCAL", "Digit amplitude in DCAL", 100, 0., 100.);
  mnumberEvents = new TH1F("NumberOfEvents", "Number Of Events", 1, 0.5, 1.5);

  //Puglishing histograms
  for (auto h : mDigitAmplitude)
    getObjectsManager()->startPublishing(h);
  for (auto h : mDigitTime)
    getObjectsManager()->startPublishing(h);
  getObjectsManager()->startPublishing(mDigitAmplitudeEMCAL);
  getObjectsManager()->startPublishing(mDigitAmplitudeDCAL);
  getObjectsManager()->startPublishing(mDigitOccupancy);
  getObjectsManager()->startPublishing(mDigitOccupancyThr);
  getObjectsManager()->startPublishing(mIntegratedOccupancy);
  getObjectsManager()->startPublishing(mnumberEvents);

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
  auto digitcontainer = ctx.inputs().get<gsl::span<o2::emcal::Cell>>("emcal-digits"); //it was emcal::Digit
  auto triggerrecords = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>("emcal-triggerecords");

  //  QcInfoLogger::GetInstance() << "Received " << digitcontainer.size() << " digits " << AliceO2::InfoLogger::InfoLogger::endm;
  int eventcounter = 0;
  for (auto trg : triggerrecords) {
    if (!trg.getNumberOfObjects())
      continue;
    QcInfoLogger::GetInstance() << QcInfoLogger::Debug << "Next event " << eventcounter << " has " << trg.getNumberOfObjects() << " digits" << QcInfoLogger::endm;
    //gsl::span<const o2::emcal::Digit> eventdigits(digitcontainer.data() + trg.getFirstEntry(), trg.getNumberOfObjects());
    gsl::span<const o2::emcal::Cell> eventdigits(digitcontainer.data() + trg.getFirstEntry(), trg.getNumberOfObjects());
    for (auto digit : eventdigits) {
      int index = digit.getHighGain() ? 0 : (digit.getLowGain() ? 1 : -1);
      if (index < 0)
        continue;

      mDigitAmplitude[index]->Fill(digit.getEnergy(), digit.getTower());
      mDigitTime[index]->Fill(digit.getTimeStamp(), digit.getTower());

      // get the supermodule for filling EMCAL/DCAL spectra

      try {

        auto [row, col] = mGeometry->GlobalRowColFromIndex(digit.getTower());
        if (digit.getEnergy() > 0) {
          mDigitOccupancy->Fill(col, row);
        }
        if (digit.getEnergy() > mCellThreshold) {
          mDigitOccupancyThr->Fill(col, row);
        }
        mIntegratedOccupancy->Fill(col, row, digit.getEnergy());

        auto cellindices = mGeometry->GetCellIndex(digit.getTower());
        if (std::get<0>(cellindices) < 12)
          mDigitAmplitudeEMCAL->Fill(digit.getEnergy());

        else
          mDigitAmplitudeDCAL->Fill(digit.getEnergy());
      } catch (o2::emcal::InvalidCellIDException& e) {
        QcInfoLogger::GetInstance() << "Invalid cell ID: " << e.getCellID() << AliceO2::InfoLogger::InfoLogger::endm;
      };
    }
    eventcounter++;
    mnumberEvents->Fill(1);
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
  mDigitOccupancy->Reset();
  mDigitOccupancyThr->Reset();
  mIntegratedOccupancy->Reset();
}
} // namespace emcal
} // namespace quality_control_modules
} // namespace o2
