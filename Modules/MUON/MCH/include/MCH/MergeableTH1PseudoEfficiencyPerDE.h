// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file MergeableTH1OccupancyPerDE.h
/// \brief An example of a custom TH2Quotient inheriting MergeInterface
///
/// \author Piotr Konopka, piotr.jan.konopka@cern.ch, Sebastien Perrin

#ifndef O2_MERGEABLETH1PSEUDOEFFICIENCYPERDE_H
#define O2_MERGEABLETH1PSEUDOEFFICIENCYPERDE_H

#include <sstream>
#include <iostream>
#include <TObject.h>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <algorithm>

#include "Mergers/MergeInterface.h"
#include "MCHRawElecMap/Mapper.h"
#ifdef HAVE_DIGIT_IN_DATAFORMATS
#include "DataFormatsMCH/Digit.h"
#else
#include "MCHBase/Digit.h"
#endif
#include "MCH/GlobalHistogram.h"

using namespace std;
namespace o2::quality_control_modules::muonchambers
{

class MergeableTH1PseudoEfficiencyPerDE : public TH1F, public o2::mergers::MergeInterface
{
 public:
  MergeableTH1PseudoEfficiencyPerDE() = default;

  MergeableTH1PseudoEfficiencyPerDE(MergeableTH1PseudoEfficiencyPerDE const& copymerge)
    : TH1F("DefaultName", "DefaultTitle", 1100, -0.5, 1099.5), o2::mergers::MergeInterface()
  {
    Bool_t bStatus = TH1::AddDirectoryStatus();
    TH1::AddDirectory(kFALSE);
    mHistoNum = (TH1F*)copymerge.getNum()->Clone();
    mHistoDen = (TH1F*)copymerge.getDen()->Clone();
    TH1::AddDirectory(bStatus);
  }

  MergeableTH1PseudoEfficiencyPerDE(const char* name, const char* title)
    : TH1F(name, title, 1100, -0.5, 1099.5), o2::mergers::MergeInterface()
  {
    Bool_t bStatus = TH1::AddDirectoryStatus();
    TH1::AddDirectory(kFALSE);
    mHistoNum = new TH1F("num", "num", 1100, -0.5, 1099.5);
    mHistoDen = new TH1F("den", "den", 1100, -0.5, 1099.5);
    TH1::AddDirectory(bStatus);
    update();
  }

  ~MergeableTH1PseudoEfficiencyPerDE()
  {
    if (mHistoNum) {
      delete mHistoNum;
    }

    if (mHistoDen) {
      delete mHistoDen;
    }
  }

  void merge(MergeInterface* const other) override
  {
    mHistoNum->Add(dynamic_cast<const MergeableTH1PseudoEfficiencyPerDE* const>(other)->getNum());
    mHistoDen->Add(dynamic_cast<const MergeableTH1PseudoEfficiencyPerDE* const>(other)->getDen());

    update();
  }

  TH1F* getNum() const
  {
    return mHistoNum;
  }

  TH1F* getDen() const
  {
    return mHistoDen;
  }

  void update()
  {
    const char* name = this->GetName();
    const char* title = this->GetTitle();
    Reset();
    Divide(mHistoNum, mHistoDen);
    SetNameTitle(name, title);
  }

  void update(std::map<int, std::shared_ptr<DetectorHistogram>> histosNum, std::map<int, std::shared_ptr<DetectorHistogram>> histosDen)
  {
    const char* name = this->GetName();
    const char* title = this->GetTitle();
    Reset();
    SetNameTitle(name, title);

    double numDE[1100] = { 0 };
    double denDE[1100] = { 0 };

    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      auto hnum = histosNum.find(de);
      auto hden = histosDen.find(de);
      if ((hden != histosDen.end()) && (hden->second != NULL) && (hnum != histosNum.end()) && (hnum->second != NULL)) {
        numDE[de] = 0;
        denDE[de] = 0;
        for (int binx = 1; binx < hnum->second->getHist()->GetXaxis()->GetNbins() + 1; binx++) {
          for (int biny = 1; biny < hnum->second->getHist()->GetYaxis()->GetNbins() + 1; biny++) {
            numDE[de] += hnum->second->getHist()->GetBinContent(binx, biny);
          }
        }
        for (int binx = 1; binx < hden->second->getHist()->GetXaxis()->GetNbins() + 1; binx++) {
          for (int biny = 1; biny < hden->second->getHist()->GetYaxis()->GetNbins() + 1; biny++) {
            denDE[de] += hden->second->getHist()->GetBinContent(binx, biny);
          }
        }
      }
    }

    for (auto i : o2::mch::raw::deIdsForAllMCH) {
      mHistoNum->SetBinContent(i + 1, numDE[i]);
      mHistoDen->SetBinContent(i + 1, denDE[i]);
    }

    update();
  }

 private:
  TH1F* mHistoNum{ nullptr };
  TH1F* mHistoDen{ nullptr };
  std::string mTreatMeAs = "TH1F";

  ClassDefOverride(MergeableTH1PseudoEfficiencyPerDE, 2);
};

} // namespace o2::quality_control_modules::muonchambers

#endif //O2_MERGEABLETH1PSEUDOEFFICIENCYPERDE_H
