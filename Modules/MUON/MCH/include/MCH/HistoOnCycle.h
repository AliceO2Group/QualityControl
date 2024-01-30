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
/// \file   HistoOnCycle.h
/// \author Andrea Ferrero
///
#ifndef QUALITYCONTROL_MCH_HISTOONCYCLE_H
#define QUALITYCONTROL_MCH_HISTOONCYCLE_H

#include <TH1.h>
#include <array>

namespace o2::quality_control_modules::muonchambers
{

/// \brief An utility class that generates histograms with data from the last processing cycle
///
/// An utility class that generates histograms with data from the last processing cycle

template <typename T>
class HistoOnCycle : public T
{
 public:
  HistoOnCycle() = default;
  ~HistoOnCycle() = default;

  void update(TObject* obj);

 private:
  std::unique_ptr<T> mHistPrevCycle;
};

template <typename T>
void HistoOnCycle<T>::update(TObject* obj)
{
  auto histo = dynamic_cast<T*>(obj);
  if (!histo) {
    return;
  }

  // if not already done, initialize the internal plot that stores the data from the previous cycle
  if (!mHistPrevCycle) {
    std::string name = std::string(T::GetName()) + "PrevCycle";
    std::string title = std::string(T::GetTitle()) + " - PrevCycle";
    Bool_t bStatus = TH1::AddDirectoryStatus();
    TH1::AddDirectory(kFALSE);
    mHistPrevCycle = std::make_unique<T>();
    histo->Copy(*mHistPrevCycle);
    mHistPrevCycle->SetNameTitle(name.c_str(), title.c_str());
    mHistPrevCycle->Reset("ICES");
    TH1::AddDirectory(bStatus);
  }

  histo->Copy(*this);
  T::SetNameTitle(TString::Format("%sOnCycle", histo->GetName()), TString::Format("%s - OnCycle", histo->GetTitle()));
  T::Add(mHistPrevCycle.get(), -1);

  mHistPrevCycle->Reset("ICES");
  mHistPrevCycle->Add(histo);
}

} // namespace o2::quality_control_modules::muonchambers

#endif // QUALITYCONTROL_MCH_HISTOONCYCLE_H
