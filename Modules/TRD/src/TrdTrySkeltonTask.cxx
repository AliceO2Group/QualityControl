#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "QualityControl/QcInfoLogger.h"
#include "TRD/TrdTrySkeltonTask.h"
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>

#include "DataFormatsTRD/Tracklet64.h"
#include "QualityControl/MonitorObject.h"
#include "DataFormatsTRD/TriggerRecord.h"
#include "DataFormatsTRD/HelperMethods.h"

// #include "DataFormatsTRD/TrackTRD.h"

namespace o2::quality_control_modules::trd
{

TrdTrySkeltonTask::~TrdTrySkeltonTask()
{
}

void TrdTrySkeltonTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  // This is how logs are created. QcInfoLogger is used. In production, FairMQ logs will go to InfoLogger as well.
  ILOG(Debug, Devel) << "initialize TrdTrySkeltonTask" << ENDM;
  ILOG(Debug, Support) << "A debug targeted for support" << ENDM;
  ILOG(Info, Ops) << "An Info log targeted for operators" << ENDM;

  ILOG(Info, Ops) << "Initializing TRD skelton histograms" << ENDM;
  // This creates and registers a histogram for publication at the end of each cycle, until the end of the task lifetime

  // histTrackletsTF = std::make_unique<TH1F>("nTrackletsTF", "Number of TRD Tracklets per Timeframe", 2000, 0, 200000); // pp
  histTrackletsTF = std::make_unique<TH1F>("nTrackletsTF", "Number of TRD Tracklets per Timeframe", 10000, 0, 600000); // Pb-Pb

  histTrackletsEvent = std::make_unique<TH1F>("nTrackletsEVENT", "Number of TRD Tracklets per Event", 10000, 0, 10000);

  histQ0 = std::make_unique<TH1F>("Q0", "Q0 per TRD Tracklet", 256, -0.5, 255.5);
  histQ1 = std::make_unique<TH1F>("Q1", "Q1 per TRD Tracklet", 256, -0.5, 255.5);
  histQ2 = std::make_unique<TH1F>("Q2", "Q2 per TRD Tracklet", 256, -0.5, 255.5);

  histChamber = std::make_unique<TH1F>("Chamber", "Tracklets per TRD Chamber", 541, 0, 541);
  histPadRow = std::make_unique<TH1F>("PadRow", "Tracklets per PadRow", 17, 0, 17);
  histPadRowVsDet = std::make_unique<TH2F>("PadRowVsDet", "PadRow vs Detector;Detector ID;PadRow", 541, 0, 541, 17, 0, 17); // 540 chambers and 224 MCM each

  // 540 chambers × 16 MCMs each = 8640 total MCMs
  histMCM = std::make_unique<TH1F>("MCM",
                                   "Tracklets per global MCM;Global MCM ID;Entries",
                                   1400000, 0, 1400000); // 120960

  histMCMOccupancy = std::make_unique<TH1F>("MCMTrackletPerMCM",
                                            "Number of tracklets per MCM;Tracklets per MCM;Count of MCMs",
                                            5, 0, 5);

  getObjectsManager()->startPublishing(histPadRowVsDet.get(), PublicationPolicy::Forever);
  getObjectsManager()->startPublishing(histMCMOccupancy.get(), PublicationPolicy::Forever);
  getObjectsManager()->startPublishing(histTrackletsTF.get(), PublicationPolicy::Forever);
  getObjectsManager()->startPublishing(histTrackletsEvent.get(), PublicationPolicy::Forever);
  getObjectsManager()->startPublishing(histQ0.get(), PublicationPolicy::Forever);
  getObjectsManager()->startPublishing(histQ1.get(), PublicationPolicy::Forever);
  getObjectsManager()->startPublishing(histQ2.get(), PublicationPolicy::Forever);
  getObjectsManager()->startPublishing(histChamber.get(), PublicationPolicy::Forever);
  getObjectsManager()->startPublishing(histPadRow.get(), PublicationPolicy::Forever);
  getObjectsManager()->startPublishing(histMCM.get(), PublicationPolicy::Forever);

  try {
    getObjectsManager()->addMetadata(histTrackletsTF->GetName(), "custom", "34");
  } catch (...) {
    ILOG(Warning, Support) << "Metadata could not be added to " << histTrackletsTF->GetName() << ENDM;
  }
}

void TrdTrySkeltonTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;

  // We clean anyhists that could have been filled in previous runs.
  histTrackletsTF->Reset();
  histTrackletsEvent->Reset();
  histQ0->Reset();
  histQ1->Reset();
  histQ2->Reset();
  histChamber->Reset();
  histPadRow->Reset();
  histMCM->Reset();
  histPadRowVsDet->Reset();
  histMCMOccupancy->Reset();
}

