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
/// \file   HmpidEquipments.h
/// \author Antonio Franco - INFN Bari
/// \brief Base Class to describe HMPID Equipment for the decoding of RawData stream
/// \version 1.0
/// \date 24 set 2020

#ifndef COMMON_HMPIDEQUIPMENT_H_
#define COMMON_HMPIDEQUIPMENT_H_

#include <cstdio>
#include <stdint.h>
#include <iostream>

namespace o2::quality_control_modules::hmpid
{

const int MAXERRORS = 13;
const int MAXHMPIDERRORS = 5;

const int ERR_NOTKNOWN = 0;
const int ERR_ROWMARKEMPTY = 1;
const int ERR_DUPLICATEPAD = 2;
const int ERR_ROWMARKWRONG = 3;
const int ERR_ROWMARKLOST = 4;
const int ERR_ROWMARKERROR = 5;
const int ERR_LOSTEOEMARK = 6;
const int ERR_DOUBLEEOEMARK = 7;
const int ERR_WRONGSIZEINEOE = 8;
const int ERR_DOUBLEMARKWORD = 9;
const int ERR_WRONGSIZESEGMENTMARK = 10;
const int ERR_LOSTEOSMARK = 11;
const int ERR_HMPID = 12;

// ---- HMPID geometry -------
const int MAXEQUIPMENTS = 14;

const int N_SEGMENTS = 3;
const int N_COLXSEGMENT = 8;
const int N_COLUMNS = 24;
const int N_DILOGICS = 10;
const int N_CHANNELS = 48;

const int N_MODULES = 7;
const int N_XROWS = 160;
const int N_YCOLS = 144;

const int N_EQUIPMENTTOTALPADS = N_SEGMENTS * N_COLXSEGMENT * N_DILOGICS * N_CHANNELS;
const int N_HMPIDTOTALPADS = MAXEQUIPMENTS * N_SEGMENTS * N_COLXSEGMENT * N_DILOGICS * N_CHANNELS;

// ---- HMPID error def -------
const int TH_FILENOTEXISTS = 9;
const int TH_OPENFILE = 8;
const int TH_CREATEFILE = 7;
const int TH_READFILE = 6;
const int TH_WRITEFILE = 5;
const int TH_WRONGEQUIPINDEX = 19;
const int TH_WRONGHEADER = 15;
const int TH_WRONGFILELEN = 14;
const int TH_NULLBUFFERPOINTER = 13;
const int TH_BUFFEREMPTY = 12;
const int TH_WRONGBUFFERDIM = 11;

class HmpidEquipment
{

 private:
  uint32_t mEquipmentId;
  uint32_t mCruId;
  uint32_t mLinkId;

 public:
  uint32_t mPadSamples[N_COLUMNS][N_DILOGICS][N_CHANNELS];
  double mPadSum[N_COLUMNS][N_DILOGICS][N_CHANNELS];
  double mPadSquares[N_COLUMNS][N_DILOGICS][N_CHANNELS];

  int mErrors[MAXERRORS];

  int mWillBeRowMarker;
  int mWillBeSegmentMarker;
  int mWillBeEoE;
  int mWillBePad;
  int mRowSize;
  int mSegment;
  int mColumnCounter;
  int mWordsPerRowCounter;
  int mWordsPerSegCounter;
  int mWordsPerDilogicCounter;

  int mErrorsCounter;
  int mErrorPadsPerEvent;

  int mEventNumber;
  int mNumberOfEvents;
  float mEventSizeAverage;
  int mEventSize;

  int mSampleNumber;
  float mPadsPerEventAverage;

  float mBusyTimeValue;
  float mBusyTimeAverage;
  int mBusyTimeSamples;
  int mNumberOfEmptyEvents;
  int mNumberOfWrongEvents;
  int mTotalPads;
  int mTotalErrors;

 public:
  HmpidEquipment(int Equipment, int Cru, int Link);
  ~HmpidEquipment();

  int getEquipmentId()
  {
    return (mEquipmentId);
  };
  int getEquipmentId(int cru, int link);

  void init();
  void resetPadMap();
  void resetErrors();
  void setError(int ErrType);
  void setPad(int col, int dil, int cha, int charge);
};

void hmpidCoordsModule2Equipment(int Mod, int Col, int Row, int* Equi, int* Colu, int* Dilo, int* Chan);
void hmpidCoordsEquipment2Module(int Equi, int Colu, int Dilo, int Chan, int* Mod, int* Col, int* Row);

} // namespace o2::quality_control_modules::hmpid

#endif /* COMMON_HMPIDEQUIPMENT_H_ */
