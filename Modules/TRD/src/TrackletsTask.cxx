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
/// \file   TrackletsTask.cxx
///

// Includes ROOT libraries for data visualization and histogramming
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <THashList.h>
#include <TLine.h>
#include <TMath.h>
#include <sstream>
#include <string>

// O² framework headers
#include "QualityControl/QcInfoLogger.h" // Handles logging messages
#include "Common/Utils.h"                // Utility functions for QC
#include "TRD/TrackletsTask.h"           // Header file defining the TrackletsTask class
#include "TRD/TRDHelpers.h"              // Helper functions for TRD
#include "TRDQC/StatusHelper.h"          // Utility for checking detector status

// Framework headers for processing data
#include <Framework/InputRecord.h> // Handles incoming data
#include <Framework/InputRecordWalker.h>
#include "DataFormatsTRD/Tracklet64.h"       // Defines TRD tracklet data format
#include "DataFormatsTRD/HelperMethods.h"    // Utility methods for handling TRD data
#include "DataFormatsTRD/Digit.h"            // TRD raw digits (individual hits)
#include "DataFormatsTRD/NoiseCalibration.h" // Noise suppression methods
#include "DataFormatsTRD/TriggerRecord.h"    // TRD trigger data structure

// Namespace Definitions
using namespace o2::quality_control_modules::common;
using namespace o2::trd::constants;
using Helper = o2::trd::HelperMethods;

// TrackletsTask Class Definition
// Defines the TrackletsTask class inside the o2::quality_control_modules::trd namespace.
// This ensures it doesn’t conflict with other modules
namespace o2::quality_control_modules::trd
{
  void TrackletsTask::initialize(o2::framework::InitContext & /*ctx*/)
  {
    ILOG(Debug, Devel) << "initialize TrackletsTask" << ENDM;
    mRemoveNoise = getFromConfig<bool>(mCustomParameters, "removeNoise", false);
    buildHistograms();
  }

