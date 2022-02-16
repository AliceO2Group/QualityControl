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
/// \file   ZDCRawDataTask.h
/// \author Carlo Puggioni
///

#ifndef QC_MODULE_ZDC_ZDCZDCRAWDATATASK_H
#define QC_MODULE_ZDC_ZDCZDCRAWDATATASK_H

#include "QualityControl/TaskInterface.h"
#include <TH1.h>
#include <TH2.h>
#include <map>
#include "ZDCBase/Constants.h"
#include "ZDCSimulation/ZDCSimParam.h"
#include "DataFormatsZDC/RawEventData.h"
#include <string>
#include <vector>

class TH1F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::zdc
{

/// \brief Example Quality Control DPL Task
/// \author Carlo Puggioni
class ZDCRawDataTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  ZDCRawDataTask() = default;
  /// Destructor
  ~ZDCRawDataTask() override;

// Definition structures
  struct infoHisto {
    int idHisto;
    std::vector <std::string> condHisto;
  };

  struct infoHisto1D {
    TH1* histo;
    std::vector <std::string> condHisto;
  };
  struct infoHisto2D {
    TH2* histo;
    std::vector <std::string> condHisto;
  };
  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;
  void init();
  void initHisto();
  int process(const o2::zdc::EventData& ev);
  int process(const o2::zdc::EventChData& ch);
  int processWord(const uint32_t* word);
  int getHPos(uint32_t board, uint32_t ch,int matrix[o2::zdc::NModules][o2::zdc::NChPerModule]);
  std::string getNameChannel(int imod, int ich);
  void setNameChannel(int imod,int ich, std::string namech);
  void setBinHisto1D(int numBinX, double minBinX, double maxBinX );
  void setBinHisto2D(int numBinX, double minBinX, double maxBinX, int numBinY, double minBinY, double maxBinY);
  void setNumBinX(int   nbin){fNumBinX=nbin;};
  void setMinBinX(double min){fMinBinX=min;};
  void setMaxBinX(double max){fMaxBinX=max;};
  void setNumBinY(int   nbin){fNumBinY=nbin;};
  void setMinBinY(double min){fMinBinY=min;};
  void setMaxBinY(double max){fMaxBinY=max;};
  int getNumBinX(){return fNumBinX;};
  int getMinBinX(){return fMinBinX;};
  int getMaxBinX(){return fMaxBinX;};
  int getNumBinY(){return fNumBinY;};
  int getMinBinY(){return fMinBinY;};
  int getMaxBinY(){return fMaxBinY;};
  bool getModAndCh(std::string chName,int * module, int * channel);
  bool addNewHisto(std::string type,std::string name,std::string title,std::string chName, std::string condition);
  std::string removeSpaces( std::string s );
  std::vector<std::string> tokenLine(std::string Line, std::string Delimiter);
  bool configureRawDataTask();
  bool checkCondition(std::string cond);
  bool decodeConfLine(std::vector<std::string> tokenString,int lineNumber);
  bool decodeModule(std::vector<std::string> tokenString,int lineNumber);
  bool decodeBinHistogram(std::vector<std::string> tokenString,int lineNumber);
  bool decodeBaseline(std::vector<std::string> tokenString,int lineNumber);
  bool decodeCounts(std::vector<std::string> tokenString,int lineNumber);
  bool decodeSignal(std::vector<std::string> tokenString,int lineNumber);
  bool decodeBunch(std::vector<std::string> tokenString,int lineNumber);
  bool decodeFireChannel(std::vector<std::string> tokenString,int lineNumber);
  bool decodeTrasmittedChannel(std::vector<std::string> tokenString,int lineNumber);
  bool decodeSummaryBaseline(std::vector<std::string> tokenString,int lineNumber);
  void DumpHistoStructure();
  void setVerbosity(int v)
  {
    mVerbosity = v;
  }
  int getVerbosity() const { return mVerbosity; }

 private:
  TH1F* mHistogram = nullptr;
  void setStat(TH1* h);
  int mVerbosity = 1;

  o2::zdc::EventChData mCh;
  std::string fNameChannel[o2::zdc::NModules][o2::zdc::NChPerModule];
  std::vector<infoHisto1D> fMatrixHistoBaseline[o2::zdc::NModules][o2::zdc::NChPerModule];
  std::vector<infoHisto1D> fMatrixHistoCounts[o2::zdc::NModules][o2::zdc::NChPerModule];
  std::vector<infoHisto2D> fMatrixHistoSignal[o2::zdc::NModules][o2::zdc::NChPerModule];
  std::vector<infoHisto2D> fMatrixHistoBunch[o2::zdc::NModules][o2::zdc::NChPerModule];

  TH2* fFireChannel;
  TH2* fTrasmChannel;
  TH1* fSummaryPedestal;

  std::vector <std::string> fNameHisto;
  std::map<std::string, int> fMapBinNameIdSummaryHisto;

  std::map <std::string, std::vector<int>> fMapChNameModCh;

  int fNumBinX=0;
  double fMinBinX=0;
  double fMaxBinX=0;
  int fNumBinY=0;
  double fMinBinY=0;
  double fMaxBinY=0;

};

} // namespace o2::quality_control_modules::zdc

#endif // QC_MODULE_ZDC_ZDCZDCRAWDATATASK_H