void TrdTrySkeltonTask::startOfCycle()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void TrdTrySkeltonTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // Get TRD tracklets
  auto tracklets = ctx.inputs().get<gsl::span<o2::trd::Tracklet64>>("tracklets");
  auto trigRec = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");

  // 1. Tracklets per timeframe (simple count)
  int nTF = tracklets.size();
  histTrackletsTF->Fill(nTF);

  // 2. Tracklets per event
  for (auto& tr : trigRec) {
    int start = tr.getFirstTracklet();
    int n = tr.getNumberOfTracklets();
    histTrackletsEvent->Fill(n);
  }

  // 3. Loop over tracklets
  for (const auto& trk : tracklets) {
    // Filling Q values for tracklet
    histQ0->Fill(trk.getQ0());
    histQ1->Fill(trk.getQ1());
    histQ2->Fill(trk.getQ2());

    histChamber->Fill(trk.getDetector());                      // ChamberIDs (0-539)
    histPadRow->Fill(trk.getPadRow());                         // PadRow (0-15)
    histPadRowVsDet->Fill(trk.getDetector(), trk.getPadRow()); // Fill PadRow vs Detector

    int det = trk.getDetector(); // 0–539
    int mcm = trk.getMCM();      // 0–15
    int hcid = trk.getHCID();    // 0–1079
    int rob = trk.getROB();      // 0..5 (C0) or 0..7 (C1)
  }
  // ---------------------------------------------------------------------
  //   Tracklets per MCM (Sean's method: streaming unique key)
  // ---------------------------------------------------------------------

  int lastKey = -1;
  int trackletCount = 0;

  for (const auto& trk : tracklets) {

    int det = trk.getDetector();
    int rob = trk.getROB();
    int mcm = trk.getMCM();

    // Build TEMPORARY unique key (channel = 0 collapses channels)
    int key = o2::trd::HelperMethods::getGlobalChannelIndex(det, rob, mcm, 0);

    if (lastKey == -1) {
      lastKey = key;
    }

    if (key != lastKey) {

      histMCM->SetBinContent(lastKey + 1, trackletCount);
      histMCMOccupancy->Fill(trackletCount);

      if (trackletCount > 3) {
        ILOG(Warning, Ops)
          << "High MCM occupancy detected: "
          << "globalKey=" << lastKey
          << " tracklets=" << trackletCount
          << ENDM;
      }

      trackletCount = 0;
      lastKey = key;
    }

    trackletCount++; // counting the tracklets belonging to one key, implies with one mcm
  }

  // Flush last MCM
  if (trackletCount > 0) {
    histMCM->SetBinContent(lastKey + 1, trackletCount);
    histMCMOccupancy->Fill(trackletCount);

    ILOG(Warning, Ops)
      << "last globalKey=" << lastKey
      << " tracklets=" << trackletCount
      << ENDM;

    if (trackletCount > 3) {
      ILOG(Warning, Ops)
        << "High MCM occupancy detected (last): "
        << "globalKey=" << lastKey
        << " tracklets=" << trackletCount
        << ENDM;
    }
  }
}

void TrdTrySkeltonTask::endOfCycle()
{
  // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void TrdTrySkeltonTask::endOfActivity(const Activity& /*activity*/)
{
  // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void TrdTrySkeltonTask::reset()
{
  //  200,000 bins for timeframe is correct ?
  // Typical TF has ~50 events × 2k tracklets/event = 100k tracklets.

  //  Per-event histogram should go to around 5000
  // Highest tracklet multiplicity per event ≈ 2500–3000.

  // Clean all the monitor objects here.
  ILOG(Debug, Devel)
    << "Resetting thehists" << ENDM;
  // if (histTracklet)
  //   histTracklet->Reset();
  if (histTrackletsTF)
    histTrackletsTF->Reset();
  if (histTrackletsEvent)
    histTrackletsEvent->Reset();
  if (histQ0)
    histQ0->Reset();
  if (histQ1)
    histQ1->Reset();
  if (histQ2)
    histQ2->Reset();
  if (histChamber)
    histChamber->Reset();
  if (histPadRow)
    histPadRow->Reset();
  if (histMCM)
    histMCM->Reset();
  if (histPadRowVsDet)
    histPadRowVsDet->Reset();
  if (histMCMOccupancy)
    histMCMOccupancy->Reset();
}

} // namespace o2::quality_control_modules::trd