  void TrackletsTask::buildHistograms()
  {
    constexpr int nLogBins = 100;
    float xBins[nLogBins + 1];
    float xBinLogMin = 0.f;
    float xBinLogMax = 8.f;
    float logBinWidth = (xBinLogMax - xBinLogMin) / nLogBins;

    for (int iBin = 0; iBin <= nLogBins; ++iBin)
    {
      xBins[iBin] = TMath::Power(10, xBinLogMin + iBin * logBinWidth);
    }

    mTrackletSlope.reset(new TH1F("trackletslope", "Tracklet inclination in natural units;pads per time bin;counts", 100, -0.15, 0.15));
    getObjectsManager()->startPublishing(mTrackletSlope.get()); // Registers it for automatic publishing

    mTrackletHCID.reset(new TH1F("tracklethcid", "Tracklet distribution over Halfchambers;HalfChamber ID;counts", 1080, -0.5, 1079.5));
    getObjectsManager()->startPublishing(mTrackletHCID.get());

    mTrackletPosition.reset(new TH1F("trackletpos", "Tracklet position relative to MCM center;number of pads;counts", 200, -30, 30));
    getObjectsManager()->startPublishing(mTrackletPosition.get());

    mTrackletsPerEvent.reset(new TH1F("trackletsperevent", "Number of Tracklets per event;Tracklets in Event;Counts", nLogBins, xBins));
    getObjectsManager()->startPublishing(mTrackletsPerEvent.get());
    getObjectsManager()->setDefaultDrawOptions(mTrackletsPerEvent->GetName(), "logx");

    mTrackletsPerEventPP.reset(new TH1F("trackletspereventPP", "Number of Tracklets per event;Tracklets in Event;Counts", 1000, 0, 5000));
    getObjectsManager()->startPublishing(mTrackletsPerEventPP.get());

    mTrackletsPerEventPbPb.reset(new TH1F("trackletspereventPbPb", "Number of Tracklets per event;Tracklets in Event;Counts", 1000, 0, 100000));
    getObjectsManager()->startPublishing(mTrackletsPerEventPbPb.get());

    // 2D map of tracklets across detector sectors;
    // The 2D histogram mTrackletsPerHC2D is used to monitor tracklet distribution across these half-chambers
    mTrackletsPerHC2D.reset(new TH2F("trackletsperHC2D", "Tracklets distribution in half-chambers;Sector_Side;Stack_Side", 36, 0, 36, 30, 0, 30));
    mTrackletsPerHC2D->SetStats(0);
    mTrackletsPerHC2D->GetXaxis()->SetTitle("Sector_Side");
    mTrackletsPerHC2D->GetXaxis()->CenterTitle(kTRUE);
    mTrackletsPerHC2D->GetYaxis()->SetTitle("Stack_Layer");
    mTrackletsPerHC2D->GetYaxis()->CenterTitle(kTRUE);

    for (int s = 0; s < NSTACK; ++s) // 5 stacks
    {
      for (int l = 0; l < NLAYER; ++l) // 6 layers
      {
        std::string label = fmt::format("{0}_{1}", s, l);
        int pos = s * NLAYER + l + 1;
        mTrackletsPerHC2D->GetYaxis()->SetBinLabel(pos, label.c_str());
      }
    }

    //  labeling the axes of the 2D histogram mTrackletsPerHC2D, which tracks the number of tracklets per half-chamber in the TRD detector

    for (int sm = 0; sm < NSECTOR; ++sm) // 18 sectors
    {
      for (int side = 0; side < 2; ++side) // A side and C side
      {
        std::string label = fmt::format("{0}_{1}", sm, side == 0 ? "A" : "B");
        int pos = sm * 2 + side + 1;
        mTrackletsPerHC2D->GetXaxis()->SetBinLabel(pos, label.c_str());
      }
    }

    getObjectsManager()->startPublishing(mTrackletsPerHC2D.get());
    getObjectsManager()->setDefaultDrawOptions(mTrackletsPerHC2D->GetName(), "COLZ");
    getObjectsManager()->setDisplayHint(mTrackletsPerHC2D->GetName(), "logz");

    for (int sm = 0; sm < NSECTOR; ++sm)
    {
      for (int chargeWindow = 0; chargeWindow < 3; ++chargeWindow)
      {
        mTrackletQSupermodule[sm][chargeWindow] = std::make_unique<TH1F>(
            Form("TrackletQ_SM%i_Q%i", sm, chargeWindow),
            Form("Tracklet Q%i for Supermodule %i;charge (a.u.);counts", chargeWindow, sm),
            256, -0.5, 255.5);
        getObjectsManager()->startPublishing(mTrackletQSupermodule[sm][chargeWindow].get());
        getObjectsManager()->setDefaultDrawOptions(mTrackletQSupermodule[sm][chargeWindow]->GetName(), "logy");
      }
    }

    mTrackletsPerTimeFrame.reset(new TH1F("trackletspertimeframe", "Number of Tracklets per timeframe;Tracklets in TimeFrame;Counts", nLogBins, xBins));
    getObjectsManager()->startPublishing(mTrackletsPerTimeFrame.get());
    getObjectsManager()->setDefaultDrawOptions(mTrackletsPerTimeFrame->GetName(), "logx");
    // A tracklet is a short segment of a charged particle's trajectory, reconstructed using hits in the TRD detector = a small piece of a full particle track, typically reconstructed at the chamber level
    // The number of tracklets per timeframe refers to how many tracklets were recorded and reconstructed within a given time interval

    mTriggersPerTimeFrame.reset(new TH1F("triggerspertimeframe", "Number of Triggers per timeframe;Triggers in TimeFrame;Counts", 1000, 0, 1000));
    getObjectsManager()->startPublishing(mTriggersPerTimeFrame.get());
    // trigger is a system that decides whether to record an event or discard it
    // The number of triggers per timeframe represents how often the trigger system has accepted an event for further processing within a given timeframe

    // Build tracklet layers
    int unitsPerSection = NCOLUMN / NSECTOR;
    for (int iLayer = 0; iLayer < NLAYER; ++iLayer)
    {
      mLayers[iLayer].reset(new TH2F(Form("TrackletsPerMCM_Layer%i", iLayer), Form("Tracklet count per MCM in layer %i;glb pad row;glb MCM col", iLayer),
                                     76, -0.5, 75.5, NCOLUMN, -0.5, NCOLUMN - 0.5));
      mLayers[iLayer]->SetStats(0);
      TRDHelpers::addChamberGridToHistogram(mLayers[iLayer], unitsPerSection);
      getObjectsManager()->startPublishing(mLayers[iLayer].get());
      getObjectsManager()->setDefaultDrawOptions(mLayers[iLayer]->GetName(), "COLZ");
      getObjectsManager()->setDisplayHint(mLayers[iLayer].get(), "logz");
    }
  }

