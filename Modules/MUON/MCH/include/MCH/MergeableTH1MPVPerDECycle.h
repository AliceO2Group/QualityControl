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

#ifndef O2_MERGEABLETH1MPVPERDECYCLE_H
#define O2_MERGEABLETH1MPVPERDECYCLE_H

#include <sstream>
#include <iostream>
#include <TObject.h>
#include <TF1.h>
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

using namespace std;
namespace o2::quality_control_modules::muonchambers
{

class MergeableTH1MPVPerDECycle : public TH1F, public o2::mergers::MergeInterface
{
 public:
  MergeableTH1MPVPerDECycle() = default;

  MergeableTH1MPVPerDECycle(MergeableTH1MPVPerDECycle const& copymerge)
    : TH1F("DefaultNameCycle", "DefaultTitleCycle", 1100, -0.5, 1099.5), o2::mergers::MergeInterface(), mhistosCharge(copymerge.getNum())
  {
  }

  MergeableTH1MPVPerDECycle(const char* name, const char* title, std::map<int, TH1F*> histosnum)
    : TH1F(name, title, 1100, -0.5, 1099.5), o2::mergers::MergeInterface(), mhistosCharge(histosnum)
  {
    update();
  }

  ~MergeableTH1MPVPerDECycle() override = default;

  void merge(MergeInterface* const other) override
  { //FIXME
    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      auto hnumMap = dynamic_cast<const MergeableTH1MPVPerDECycle* const>(other)->getNum();
      auto hnum = hnumMap.find(de);
      if ((hnum != dynamic_cast<const MergeableTH1MPVPerDECycle* const>(other)->getNum().end()) && (hnum->second != NULL)) {

        auto hnumfinal = mhistosCharge.find(de);
        if ((hnumfinal != mhistosCharge.end()) && (hnumfinal->second != NULL)) {
          hnumfinal->second->Add(hnum->second);
        }
      }
    }

    update();
  }

  std::map<int, TH1F*> getNum() const
  {
    return mhistosCharge;
  }

  void update()
  { //FIXME
    const char* name = this->GetName();
    const char* title = this->GetTitle();
    Reset();
    SetNameTitle(name, title);

    TF1* f1 = new TF1("f1", "landau", 200, 5000); //1650V [200,5k]    1700V [400,10k]

    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      auto hLandauCycle = mhistosCharge.find(de);
      if ((hLandauCycle != mhistosCharge.end()) && (hLandauCycle->second != NULL)) {

        double chindf = 0;
        double mpv = 0;

        if (hLandauCycle->second->GetEntries() > 0) {
          f1->SetParameter(1, 0);
          f1->SetParameter(2, 500); // Start Sigma at 500
                                    //  f1->FixParameter(1,1000);           // Fix MPV 1650V 500    1700V 1000
          f1->SetParLimits(2, 100, 10000);
          f1->SetParLimits(1, 0, 10000);
          f1->SetParLimits(0, 0, 100000); // Say we want a positive Normalisation
          TFitResultPtr ptr = hLandauCycle->second->Fit("f1", "RB");
          chindf = f1->GetChisquare() / f1->GetNDF();
          mpv = f1->GetParameter(1);
          if (mpv > 0) {
            cout << "DE : " << de << endl;
            cout << "Chi2/ndf :" << chindf << endl;
            cout << "MPV :" << mpv << endl;
          }
        }

        SetBinContent(de + 1, mpv);
      }
    }
  }

 private:
  std::map<int, TH1F*> mhistosCharge;
  std::string mTreatMeAs = "TH1F";

  ClassDefOverride(MergeableTH1MPVPerDECycle, 1);
};

} // namespace o2::quality_control_modules::muonchambers

#endif //O2_MERGEABLETH1MPVPERDECYCLE_H
