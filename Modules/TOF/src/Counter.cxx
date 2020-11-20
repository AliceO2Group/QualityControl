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
/// \file   Counter.cxx
/// \author Nicolo' Jacazio
/// \since 24/03/2020
/// \brief Utilities to count events and fill histograms at the end of the main processing loops
///

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TOF/TaskRaw.h"

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
