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
#include "EMCAL/FECRateVisualization.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/QcInfoLogger.h"

#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>
#include <TLegend.h>
#include <TTree.h>
#include <TLatex.h>

#include <boost/property_tree/ptree.hpp>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::emcal
{

FECRateVisualization::~FECRateVisualization()
{
}

void FECRateVisualization::configure(const boost::property_tree::ptree& config)
{
  auto taskConfig = config.get_child_optional("qc.postprocessing." + getID() + ".configuration");
  if (taskConfig) {
    auto cfgMaxRate = taskConfig.get().get_child_optional("MaxRate");
    if (cfgMaxRate) {
      mMaxRate = cfgMaxRate->get_value<double>();
      ILOG(Info, Support) << "Found max FEC rate: " << mMaxRate << ENDM;
    }
  }
}

void FECRateVisualization::initialize(Trigger, framework::ServiceRegistryRef)
{
  mIndicesConverter.Initialize();
  for (int ism = 0; ism < 20; ism++) {
    mSupermoduleCanvas[ism] = std::make_unique<TCanvas>(Form("FECRatesSM%d", ism), Form("FEC rates for supermodule %d", ism), 800, 600);
    getObjectsManager()->startPublishing(mSupermoduleCanvas[ism].get(), PublicationPolicy::Forever);
  }
}

void FECRateVisualization::update(Trigger t, framework::ServiceRegistryRef services)
{
  std::cout << "Using max rate: " << mMaxRate << std::endl;
  std::array<Color_t, 10> colors = { { kRed, kGreen, kBlue, kOrange, kTeal, kMagenta, kCyan, kViolet, kGray, kBlack } };
  std::array<Style_t, 10> markers = { { 20, 21, 22, 24, 25, 26, 27, 28, 29, 30 } };

  const std::array<int, 12> fecs_small_A = { { 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13 } },
                            fecs_small_C = { { 27, 28, 29, 31, 32, 33, 34, 39, 36, 37, 38, 39 } };
  const std::array<int, 24> fecs_DCAL = { { 1, 2, 3, 4, 5, 6, 7, 8, 14, 15, 16, 17, 18, 19, 21, 22, 27, 28, 29, 31, 32, 33, 34, 35 } };
  enum SMType {
    FULL,
    SMALL_A,
    SMALL_C,
    DCAL
  };
  const std::array<SMType, 20> smtypes{ { FULL, FULL, FULL, FULL, FULL, FULL, FULL, FULL, FULL, FULL, SMALL_A, SMALL_C, DCAL, DCAL, DCAL, DCAL, DCAL, DCAL, SMALL_C, SMALL_A } };

  auto& qcdb = services.get<quality_control::repository::DatabaseInterface>();
  auto mo = qcdb.retrieveMO("EMC/MO/CellTrendingDetailEMCAL", "CellTrendingDetailEMCAL", t.timestamp, t.activity);
  TTree* datatree = mo ? static_cast<TTree*>(mo->getObject()) : nullptr;
  if (!datatree) {
    ILOG(Error, Support) << "Tree not found" << ENDM;
    return;
  }

  for (int smID = 0; smID < 20; smID++) {
    int minfec = smID * 40;
    mSupermoduleCanvas[smID]->Clear();
    mSupermoduleCanvas[smID]->Divide(2, 2);
    mSupermoduleCanvas[smID]->SetTopMargin(0.1);
    int currentpad = 0;
    TLegend* leg = nullptr;
    bool isFirst = true;
    for (int ifec = 0; ifec < 40; ifec++) {
      if (ifec % 10 == 0) {
        currentpad++;
        mSupermoduleCanvas[smID]->cd(currentpad);

        leg = new TLegend(0.15, 0.75, 0.89, 0.89);
        leg->SetBorderSize(0);
        leg->SetFillStyle(0);
        leg->SetTextFont(42);
        leg->SetNColumns(5);
        isFirst = true;
      }
      bool found = true;
      switch (smtypes[smID]) {
        case SMType::FULL:
          // exclude LEDMON (0) and TRU FECs (10, 20, 30) as they leave no data in the occupancy plot
          found = (ifec != 0) && (ifec != 10) && (ifec != 20) && (ifec != 30);
          break;
        case SMType::SMALL_A:
          found = std::find(fecs_small_A.begin(), fecs_small_A.end(), ifec) != fecs_small_A.end();
          break;
        case SMType::SMALL_C:
          found = std::find(fecs_small_C.begin(), fecs_small_C.end(), ifec) != fecs_small_C.end();
          break;
        case SMType::DCAL:
          found = std::find(fecs_DCAL.begin(), fecs_DCAL.end(), ifec) != fecs_DCAL.end();
          break;
        default:
          break;
      };
      if (!found) {
        continue;
      }
      int globalfec = ifec + minfec;
      const char* drawoptions = (isFirst) ? "pl" : "plsame";
      datatree->Draw(Form("cellOccupancyEMCwThr_PHYS.fecCounts[%d]:time", globalfec), "", drawoptions);
      // gPad->GetListOfPrimitives()->ls();
      auto targetgraph = static_cast<TGraph*>(gPad->GetPrimitive("Graph"));
      targetgraph->SetName(Form("fectrend%d", ifec));
      targetgraph->SetMarkerColor(colors[ifec % 10]);
      targetgraph->SetLineColor(colors[ifec % 10]);
      targetgraph->SetMarkerStyle(markers[ifec % 10]);
      leg->AddEntry(targetgraph, Form("FEC %d", ifec), "lep");

      if (isFirst) {
        auto frame = static_cast<TH1*>(gPad->GetPrimitive("htemp"));
        frame->SetName(Form("frame_SM%d_branch_%d", smID, ifec / 10));
        frame->GetXaxis()->SetTimeDisplay(1);
        frame->GetXaxis()->SetNdivisions(503);
        frame->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
        frame->GetXaxis()->SetTimeOffset(0, "gmt");
        frame->GetYaxis()->SetLimits(0., mMaxRate);
        frame->SetYTitle("FEC counts / minute");
        frame->SetTitle(Form("Branch %d", ifec / 10));
        leg->Draw();
        isFirst = false;
      }

      gPad->Update();
    }

    mSupermoduleCanvas[smID]->cd();
    TLatex* msg = new TLatex(0.35, 0.96, Form("Supermodule %d (%s)", smID, mIndicesConverter.GetOnlineSMIndex(smID).data()));
    msg->SetNDC();
    msg->SetTextSize(0.05);
    msg->SetTextFont(62);
    msg->Draw();
    mSupermoduleCanvas[smID]->Update();
  }
}

void FECRateVisualization::finalize(Trigger, framework::ServiceRegistryRef)
{
  for (auto& canvas : mSupermoduleCanvas) {
    getObjectsManager()->stopPublishing(canvas.get());
  }
}

} // namespace o2::quality_control_modules::emcal
