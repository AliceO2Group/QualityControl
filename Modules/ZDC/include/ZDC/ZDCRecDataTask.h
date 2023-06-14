// Copyright 2019-2022 CERN and copyright holders of ALICE O2.
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
/// \file   ZDCRecDataTask.h
/// \author Carlo Puggioni
///

#ifndef QC_MODULE_ZDC_ZDCZDCRECDATATASK_H
#define QC_MODULE_ZDC_ZDCZDCRECDATATASK_H

#include "QualityControl/TaskInterface.h"
#include "ZDCBase/Constants.h"
#include "ZDCSimulation/ZDCSimParam.h"
#include "DataFormatsZDC/BCRecData.h"
#include "DataFormatsZDC/RecEventFlat.h"
#include "DataFormatsZDC/ZDCEnergy.h"
#include "DataFormatsZDC/ZDCTDCData.h"
#include "SimulationDataFormat/DigitizationContext.h"
#include "ZDCBase/Constants.h"
#include "ZDCSimulation/Digitizer.h"
#include <TH1.h>
#include <TH2.h>
#include <map>
#include <string>
#include <vector>

class TH1F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::zdc
{

/// \brief Quality Control ZDC Rec Task
/// \author My Name
class ZDCRecDataTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  ZDCRecDataTask() = default;
  /// Destructor
  ~ZDCRecDataTask() override;

  struct sHisto1D {
    TH1* histo;
    std::string ch;
    std::string typeh;
    std::string typech;
    int bin;
  };
  struct sHisto2D {
    TH2* histo;
    std::string typeh;
    std::string ch1;
    std::string ch2;
    std::string typech1;
    std::string typech2;
  };

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;
  void init();
  void initVecCh();
  void initVecType();
  void initHisto();
  void insertChVec(std::string ch);
  void insertTypeVec(std::string type);
  void setBinHisto1D(int numBinX, double minBinX, double maxBinX);
  void setBinHisto2D(int numBinX, double minBinX, double maxBinX, int numBinY, double minBinY, double maxBinY);
  void setNumBinX(int nbin) { fNumBinX = nbin; };
  void setMinBinX(double min) { fMinBinX = min; };
  void setMaxBinX(double max) { fMaxBinX = max; };
  void setNumBinY(int nbin) { fNumBinY = nbin; };
  void setMinBinY(double min) { fMinBinY = min; };
  void setMaxBinY(double max) { fMaxBinY = max; };
  int getNumBinX() { return fNumBinX; };
  int getMinBinX() { return fMinBinX; };
  int getMaxBinX() { return fMaxBinX; };
  int getNumBinY() { return fNumBinY; };
  int getMinBinY() { return fMinBinY; };
  int getMaxBinY() { return fMaxBinY; };
  float getADCRecValue(std::string typech, std::string ch);
  int getIdTDCch(std::string typech, std::string ch);
  // int getTDCRecValue(int tdcid);
  bool addNewHisto(std::string typeH, std::string name, std::string title, std::string typeCh1, std::string ch1, std::string typeCh2, std::string ch2, int bin);
  bool add1DHisto(std::string typeH, std::string name, std::string title, std::string typeCh1, std::string ch, int bin);
  bool add2DHisto(std::string typeH, std::string name, std::string title, std::string typeCh1, std::string ch1, std::string typeCh2, std::string ch2);
  void dumpHistoStructure();
  int process(const gsl::span<const o2::zdc::BCRecData>& RecBC, const gsl::span<const o2::zdc::ZDCEnergy>& Energy, const gsl::span<const o2::zdc::ZDCTDCData>& TDCData, const gsl::span<const uint16_t>& Info);
  bool FillTDCValueHisto();
  std::vector<std::string> tokenLine(std::string Line, std::string Delimiter);

 private:
  std::vector<std::string> mVecCh;
  std::vector<std::string> mVecType;
  std::vector<std::string> mNameHisto;
  std::vector<std::string> mVecTDC{
    "ZNAC",
    "ZNAS",
    "ZPAC",
    "ZPAS",
    "ZEM1",
    "ZEM2",
    "ZNCC",
    "ZNCS",
    "ZPCC",
    "ZPCS",
  };
  std::vector<sHisto1D> mHisto1D;
  std::vector<sHisto2D> mHisto2D;
  o2::zdc::RecEventFlat mEv;
  int fNumBinX = 0;
  double fMinBinX = 0;
  double fMaxBinX = 0;
  int fNumBinY = 0;
  double fMinBinY = 0;
  double fMaxBinY = 0;
  int mIdhTDC = 0;
  int mIdhADC = 0;
  // TH1F* mHistogram = nullptr;
};

} // namespace o2::quality_control_modules::zdc

#endif // QC_MODULE_ZDC_ZDCZDCRECDATATASK_H
