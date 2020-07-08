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
/// \file   ITSOnlineTask.h
/// \author Liang Zhang
/// \author Jian Liu
///

#ifndef QC_MODULE_ITS_ITSONLINETASK_H
#define QC_MODULE_ITS_ITSONLINETASK_H

#include "QualityControl/TaskInterface.h"
#include <ITSMFTReconstruction/ChipMappingITS.h>
#include <ITSMFTReconstruction/PixelData.h>
#include <ITSBase/GeometryTGeo.h>
#include <ITSMFTReconstruction/RawPixelReader.h>
#include <TH2I.h>
#include <THnSparse.h>
#include <pthread.h>

class TH2I;
class TH1F;

using namespace o2::quality_control::core;
using namespace o2::framework;
using namespace std;
using namespace o2::itsmft;
using namespace o2::header;
using ChipPixelData = o2::itsmft::ChipPixelData;
using PixelReader = o2::itsmft::PixelReader;

namespace o2::quality_control_modules::its
{

/// \brief ITS Fake-hit rate real-time data processing task
/// Working with the chain of "Detector -> RU -> CRU -> Readout -> STFB -> DPL + QC"
class ITSOnlineTask final : public TaskInterface
{
  using ChipPixelData = o2::itsmft::ChipPixelData;
  struct DecodeInfomation {                   //transfer variables to threads
    static constexpr int NumberOfErrors = 17; //Error infomation
    std::array<std::array<unsigned int, NumberOfErrors>, 5> errorsCount = { 0 };

    static constexpr int NumberOfTrigger = 13; //Trigger infomation
    std::array<unsigned int, NumberOfTrigger> triggersCount;

    o2::its::GeometryTGeo* Geom; //Geometry infomation

    int threadId;
    std::vector<int> feeId;
    std::vector<int> layerId;
    std::vector<uint8_t*> payloadMessage; //vector of raw data pointer
    std::vector<int> messageSize;         //vector of payload size

    std::unique_ptr<o2::itsmft::RawPixelReader<o2::itsmft::ChipMappingITS>> rawReader[5]; //decoder
    ChipPixelData* chipDataBuffer = nullptr;
    std::vector<ChipPixelData> chipsBuffer;

    //declare some plots pointer of decoding result, just include inner barrel
    TH2I* hicHitMap[3][20]; //TODO: replaced by THnSparse
    TH2D* chipStaveOccupancy[3];
    //plots declare end

    std::vector<std::pair<int, int>> hitPixelID[3][20][9]; //The ID of hit pixel
    std::vector<int> hitNumber[3][20][9];                  //The number of hits in one pixel, corresponds to HitPixelID

    int hitNumberOfChip[3][20][9] = { 0 }; //Hit number of chip
    //int eventNumber[3][20][9] = { 0 };	//Actually it is not be used

    pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER; //mutex
  };

 public:
  /// \brief Constructor
  ITSOnlineTask();
  /// Destructor
  ~ITSOnlineTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

  void createDecoder();
  void createPlots();

  static void* decodeThread(void* threadarg);

 private:
  int timeFrameId = 0;
  static constexpr int sThreadNumber = 2;
  std::array<struct DecodeInfomation, sThreadNumber> threadInfomation; //information for thread
                                                                       //included inputs: raw data, payloadsize
                                                                       //	  output: plots

  int mNError = 17;
  int mNTrigger = 13;
  unsigned int mErrors[17] = { 0 };
  //General plots
  TH1F* mProcessingTime; //plots for check processing time
  TH1F* mProcessingTimevsTF;
  TH1F* mTFInfo; //count vs TF ID
  TH1D* mErrorPlots;
  TH1D* mTriggerPlots;
  //General plots end

  //Occupancy plots
  //the memory address of plots, the pointers in threads will point these address
  //memory will allocate in init() function
  TH2I* mHicHitmapAddress[3][20]; //TODO: replaced by THnSparse
  TH2D* mChipStaveOccupancy[3];
  TH1D* mOccupancyPlot[3];
  //Occupancy plots end

  //Geometry decoder
  //the memory address of Geometry decoder, the pointers in threads will point these address
  o2::its::GeometryTGeo* mGeom;
  //Geometry decoder end
};

} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSONLINETASK_H
