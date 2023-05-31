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
/// \file   ITSDecodingErrorCheck.h
/// \auhtor Zhen Zhang
///

#ifndef QC_MODULE_ITS_ITSDECODINGERRORCHECK_H
#define QC_MODULE_ITS_ITSDECODINGERRORCHECK_H

#include "QualityControl/CheckInterface.h"
#include <TLatex.h>
#include <string>
#include <vector>
#include <sstream>

namespace o2::quality_control_modules::its
{

/// \brief  Check the FAULT flag for the lanes

class ITSDecodingErrorCheck : public o2::quality_control::checker::CheckInterface
{

 public:
  /// Default constructor
  ITSDecodingErrorCheck() = default;
  /// Destructor
  ~ITSDecodingErrorCheck() override = default;

  // Override interface
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::vector<int> vDecErrorLimits, vListErrorIdBad, vListErrorIdMedium;
  bool doFlatCheck = false;

  TString sErrorDesc[23] = {
    "Page data does not start with expected RDH",                        // ErrNoRDHAtStart
    "RDH is stopped, but the time is not matching the stop packet",      // ErrPageNotStopped
    "Page with RDH.stop does not contain diagnostic word only",          // ErrStopPageNotEmpty
    "RDH page counters for the same RU/trigger are not continuous",      // ErrPageCounterDiscontinuity
    "RDH and GBT header page counters are not consistent",               // ErrRDHvsGBTHPageCnt
    "GBT trigger word was expected but not found",                       // ErrMissingGBTTrigger
    "GBT payload header was expected but not found",                     // ErrMissingGBTHeader
    "GBT payload trailer was expected but not found",                    // ErrMissingGBTTrailer
    "All lanes were stopped but the page counter in not 0",              // ErrNonZeroPageAfterStop
    "End of FEE data reached while not all lanes received stop",         // ErrUnstoppedLanes
    "Data was received for stopped lane",                                // ErrDataForStoppedLane
    "No data was seen for lane (which was not in timeout)",              // ErrNoDataForActiveLane
    "ChipID (on module) was different from the lane ID on the IB stave", // ErrIBChipLaneMismatch
    "Cable data does not start with chip header or empty chip",          // ErrCableDataHeadWrong
    "Active lanes pattern conflicts with expected for given RU type",    // ErrInvalidActiveLanes
    "Jump in RDH_packetCounter",                                         // ErrPacketCounterJump
    "Packet done is missing in the trailer while CRU page is not over",  // ErrPacketDoneMissing
    "Wrong/missing diagnostic GBT word after RDH with stop",             // ErrMissingDiagnosticWord
    "GBT word not recognized",                                           // ErrGBTWordNotRecognized
    "Wrong cable ID",                                                    // ErrWrongeCableID
    "Unexpected CRU page alignment padding word",                        // ErrWrongAlignmentWord
    "ROF in future, pause decoding to synchronize",                      // ErrMissingROF
    "Old ROF, discarding",                                               // ErrOldROF
  };
  template <typename T>
  std::vector<T> convertToArray(std::string input)
  {

    std::istringstream ss{ input };

    std::vector<T> result;
    std::string token;

    while (std::getline(ss, token, ',')) {

      if constexpr (std::is_same_v<T, int>) {
        result.push_back(std::stoi(token));
      } else if constexpr (std::is_same_v<T, std::string>) {
        result.push_back(token);
      }
    }
    return result;
  }

 private:
  ClassDefOverride(ITSDecodingErrorCheck, 1);

  std::shared_ptr<TLatex> tInfo;
};

} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSDecodingErrorCheck_H