  void TrackletsTask::monitorData(o2::framework::ProcessingContext &ctx) // This function, monitorData, is responsible for processing TRD tracklet data and filling histograms for monitoring purposes in the ALICE O² framework
  {
    // Load CCDB objects (needs to be done only once) i.e. This loads the noise map from the CCDB
    if (!mNoiseMap)
    {
      auto ptr = ctx.inputs().get<o2::trd::NoiseStatusMCM *>("noiseMap");
      mNoiseMap = ptr.get();
    }

    auto ptr = ctx.inputs().get<std::array<int, MAXCHAMBER> *>("fedChamberStatus");
    auto tracklets = ctx.inputs().get<gsl::span<o2::trd::Tracklet64>>("tracklets");

    for (const auto &trklt : tracklets)
    {
      if (mNoiseMap != nullptr && mRemoveNoise && mNoiseMap->isTrackletFromNoisyMCM(trklt))
      {
        continue;
      }

      int hcid = trklt.getHCID();
      int supermodule = Helper::getSector(hcid / 2);
      if (supermodule < 0 || supermodule >= 18)
      {
        LOG(ERROR) << "Invalid supermodule sector: " << supermodule;
        continue;
      }

      // Ensure histograms are initialized
      if (!mTrackletQSupermodule[supermodule][0])
      {
        mTrackletQSupermodule[supermodule][0] = new TH1F(Form("Q0_Sector_%d", supermodule), "Q0 distribution", 100, 0, 1000);
        mTrackletQSupermodule[supermodule][1] = new TH1F(Form("Q1_Sector_%d", supermodule), "Q1 distribution", 100, 0, 1000);
        mTrackletQSupermodule[supermodule][2] = new TH1F(Form("Q2_Sector_%d", supermodule), "Q2 distribution", 100, 0, 1000);
      }

      mTrackletQSupermodule[supermodule][0]->Fill(trklt.getQ0());
      mTrackletQSupermodule[supermodule][1]->Fill(trklt.getQ1());
      mTrackletQSupermodule[supermodule][2]->Fill(trklt.getQ2());
    }
    auto triggerrecords = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");

    mTrackletsPerTimeFrame->Fill(tracklets.size());     // Number of tracklets in the current timeframe
    mTriggersPerTimeFrame->Fill(triggerrecords.size()); // Number of triggers in the current timeframe

    for (const auto &trigger : triggerrecords) // Loop Over Each Trigger Record
    {
      mTrackletsPerEvent->Fill(trigger.getNumberOfTracklets());     // General tracklets per event
      mTrackletsPerEventPP->Fill(trigger.getNumberOfTracklets());   // Tracklets per proton-proton event
      mTrackletsPerEventPbPb->Fill(trigger.getNumberOfTracklets()); // Tracklets per heavy-ion (Pb-Pb) event

      // Loops through all tracklets belonging to the current trigger
      for (int currenttracklet = trigger.getFirstTracklet(); currenttracklet < trigger.getFirstTracklet() + trigger.getNumberOfTracklets(); ++currenttracklet)
      {
        const auto &trklt = tracklets[currenttracklet]; // Fetches the tracklet data from the array

        if (mNoiseMap != nullptr && mRemoveNoise && mNoiseMap->isTrackletFromNoisyMCM(trklt))
        {
          continue;
        } // Remove Noisy (noisy MCM originated) Tracklets

        // Extract Tracklet Geometric Information
        int hcid = trklt.getHCID();               // Hardware Chamber ID (HCID) of the tracklet
        int layer = Helper::getLayer(hcid / 2);   // TRD layer the tracklet was found in
        int stack = Helper::getStack(hcid / 2);   // Stack index of the tracklet
        int sector = Helper::getSector(hcid / 2); // Detector sector where the tracklet was detected
        int stackLayer = stack * NLAYER + layer;  // Converts stack and layer into a single index for plotting
        int sectorSide = sector * 2 + (hcid % 2); // Differentiates the two sides of a sector

        // Converts the local detector coordinates into global detector coordinates
        int rowGlb = FIRSTROW[stack] + trklt.getPadRow();
        int colGlb = trklt.getMCMCol() + sector * 8;

        mTrackletsPerHC2D->Fill(sectorSide, stackLayer);
        mTrackletSlope->Fill(trklt.getSlopeFloat());
        mTrackletPosition->Fill(trklt.getPositionFloat());
        mTrackletHCID->Fill(hcid);
        // Fills histograms for tracklet charge deposits in 3 different charge windows
        // mTrackletQ[0]->Fill(trklt.getQ0());
        // mTrackletQ[1]->Fill(trklt.getQ1());
        // mTrackletQ[2]->Fill(trklt.getQ2());
        mLayers[layer]->Fill(rowGlb, colGlb);
      }
    }
  }

