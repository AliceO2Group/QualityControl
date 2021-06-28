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
/// \file   Counter.h
/// \author Nicolo' Jacazio
/// \since 24/03/2020
/// \brief Utilities to count events and fill histograms at the end of the main processing loops.
///        Can be used to count labelled and non labelled events and fill histograms only once.
///

#ifndef QC_MODULE_TOF_COUNTER_H
#define QC_MODULE_TOF_COUNTER_H

// ROOT includes
#include "TH1.h"
#include "TMath.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"

// Fairlogger includes
#include <fairlogger/Logger.h>

namespace o2::quality_control_modules::tof
{

/// \brief Class to count events
/// \author Nicolo' Jacazio
template <const unsigned int size, const char* labels[size]>
class Counter
{
 public:
  /// \brief Constructor
  Counter() = default;

  /// Destructor
  ~Counter() = default;

  /// Functions to increment a counter
  /// @param index Index in the counter array to increment
  /// @param weight weight to add to the array element
  void Add(const unsigned int& index, const uint32_t& weight);

  /// Functions to count a single event
  /// @param index Index in the counter array to increment by one
  void Count(const unsigned int& index) { Add(index, 1); }

  /// Function to reset counters to zero
  void Reset();

  /// Function to print the counter content
  void Print();

  /// Function to get the total number of counts
  uint32_t Total();

  /// Function to get the total number of counts added since last call
  uint32_t TotalNew();

  /// Function to get the total number of counts and reset the counts
  uint32_t TotalAndReset();

  /// Function to get how many counts where observed
  /// @return Returns the number of counts observed for a particular index
  uint32_t HowMany(const unsigned int& index) const { return counter[index]; }

  /// Function to check if the counter has a label at position index
  /// @param index Index in the counter array to be checked
  /// @return Returns if the counter has label corresponding to a particular index. Returns false if the label is not there or is empty.
  constexpr bool HasLabel(const unsigned int& index) const;

  /// Function to make a histogram out of the counters. If a counter has labels defined these are used as axis labels, if not this will not be done.
  /// @param histogram histogram to shape in order to have room for the counter size
  /// @returns Returns 0 if everything went OK
  int MakeHistogram(TH1* histogram) const;

  /// Function to fill a histogram with the counters
  /// @param histogram The histogram to fill
  /// @param biny Y offset to fill to histogram, useful for TH2 and TH3
  /// @param binz Z offset to fill to histogram, useful for TH3
  /// @returns Returns 0 if everything went OK
  int FillHistogram(TH1* histogram, const unsigned int& biny = 0, const unsigned int& binz = 0) const;

  /// Getter for the size
  /// @return Returns the size of the counter
  unsigned int Size() const { return size; }

