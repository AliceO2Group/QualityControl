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

#ifndef O2_MERGEABLETH1OCCUPANCYPERDECYCLE_H
#define O2_MERGEABLETH1OCCUPANCYPERDECYCLE_H

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
#ifdef MCH_HAS_MAPPING_FACTORY
#include "MCHMappingFactory/CreateSegmentation.h"
#endif
#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingInterface/CathodeSegmentation.h"
#include "MCHMappingSegContour/CathodeSegmentationContours.h"

using namespace std;
namespace o2::quality_control_modules::muonchambers
{

class MergeableTH1OccupancyPerDECycle : public TH1F, public o2::mergers::MergeInterface
{
 public:
  MergeableTH1OccupancyPerDECycle() = default;

  MergeableTH1OccupancyPerDECycle(MergeableTH1OccupancyPerDECycle const& copymerge)
    : TH1F("DefaultNameCycle", "DefaultTitleCycle", 1100, -0.5, 1099.5), o2::mergers::MergeInterface(), mhistoNum(copymerge.getNum()), mhistoDen(copymerge.getDen())
  {
  }

  MergeableTH1OccupancyPerDECycle(const char* name, const char* title, TH2F* histonum, TH2F* histoden)
    : TH1F(name, title, 1100, -0.5, 1099.5), o2::mergers::MergeInterface(), mhistoNum(histonum), mhistoDen(histoden)
  {
    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      LastMeanNhitsDE[de] = 0;
      LastMeanNorbitsDE[de] = 0;
    }
    update();
  }

  ~MergeableTH1OccupancyPerDECycle() override = default;

  void merge(MergeInterface* const other) override
  {
    mhistoNum->Add(dynamic_cast<const MergeableTH1OccupancyPerDECycle* const>(other)->getNum());
    mhistoDen->Add(dynamic_cast<const MergeableTH1OccupancyPerDECycle* const>(other)->getDen());

    const double* ptrotherLastMeanNhitsDE = dynamic_cast<const MergeableTH1OccupancyPerDECycle* const>(other)->getLastMeanNhitsDE();
    const double* ptrotherLastMeanNorbitsDE = dynamic_cast<const MergeableTH1OccupancyPerDECycle* const>(other)->getLastMeanNorbitsDE();

    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      LastMeanNhitsDE[de] += ptrotherLastMeanNhitsDE[de];
      LastMeanNorbitsDE[de] += ptrotherLastMeanNorbitsDE[de];
    }

