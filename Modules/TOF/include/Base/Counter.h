// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
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

  /// Function to increment a counter
  /// @param v Index in the counter array to increment
  void Count(const unsigned int& v);

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
  uint32_t HowMany(const unsigned int& pos) const { return counter[pos]; }

  /// Function to make a histogram out of the counters
  /// @param h histogram to shape in order to have room for the counter size
  void MakeHistogram(TH1* h) const;

  /// Function to fill a histogram with the counters
  /// @param h The histogram to fill
  /// @param biny Y offset to fill to histogram, useful for TH2 and TH3
  /// @param binz Z offset to fill to histogram, useful for TH3
  void FillHistogram(TH1* h, const unsigned int& biny = 0, const unsigned int& binz = 0) const;

  /// Getter for the size
  /// @return Returns the size of the counter
  unsigned int Size() const { return size; }

 private:
  // static_assert((sizeof(labels) / sizeof(const char*)) == size, "size of the counter and the one of the labels must coincide");
  /// Containers to fill
  std::array<uint32_t, size> counter = { 0 };
  uint32_t mTotal = 0;
};

////////////////////
// Implementation //
////////////////////

// #define ENABLE_COUNTER_DEBUG_MODE // Flag used to enable more printing and more debug
// #define ENABLE_PRINT_HISTOGRAMS_MODE // Flag used to enable more printing and more debug

namespace o2::quality_control_modules::tof
{

template <const unsigned int size, const char* labels[size]>
void Counter<size, labels>::Count(const unsigned int& v)
{
  if (v > size) {
    LOG(FATAL) << "Incrementing counter too far! " << v << "/" << size;
  }
#ifdef ENABLE_COUNTER_DEBUG_MODE
  LOG(INFO) << "Incrementing " << v << "/" << size << " to " << counter[v];
#endif
  counter[v]++;
}

template <const unsigned int size, const char* labels[size]>
void Counter<size, labels>::Reset()
{
  LOG(INFO) << "Resetting Counter";
  for (unsigned int i = 0; i < size; i++) {
    counter[i] = 0;
  }
}

template <const unsigned int size, const char* labels[size]>
void Counter<size, labels>::Print()
{
  for (unsigned int i = 0; i < size; i++) {
    if (labels) {
      LOG(INFO) << "Bin " << i << "/" << size - 1 << " " << labels[i] << " = " << HowMany(i);
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
void Counter<size, labels>::MakeHistogram(TH1* h) const
{
  LOG(INFO) << "Making Histogram " << h->GetName() << " to accomodate counter of size " << size;
  TAxis* axis = h->GetXaxis();
  if (static_cast<unsigned int>(axis->GetNbins()) < size) {
    LOG(FATAL) << "The histogram size (" << axis->GetNbins() << ") is not large enough to accomodate the counter size (" << size << ")";
  }
  h->Reset();
  unsigned int histo_size = size;
  if (labels) { // Only if labels are defined
    for (unsigned int i = 0; i < size; i++) {
      if (labels[i] && !labels[i][0]) { // If label at position i is empty
        LOG(DEBUG) << "Skipping label '" << labels[i] << "' at position " << i << "/" << size - 1;
        histo_size--;
      }
    }
  }
  if (histo_size == 0) {
    LOG(FATAL) << "Asked to produce a histogram with size " << histo_size << ", check counter bin labels";
  } else {
    LOG(DEBUG) << "Asked to produce a histogram with size " << histo_size << " out of the size " << size << " due to empty labels";
  }

  axis->Set(histo_size, 0, histo_size);
  if (labels) { // Only if labels are defined
    unsigned int binx = 1;
    for (unsigned int i = 0; i < size; i++) {
      if (labels[i] && !labels[i][0]) { // If label at position i is empty
        continue;
      }
      LOG(DEBUG) << "Setting bin " << binx << "/" << histo_size << " to contain counter for '" << labels[i] << "' (index " << i << "/" << size - 1 << ")";
      if (histo_size < binx) {
        LOG(FATAL) << "Making bin outside of histogram limits!";
      }
      axis->SetBinLabel(binx++, labels[i]);
    }
  }
  h->Reset();
#ifdef ENABLE_PRINT_HISTOGRAMS_MODE
  h->Print("All");
#endif
}

template <const unsigned int size, const char* labels[size]>
void Counter<size, labels>::FillHistogram(TH1* h, const unsigned int& biny, const unsigned int& binz) const
{
  LOG(INFO) << "Filling Histogram " << h->GetName() << " with counter contents";
  unsigned int binx = 1;
  const unsigned int nbinsx = h->GetNbinsX();
  for (unsigned int i = 0; i < size; i++) {
    if (labels && labels[i] && !labels[i][0]) { // Labels are defined and label is empty
      if (counter[i] > 0) {
        LOG(FATAL) << "Counter at position " << i << " was non empty (" << counter[i] << ") but was discarded because of empty labels";
      }
      continue;
    }
#ifdef ENABLE_COUNTER_DEBUG_MODE
    LOG(INFO) << "Filling bin " << binx << " of position " << i << " of label " << labels[i] << " with " << counter[i];
#endif
    if (binx > nbinsx) {
      LOG(FATAL) << "Filling histogram " << h->GetName() << " at position " << binx << " i.e. past its size (" << nbinsx << ")!";
    }
    const char* bin_label = h->GetXaxis()->GetBinLabel(binx);
    if (labels) {
      if (!labels[i]) {
        LOG(DEBUG) << "Label at position " << i << " does not exist for axis label '" << bin_label << "'";
      } else if (strcmp(labels[i], bin_label) != 0) {
        LOG(FATAL) << "Bin " << binx << " does not have the expected label '" << bin_label << "' vs '" << labels[i] << "'";
      }
    }
    if (counter[i] > 0) {
      if (biny > 0) {
        if (binz > 0) {
          h->SetBinContent(binx, biny, binz, counter[i]);
        } else {
          h->SetBinContent(binx, biny, counter[i]);
        }
      } else {
        h->SetBinContent(binx, counter[i]);
      }
    }
    binx++;
  }
#ifdef ENABLE_PRINT_HISTOGRAMS_MODE
  h->Print("All");
#endif
}

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_COUNTER_H