 private:
  static_assert(size > 0, "size of the counter cannot be 0!");
  // static_assert(((labels == nullptr) || (sizeof(labels) / sizeof(const char*)) == size), "size of the counter and the one of the labels must coincide");
  /// Containers to fill
  std::array<uint32_t, size> counter = { 0 };
  uint32_t mTotal = 0;
};

////////////////////
// Implementation //
////////////////////

// #define ENABLE_BIN_SHIFT // Flag used to enable different binning in counter and histograms

template <const unsigned int size, const char* labels[size]>
void Counter<size, labels>::Add(const unsigned int& index, const uint32_t& weight)
{
  if (index > size) {
    LOG(FATAL) << "Incrementing counter too far! " << index << "/" << size;
  }
  LOG(DEBUG) << "Incrementing " << index << "/" << size << " of " << weight << " to " << counter[index];
  counter[index] += weight;
}

template <const unsigned int size, const char* labels[size]>
void Counter<size, labels>::Reset()
{
  LOG(DEBUG) << "Resetting Counter";
  for (unsigned int i = 0; i < size; i++) {
    counter[i] = 0;
  }
}

template <const unsigned int size, const char* labels[size]>
constexpr bool Counter<size, labels>::HasLabel(const unsigned int& index) const
{
  if constexpr (labels != nullptr) {
    return (labels[index] && labels[index][0]);
  }
  return false;
}

template <const unsigned int size, const char* labels[size]>
void Counter<size, labels>::Print()
{
  for (unsigned int i = 0; i < size; i++) {
    if (labels != nullptr) {
      LOG(INFO) << "Bin " << i << "/" << size - 1 << " '" << labels[i] << "' = " << HowMany(i);
    } else {
      LOG(INFO) << "Bin " << i << "/" << size - 1 << " = " << HowMany(i);
    }
  }
}

template <const unsigned int size, const char* labels[size]>
uint32_t Counter<size, labels>::Total()
{
  uint32_t sum = 0;
  for (unsigned int i = 0; i < size; i++) {
    sum += counter[i];
  }
  mTotal = sum;
  return sum;
}

template <const unsigned int size, const char* labels[size]>
uint32_t Counter<size, labels>::TotalNew()
{
  uint32_t sum = mTotal;
  return sum - Total();
}

template <const unsigned int size, const char* labels[size]>
uint32_t Counter<size, labels>::TotalAndReset()
{
  uint32_t sum = Total();
  Reset();
  return sum;
}

template <const unsigned int size, const char* labels[size]>
int Counter<size, labels>::MakeHistogram(TH1* histogram) const
{
  LOG(DEBUG) << "Making Histogram " << histogram->GetName() << " to accomodate counter of size " << size;
  TAxis* axis = histogram->GetXaxis();
  if (static_cast<unsigned int>(axis->GetNbins()) < size) {
    LOG(FATAL) << "The histogram size (" << axis->GetNbins() << ") is not large enough to accomodate the counter size (" << size << ")";
    return 1;
  }
  histogram->Reset();

#ifndef ENABLE_BIN_SHIFT
  LOG(DEBUG) << "Asked to produce a histogram with size " << size << " out of the size " << size << " due to " << size - size << " empty labels";
  axis->Set(size, 0, size);
  for (unsigned int i = 0; i < size; i++) {
    if (!HasLabel(i)) { // If label at position i is empty
      continue;
    }
    LOG(DEBUG) << "Setting bin " << i + 1 << "/" << size << " to contain counter for '" << labels[i] << "' (index " << i << "/" << size - 1 << ")";
    axis->SetBinLabel(i + 1, labels[i]);
  }
#else
  unsigned int histo_size = size;
  if (labels != nullptr) { // Only if labels are defined
    for (unsigned int i = 0; i < size; i++) {
      if (!HasLabel(i)) { // If label at position i is empty or does not exist
        LOG(DEBUG) << "Skipping label '" << labels[i] << "' at position " << i << "/" << size - 1;
        histo_size--;
      }
    }
  }
  if (histo_size == 0) {
    LOG(FATAL) << "Asked to produce a histogram with size " << histo_size << ", check counter bin labels";
    return 1;
  } else {
    LOG(DEBUG) << "Asked to produce a histogram with size " << histo_size << " out of the size " << size << " due to " << size - histo_size << " empty labels";
  }

  axis->Set(histo_size, 0, histo_size);
  if (labels != nullptr) { // Only if labels are defined
    unsigned int binx = 1;
    for (unsigned int i = 0; i < size; i++) {
      if (!HasLabel(i)) { // If label at position i is empty
        continue;
      }
      if (histo_size < binx) {
        LOG(FATAL) << "Making bin outside of histogram limits!";
        return 1;
      }
      LOG(DEBUG) << "Setting bin " << binx << "/" << histo_size << " to contain counter for '" << labels[i] << "' (index " << i << "/" << size - 1 << ")";
      axis->SetBinLabel(binx++, labels[i]);
    }
  }
#endif
  histogram->Reset();
  return 0;
}

template <const unsigned int size, const char* labels[size]>
int Counter<size, labels>::FillHistogram(TH1* histogram, const unsigned int& biny, const unsigned int& binz) const
{
  auto fillIt = [&](const unsigned int& bin, const unsigned int& index) {
    if (counter[index] > 0) {
      if (biny > 0) {
        if (binz > 0) {
          histogram->SetBinContent(bin, biny, binz, counter[index]);
          histogram->SetBinError(bin, biny, binz, TMath::Sqrt(counter[index]));
        } else {
          histogram->SetBinContent(bin, biny, counter[index]);
          histogram->SetBinError(bin, biny, TMath::Sqrt(counter[index]));
        }
      } else {
        histogram->SetBinContent(bin, counter[index]);
        histogram->SetBinError(bin, TMath::Sqrt(counter[index]));
      }
    }
  };

  LOG(DEBUG) << "Filling Histogram " << histogram->GetName() << " with counter contents";
#ifndef ENABLE_BIN_SHIFT
  if (size != (histogram->GetNbinsX())) {
    LOG(FATAL) << "Counter of size " << size << " does not fit in histogram " << histogram->GetName() << " with size " << histogram->GetNbinsX() - 1;
    return 1;
  }
  for (unsigned int i = 0; i < size; i++) {
    LOG(DEBUG) << "Filling bin " << i + 1 << " with counter at position " << i;
    if (HasLabel(i) && strcmp(labels[i], histogram->GetXaxis()->GetBinLabel(i + 1)) != 0) { // If it has a label check its consistency!
      LOG(FATAL) << "Bin " << i + 1 << " does not have the expected label '" << histogram->GetXaxis()->GetBinLabel(i + 1) << "' vs '" << labels[i] << "'";
      return 1;
    }
    fillIt(i + 1, i);
  }
#else
  const unsigned int nbinsx = histogram->GetNbinsX();
  if constexpr (labels == nullptr) { // Fill without labels
    if (nbinsx < size) {
      LOG(FATAL) << "Counter size " << size << " is too large to fit in histogram " << histogram->GetName() << " with size " << nbinsx;
      return 1;
    }
    for (unsigned int i = 0; i < size; i++) {
      LOG(DEBUG) << "Filling bin " << i + 1 << " with position " << i << " with " << counter[i];
      fillIt(i + 1, i);
    }
  } else { // Fill with labels
    unsigned int binx = 1;
    for (unsigned int i = 0; i < size; i++) {
      if (!HasLabel(i)) { // Labels are defined and label is empty
        if (counter[i] > 0) {
          LOG(FATAL) << "Counter at position " << i << " was non empty (" << counter[i] << ") but was discarded because of empty labels";
          return 1;
        }
        continue;
      }
      LOG(DEBUG) << "Filling bin " << binx << " with position " << i << " of label " << labels[i] << " with " << counter[i];
      if (binx > nbinsx) {
        LOG(FATAL) << "Filling histogram " << histogram->GetName() << " at position " << binx << " i.e. past its size (" << nbinsx << ")!";
        return 1;
      }
      const char* bin_label = histogram->GetXaxis()->GetBinLabel(binx);
      if (!HasLabel(i) && counter[i] > 0) {
        LOG(FATAL) << "Label at position " << i << " does not exist for axis label '" << bin_label << "' but counter is *non* empty!";
        return 1;
      } else if (strcmp(labels[i], bin_label) != 0) {
        LOG(FATAL) << "Bin " << binx << " does not have the expected label '" << bin_label << "' vs '" << labels[i] << "'";
        return 1;
      }
      fillIt(binx, i);
      binx++;
    }
    if (binx != nbinsx + 1) {
      LOG(FATAL) << "Did not fully fill histogram " << histogram->GetName() << ", filled " << binx << " out of " << nbinsx;
      return 1;
    }
  }
#endif

  return 0;
}

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_COUNTER_H