    const double* ptrotherNewMeanNhitsDE = dynamic_cast<const MergeableTH1OccupancyPerDECycle* const>(other)->getNewMeanNhitsDE();
    const double* ptrotherNewMeanNorbitsDE = dynamic_cast<const MergeableTH1OccupancyPerDECycle* const>(other)->getNewMeanNorbitsDE();

    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      NewMeanNhitsDE[de] += ptrotherNewMeanNhitsDE[de];
      NewMeanNorbitsDE[de] += ptrotherNewMeanNorbitsDE[de];
    }

    updateAfterMerge();
  }

  TH2F* getNum() const
  {
    return mhistoNum;
  }

  TH2F* getDen() const
  {
    return mhistoDen;
  }

  const double* getLastMeanNhitsDE() const
  {
    const double* ptrLastMeanNhitsDE = LastMeanNhitsDE;
    return ptrLastMeanNhitsDE;
  }

  const double* getLastMeanNorbitsDE() const
  {
    const double* ptrLastMeanNorbitsDE = LastMeanNorbitsDE;
    return ptrLastMeanNorbitsDE;
  }

  const double* getNewMeanNhitsDE() const
  {
    const double* ptrNewMeanNhitsDE = NewMeanNhitsDE;
    return ptrNewMeanNhitsDE;
  }

  const double* getNewMeanNorbitsDE() const
  {
    const double* ptrNewMeanNorbitsDE = NewMeanNorbitsDE;
    return ptrNewMeanNorbitsDE;
  }

  void update()
  {
    const char* name = this->GetName();
    const char* title = this->GetTitle();
    Reset();
    SetNameTitle(name, title);

    o2::mch::raw::Elec2DetMapper mElec2DetMapper = o2::mch::raw::createElec2DetMapper<o2::mch::raw::ElectronicMapperGenerated>();
    o2::mch::raw::Det2ElecMapper mDet2ElecMapper = o2::mch::raw::createDet2ElecMapper<o2::mch::raw::ElectronicMapperGenerated>();
    o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper = o2::mch::raw::createFeeLink2SolarMapper<o2::mch::raw::ElectronicMapperGenerated>();
    o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper = o2::mch::raw::createSolar2FeeLinkMapper<o2::mch::raw::ElectronicMapperGenerated>();

    // Using NHitsElec and Norbits to get the mean occupancy per DE on last cycle
    // Similarly, by looking at NHits and NOrbits Elec histogram, for each bin incrementing the corresponding DE total hits and total orbits, and from the values obtained at the end of a cycle compared to the beginning of the cycle, computing the occupancy during the cycle.
    auto hhits = mhistoNum;
    auto horbits = mhistoDen;
    int NbinsDE[1100]{ 0 };
    if (hhits && horbits) {
      for (auto de : o2::mch::raw::deIdsForAllMCH) {
        LastMeanNhitsDE[de] = NewMeanNhitsDE[de];
        LastMeanNorbitsDE[de] = NewMeanNorbitsDE[de];
        NewMeanNhitsDE[de] = 0;
        NewMeanNorbitsDE[de] = 0;
      }
      for (int binx = 1; binx < hhits->GetXaxis()->GetNbins() + 1; binx++) {
        for (int biny = 1; biny < hhits->GetYaxis()->GetNbins() + 1; biny++) {
          // Same procedure as above in getting Elec info
          uint32_t dsAddr = (binx - 1) % 40;
          uint32_t linkId = ((binx - 1 - dsAddr) / 40) % 12;
          uint32_t feeId = (binx - 1 - dsAddr - 40 * linkId) / (12 * 40);
          uint32_t de;
          std::optional<uint16_t> solarId = mFeeLink2SolarMapper(o2::mch::raw::FeeLinkId{ static_cast<uint16_t>(feeId), static_cast<uint8_t>(linkId) });
          if (!solarId.has_value()) {
            continue;
          }
          std::optional<o2::mch::raw::DsDetId> dsDetId =
            mElec2DetMapper(o2::mch::raw::DsElecId{ solarId.value(), static_cast<uint8_t>(dsAddr / 5), static_cast<uint8_t>(dsAddr % 5) });
          if (!dsDetId.has_value()) {
            continue;
          }
          de = dsDetId->deId();

          NewMeanNhitsDE[de] += hhits->GetBinContent(binx, biny);
          NewMeanNorbitsDE[de] += horbits->GetBinContent(binx, biny);
          NbinsDE[de] += 1;
        }
      }
      for (auto i : o2::mch::raw::deIdsForAllMCH) {
        double MeanOccupancyDECycle = 0;
        if (NbinsDE[i] > 0) {
          NewMeanNhitsDE[i] /= NbinsDE[i];
          NewMeanNorbitsDE[i] /= NbinsDE[i];
        }
        if ((NewMeanNorbitsDE[i] - LastMeanNorbitsDE[i]) > 0) {
          MeanOccupancyDECycle = (NewMeanNhitsDE[i] - LastMeanNhitsDE[i]) / (NewMeanNorbitsDE[i] - LastMeanNorbitsDE[i]) / 87.5; //Scaling to MHz
        }
        SetBinContent(i + 1, MeanOccupancyDECycle);
      }
    }
  }

  void updateAfterMerge()
  {
    const char* name = this->GetName();
    const char* title = this->GetTitle();
    Reset();
    SetNameTitle(name, title);

    // Using NHitsElec and Norbits to get the mean occupancy per DE on last cycle
    // Similarly, by looking at NHits and NOrbits Elec histogram, for each bin incrementing the corresponding DE total hits and total orbits, and from the values obtained at the end of a cycle compared to the beginning of the cycle, computing the occupancy during the cycle.
    auto hhits = mhistoNum;
    auto horbits = mhistoDen;
    if (hhits && horbits) {
      for (auto i : o2::mch::raw::deIdsForAllMCH) {
        double MeanOccupancyDECycle = 0;
        if ((NewMeanNorbitsDE[i] - LastMeanNorbitsDE[i]) > 0) {
          MeanOccupancyDECycle = (NewMeanNhitsDE[i] - LastMeanNhitsDE[i]) / (NewMeanNorbitsDE[i] - LastMeanNorbitsDE[i]) / 87.5; //Scaling to MHz
        }
        SetBinContent(i + 1, MeanOccupancyDECycle);
      }
    }
  }

 private:
  TH2F* mhistoNum{ nullptr };
  TH2F* mhistoDen{ nullptr };
  std::string mTreatMeAs = "TH1F";
  double NewMeanNhitsDE[1100]{ 0 };
  double NewMeanNorbitsDE[1100]{ 0 };
  double LastMeanNhitsDE[1100]{ 0 };
  double LastMeanNorbitsDE[1100]{ 0 };

  ClassDefOverride(MergeableTH1OccupancyPerDECycle, 1);
};

} // namespace o2::quality_control_modules::muonchambers

#endif //O2_MERGEABLETH1OCCUPANCYPERDECYCLE_H
