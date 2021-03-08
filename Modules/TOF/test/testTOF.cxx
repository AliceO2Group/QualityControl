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
/// \file   testTOF.cxx
/// \author
///

#include "QualityControl/TaskFactory.h"
#include "Base/Counter.h"
#include "DataFormatsTOF/CompressedDataFormat.h"
#include "TH1F.h"

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

namespace o2::quality_control_modules::tof
{

BOOST_AUTO_TEST_CASE(instantiate_task) { BOOST_CHECK(true); }

BOOST_AUTO_TEST_CASE(check_tof_counter)
{
  BOOST_TEST_CHECKPOINT("Starting");

  TH1F* hFull = new TH1F("hFull", "hFull;DRM Word;Crate;Words", 32, 0, 32);
  Counter<32, o2::tof::diagnostic::DRMDiagnosticName> counterFull;
  BOOST_CHECK_MESSAGE(counterFull.MakeHistogram(hFull) == 0, "Did not correctly make the histogram for Full labels");
  int nFull = 0;
  for (int i = 0; i < 32; i++) {
    if (o2::tof::diagnostic::DRMDiagnosticName[i] && o2::tof::diagnostic::DRMDiagnosticName[i][0]) {
      nFull++;
    }
  }

  TH1F* hEmpty = new TH1F("hEmpty", "hEmpty;DRM Word;Crate;Words", 32, 0, 32);
  Counter<32, nullptr> counterEmpty;
  BOOST_CHECK_MESSAGE(counterEmpty.MakeHistogram(hEmpty) == 0, "Did not correctly make the histogram for Empty labels");
  BOOST_CHECK_MESSAGE((hFull->GetNbinsX() == 32) || (hFull->GetNbinsX() + hEmpty->GetNbinsX()) == (32 + nFull),
                      "Sum of histogram size does not match the number of possible words " << hFull->GetNbinsX() + hEmpty->GetNbinsX() << " vs " << (32 + nFull));

  const unsigned int n = 1000;
  for (unsigned int i = 0; i < n; i++) {
    for (unsigned int j = 0; j < 32; j++) {
      if (o2::tof::diagnostic::DRMDiagnosticName[j] && o2::tof::diagnostic::DRMDiagnosticName[j][0]) {
        counterFull.Count(j);
      } else {
        counterEmpty.Count(j);
      }
    }
  }

  LOG(INFO) << "Printing counter of full labels";
  counterFull.Print();
  for (unsigned int j = 0; j < 32; j++) {
    unsigned int expected = 0;
    if (o2::tof::diagnostic::DRMDiagnosticName[j] && o2::tof::diagnostic::DRMDiagnosticName[j][0]) {
      expected = n;
    }
    BOOST_CHECK_MESSAGE(counterFull.HowMany(j) == expected,
                        "Issue with the counts in " << o2::tof::diagnostic::DRMDiagnosticName[j] << ": " << counterFull.HowMany(j) << " should be " << expected);
  }
  BOOST_CHECK_MESSAGE(counterFull.FillHistogram(hFull) == 0, "Did not correctly fill the histogram for Full labels");
  BOOST_CHECK_MESSAGE(hFull->Integral() == n * nFull,
                      "Issue with the counts in histogram of full counter, expected " << n * nFull << " entries and got " << hFull->Integral());
  for (int j = 0; j < hFull->GetNbinsX(); j++) {
    if (hFull->GetXaxis()->GetBinLabel(j + 1)[0]) {
      BOOST_CHECK_MESSAGE(hFull->GetBinContent(j + 1) == n,
                          "Issue with the counts in bin " << j + 1 << " they must be " << n << " and instead are: " << hFull->GetBinContent(j + 1));
    }
    LOG(INFO) << "in: " << j + 1 << "/" << hFull->GetNbinsX() + 1 << " (bin '" << hFull->GetXaxis()->GetBinLabel(j + 1) << "') there are " << hFull->GetBinContent(j + 1) << " counts";
  }

  LOG(INFO) << "Printing counter of empty labels";
  counterEmpty.Print();
  for (unsigned int j = 0; j < 32; j++) {
    unsigned int expected = n;
    if (o2::tof::diagnostic::DRMDiagnosticName[j] && o2::tof::diagnostic::DRMDiagnosticName[j][0]) {
      expected = 0;
    }
    BOOST_CHECK_MESSAGE(counterEmpty.HowMany(j) == expected,
                        "Issue with the counts in " << o2::tof::diagnostic::DRMDiagnosticName[j] << ": " << counterEmpty.HowMany(j) << " should be " << expected);
  }
  BOOST_CHECK_MESSAGE(counterEmpty.FillHistogram(hEmpty) == 0, "Did not correctly fill the histogram for Empty labels");
  BOOST_CHECK_MESSAGE(hEmpty->Integral() == n * (32 - nFull),
                      "Issue with the counts in histogram of empty counter, expected " << n * (32 - nFull) << " entries and got " << hEmpty->Integral());

  BOOST_TEST_CHECKPOINT("Ending");
  BOOST_CHECK(true);
}
} // namespace o2::quality_control_modules::tof