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
/// \file   ITSFhrTask.h
/// \author Liang Zhang
/// \author Jian Liu
///

#ifndef QC_MODULE_ITS_ITSFHRTASK_H
#define QC_MODULE_ITS_ITSFHRTASK_H

#include "QualityControl/TaskInterface.h"
#include <ITSMFTReconstruction/ChipMappingITS.h>
#include <ITSMFTReconstruction/PixelData.h>
#include <ITSBase/GeometryTGeo.h>
#include <ITSMFTReconstruction/RawPixelDecoder.h>

#include <TText.h>
#include <TH1.h>
#include <TH2.h>
#include <THnSparse.h>

class TH1F;
class TH2;

using namespace o2::quality_control::core;
using PixelReader = o2::itsmft::PixelReader;

namespace o2::quality_control_modules::its
{

/// \brief ITS Fake-hit rate real-time data processing task
/// Working with the chain of "Detector -> RU -> CRU -> Readout -> STFB -> o2-dpl-raw-proxy ->  QC"
class ITSFhrTask final : public TaskInterface
{
  using ChipPixelData = o2::itsmft::ChipPixelData;

 public:
  /// \brief Constructor
  ITSFhrTask();
  /// Destructor
  ~ITSFhrTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  int mAverageProcessTime = 0;
  void setAxisTitle(TH1* object, const char* xTitle, const char* yTitle);
  void createGeneralPlots(int barrel); //create General PLots for IB/OB/ALL (1/2/3)
  void createErrorTriggerPlots();
  void createOccupancyPlots();
  void setPlotsFormat();
  void getEnableLayers();
  void resetGeneralPlots();
  void resetOccupancyPlots();
  //detector information
  static constexpr int NCols = 1024; //column number in Alpide chip
  static constexpr int NRows = 512;  //row number in Alpide chip
  static constexpr int NLayer = 7;   //layer number in ITS detector
  static constexpr int NLayerIB = 3;

  const int NSubStave[NLayer] = { 1, 1, 1, 2, 2, 2, 2 };
  const int NStaves[NLayer] = { 12, 16, 20, 24, 30, 42, 48 };
  const int nHicPerStave[NLayer] = { 1, 1, 1, 8, 8, 14, 14 };
  const int nChipsPerHic[NLayer] = { 9, 9, 9, 14, 14, 14, 14 };
  //const int ChipBoundary[NLayer + 1] = { 0, 108, 252, 432, 3120, 6480, 14712, 24120 };
  const int StaveBoundary[NLayer + 1] = { 0, 12, 28, 48, 72, 102, 144, 192 };
  const int ReduceFraction = 1; //TODO: move to Config file to define this number

  std::array<bool, NLayer> mEnableLayers = { false };

  int mNThreads = 0;
  std::unordered_map<unsigned int, int> mHitPixelID_Hash[7][48][2][14][14]; //layer, stave, substave, hic, chip

  o2::itsmft::RawPixelDecoder<o2::itsmft::ChipMappingITS>* mDecoder;
  ChipPixelData* mChipDataBuffer = nullptr;
  std::vector<ChipPixelData> mChipsBuffer;
  int mHitNumberOfChip[7][48][2][14][14] = { { { { { 0 } } } } }; //layer, stave, substave, hic, chip
  int mTimeFrameId = 0;
  uint32_t mTriggerTypeCount[13] = { 0 };

  int mNError = 19;
  int mNTrigger = 13;
  unsigned int mErrors[19] = { 0 };
  static constexpr int NTrigger = 13;
  int16_t partID = 0;

  TString mTriggerType[NTrigger] = { "ORBIT", "HB", "HBr", "HC", "PHYSICS", "PP", "CAL", "SOT", "EOT", "SOC", "EOC", "TF", "INT" };

  //General plots
  TH1F* mTFInfo; //count vs TF ID
  TH1D* mErrorPlots;
  TH2I* mErrorVsFeeid;
  TH2I* mTriggerVsFeeid;
  TH1D* mTriggerPlots;
  //TH1D* mInfoCanvas;//TODO: default, not implemented yet
  TH2I* mInfoCanvasComm;   //tmp object decidated to ITS commissioning
  TH2I* mInfoCanvasOBComm; //tmp object decidated to ITS Outer Barral commissioning

  TText* mTextForShifter;
  TText* mTextForShifter2;

  //Occupancy and hit-map
  THnSparseI* mStaveHitmap[7][48];
  TH2D* mChipStaveOccupancy[7];
  TH2I* mChipStaveEventHitCheck[7];
  TH1D* mOccupancyPlot[7];

  std::string mRunNumberPath;
  std::string mRunNumber = "000000";

  //Geometry decoder
  o2::its::GeometryTGeo* mGeom;
  std::string mGeomPath;
};
} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSFHRTASK_H