  void TrackletsTask::startOfActivity(const Activity &activity) // when a new activity (data-taking session) starts
  {
    ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  }

  void TrackletsTask::startOfCycle() // Called at the start of each processing cycle
  {
    ILOG(Debug, Devel) << "startOfCycle" << ENDM;
  }

  void TrackletsTask::endOfCycle() // Called at the end of a processing cycle
  {
    ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  }

  void TrackletsTask::endOfActivity(const Activity & /*activity*/) // Called at the end of an activity
  {
    ILOG(Debug, Devel) << "endOfActivity" << ENDM;
  }

  void TrackletsTask::finaliseCCDB(o2::framework::ConcreteDataMatcher &matcher, void *obj)
  {
    if (matcher == o2::framework::ConcreteDataMatcher("TRD", "FCHSTATUS", 0))
    {
      TRDHelpers::drawChamberStatusOnHistograms(static_cast<std::array<int, MAXCHAMBER> *>(obj), mTrackletsPerHC2D, mLayers, NCOLUMN / NSECTOR);
    }
  }

  void TrackletsTask::reset()
  {
    // clean all the monitor objects here
    ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
    for (auto &supermoduleHistos : mTrackletQSupermodule)
    {
      for (auto &histo : supermoduleHistos)
      {
        if (histo)
        {
          histo->Reset();
        }
      }
    }
    mTrackletSlope->Reset();
    mTrackletHCID->Reset();
    mTrackletPosition->Reset();
    mTrackletsPerEvent->Reset();
    mTrackletsPerEventPP->Reset();
    mTrackletsPerEventPbPb->Reset();
    mTrackletsPerHC2D->Reset();
    mTrackletsPerTimeFrame->Reset();
    mTriggersPerTimeFrame->Reset();
    for (auto h : mLayers)
    {
      h->Reset();
    }
    for (auto h : mTrackletQ)
    {
      h->Reset();
    }
  }

} // namespace o2::quality_control_modules::trd

// Histograms Being Reset

// mTrackletSlope → Tracklet slope distribution.

// mTrackletHCID → Histogram of tracklet Hardware Chamber IDs.

// mTrackletPosition → Position of tracklets.

// mTrackletsPerEvent → Tracklets per event.

// mTrackletsPerEventPP → Tracklets per proton-proton event.

// mTrackletsPerEventPbPb → Tracklets per heavy-ion event.

// mTrackletsPerHC2D → Tracklets per sector and stack-layer.

// mTrackletsPerTimeFrame → Tracklets per timeframe.

// mTriggersPerTimeFrame → Number of triggers per timeframe.

// Loop resets histograms for:

// mLayers → Stores tracklet positions per layer.

// mTrackletQ → Tracklet charge Q0, Q1, Q2.
