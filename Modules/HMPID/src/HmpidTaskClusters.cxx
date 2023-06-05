// Copyright CERN and copyright holders of ALICE O2. This software is
// // distributed under the terms of the GNU General Public License v3 (GPL
// // Version 3), copied verbatim in the file "COPYING".
// //
// // See http://alice-o2.web.cern.ch/license for full licensing information.
// //
// // In applying this license CERN does not waive the privileges and immunities
// // granted to it by virtue of its status as an Intergovernmental Organization
// // or submit itself to any jurisdiction.
//

///
/// \file   TaskClusters.cxx
/// \author Annalisa Mastroserio
///

#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsHMP/Cluster.h"
#include "DataFormatsHMP/Trigger.h"
#include "HMPID/HmpidTaskClusters.h"

#include <sstream>
#include <TCanvas.h>
#include <DataFormatsParameters/GRPObject.h>

#include <Framework/InputRecord.h>

#ifdef WITH_OPENMP
#include <omp.h>
#endif

// using namespace o2::hmpid;

namespace o2::quality_control_modules::hmpid
{

HmpidTaskClusters::HmpidTaskClusters() : TaskInterface() {}

HmpidTaskClusters::~HmpidTaskClusters()
{
  ILOG(Info, Support) << "destructor called" << ENDM;
  delete hClusMultEv;
  delete ThClusMult;
  for (Int_t iCh = 0; iCh < 7; iCh++) {
    delete hHMPIDchargeClus[iCh];
    delete hHMPIDpositionClus[iCh];
  }
}

void HmpidTaskClusters::initialize(o2::framework::InitContext& /*ctx*/)
{

  ILOG(Debug, Devel) << "initialize TaskClusters" << ENDM;

  // getJsonParameters();

  BookHistograms();

  // publish histograms (Q: why publishing in initalize?)
  getObjectsManager()->startPublishing(hClusMultEv);
  getObjectsManager()->startPublishing(ThClusMult);
  for (Int_t iCh = 0; iCh < 7; iCh++) {
    getObjectsManager()->startPublishing(hHMPIDchargeClus[iCh]);
    getObjectsManager()->startPublishing(hHMPIDpositionClus[iCh]);
  }

  ILOG(Info, Support) << "START DOING QC HMPID Cluster" << ENDM;

  // here do the QC
}

void HmpidTaskClusters::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  reset();
}

void HmpidTaskClusters::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void HmpidTaskClusters::monitorData(o2::framework::ProcessingContext& ctx)
{
  ILOG(Info, Support) << "monitorData" << ENDM;

  // Get HMPID triggers
  const auto triggers = ctx.inputs().get<std::vector<o2::hmpid::Trigger>>("intrecord");

  // Get HMPID clusters
  auto clusters = ctx.inputs().get<std::vector<o2::hmpid::Cluster>>("clusters");

  int nEvents = triggers.size();

  for (int i = 0; i < nEvents; i++) { // events loop

    double nClusters = triggers[i].getLastEntry() - triggers[i].getFirstEntry();
    hClusMultEv->Fill(nClusters);

    Int_t nClusCh[7] = { 0, 0, 0, 0, 0, 0, 0 };

    for (int j = triggers[i].getFirstEntry(); j <= triggers[i].getLastEntry(); j++) { // cluster loop on the same event

      int chamber = clusters[j].ch();
      nClusCh[chamber]++;

      if (chamber <= 6 && chamber >= 0) {
        hHMPIDchargeClus[chamber]->Fill(clusters[j].q());
        hHMPIDpositionClus[chamber]->Fill(clusters[j].x(), clusters[j].y());
      }
    } // cluster loop
    for (Int_t ich = 0; ich < 7; ich++) {
      ThClusMult->Fill(ich, nClusCh[ich]);
    }
  } // events loop
}

void HmpidTaskClusters::endOfCycle()
{

  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void HmpidTaskClusters::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void HmpidTaskClusters::BookHistograms()
{
  hClusMultEv = new TH1F("ClusMultEve", "HMPID Cluster multiplicity per event", 500, 0, 500);

  ThClusMult = new TProfile("ClusMult", "HMPID Cluster multiplicity per chamber;Chamber Id;# of clusters", 16, -1, 7, 0, 500);
  ThClusMult->Sumw2();
  ThClusMult->SetOption("P");
  ThClusMult->SetMinimum(0);
  ThClusMult->SetMarkerStyle(20);
  ThClusMult->SetMarkerColor(kBlack);
  ThClusMult->SetLineColor(kBlack);

  for (Int_t iCh = 0; iCh < 7; iCh++) {
    hHMPIDchargeClus[iCh] = new TH1F(Form("CluQ%i", iCh), Form("Cluster Q in HMPID Chamber %i ; Q (ADC)", iCh), 50, 0, 50);
    hHMPIDpositionClus[iCh] = new TH2F(Form("ClusterPoistion%i", iCh), Form("Cluster Position in HMPID Chamber %i; X (cm); Y (cm)", iCh), 532, -1, 131, 500, -1, 124); // cmxcm
  }
  //
}

void HmpidTaskClusters::reset()
{
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;

  hClusMultEv->Reset();
  ThClusMult->Reset();

  for (Int_t iCh = 0; iCh < 7; iCh++) {
    hHMPIDchargeClus[iCh]->Reset();
    hHMPIDpositionClus[iCh]->Reset();
  }
}

void HmpidTaskClusters::getJsonParameters()
{
  ILOG(Info, Support) << "GetJsonParams" << ENDM;
}

} // namespace o2::quality_control_modules::hmpid
