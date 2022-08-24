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
/// \file   ITSDecodingErrorTask.h
/// \author Zhen Zhang
///

#ifndef QC_MODULE_ITS_ITSDECODINGERRORTASK_H
#define QC_MODULE_ITS_ITSDECODINGERRORTASK_H

#include "QualityControl/TaskInterface.h"
#include <ITSMFTReconstruction/ChipMappingITS.h>
#include <ITSMFTReconstruction/PixelData.h>
#include <ITSBase/GeometryTGeo.h>
#include <ITSMFTReconstruction/RawPixelDecoder.h>

#include <TH1.h>
#include <TH2.h>
#include <TH2Poly.h>
#include "TMath.h"
#include <TLine.h>
#include <TText.h>
#include <TLatex.h>

class TH2I;
class TH1I;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::its
{

/// \brief ITS FEE task aiming at 100% online data integrity checking
class ITSDecodingErrorTask final : public TaskInterface
{
  struct GBTDiagnosticWord { // GBT diagnostic word
    union {
      uint64_t word0 = 0x0;
      struct {
        uint64_t laneStatus : 56;
        uint16_t zero0 : 8;
      } laneBits;
    } laneWord;
    union {
      uint64_t word1 = 0x0;
      struct {
        uint8_t flag1 : 4;
        uint8_t index : 4;
        uint8_t id : 8;
        uint64_t padding : 48;
      } indexBits;
    } indexWord;
  };

 public:
  /// \brief Constructor
  ITSDecodingErrorTask();
  /// Destructor
  ~ITSDecodingErrorTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  void getParameters(); // get Task parameters from json file
  void setAxisTitle(TH1* object, const char* xTitle, const char* yTitle);
  void createDecodingPlots();
  void getStavePoint(int layer, int stave, double* px, double* py); // prepare for fill TH2Poly, get all point for add TH2Poly bin
  void setPlotsFormat();
  void resetGeneralPlots();
  static constexpr int NLayer = 7;
  const int NStaves[NLayer] = { 12, 16, 20, 24, 30, 42, 48 };
  static constexpr int NLayerIB = 3;
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };

  int**** mLinkErrorCount /* = new int***[NStaves[lay]]*/; //  errorcount[layer][stave][FEE][errorid]
  int**** mChipErrorCount /* = new int***[NStaves[lay]]*/; //  errorcount[layer][stave][FEE][errorid]

  o2::itsmft::RawPixelDecoder<o2::itsmft::ChipMappingITS>* mDecoder;
  // parameters taken from the .json
  int mNPayloadSizeBins = 0;
  int mNThreads = 0;

  TH1D* mLinkErrorPlots;
  TH1D* mChipErrorPlots;
  TH2I* mLinkErrorVsFeeid; // link ErrorVsFeeid
  TH2I* mChipErrorVsFeeid; // chip ErrorVsFeeid
  std::string mRunNumberPath;
  std::string mRunNumber = "000000";
};

} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSDECODINGERRORTASK_H
