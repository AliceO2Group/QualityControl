/*
 * HmpidEquipments.cpp
 *
 *  Created on: 24 set 2020
 *      Author: fap
 */

#include "HMPID/HmpidEquipment.h"

using namespace Equipment;

// ============= HmpidEquipment Class implementation =======

HmpidEquipment::HmpidEquipment(int Equipment, int Cru, int Link)
{
  mEquipmentId = Equipment;
  mCruId = Cru;
  mLinkId = Link;
  return;
}

HmpidEquipment::~HmpidEquipment()
{
  return;
}

void HmpidEquipment::init()
{
  mWillBeRowMarker = true;
  mWillBeSegmentMarker = false;
  mWillBeEoE = false;
  mWillBePad = false;
  mRowSize = 0;
  mSegment = 0;
  mColumnCounter = 0;
  mWordsPerRowCounter = 0;
  mWordsPerSegCounter = 0;
  mWordsPerDilogicCounter = 0;
  mSampleNumber = 0;
  mErrorsCounter = 0;
  mErrorPadsPerEvent = 0;

  mEventNumber = -1;
  mNumberOfEvents = 0;

  mBusyTimeValue = 0.0;
  mBusyTimeAverage = 0.0;
  mBusyTimeSamples = 0;

  mEventSizeAverage = 0.0;
  mEventSize = 0;

  mPadsPerEventAverage = 0.0;

  mNumberOfEmptyEvents = 0;
  mNumberOfWrongEvents = 0;
  mTotalPads = 0;
  mTotalErrors = 0;

  return;
}

void HmpidEquipment::resetPadMap()
{
  for (int r = 0; r < N_COLUMNS; r++)
    for (int d = 0; d < N_DILOGICS; d++)
      for (int c = 0; c < N_CHANNELS; c++) {
        mPadSamples[r][d][c] = 0;
        mPadSum[r][d][c] = 0.0;
        mPadSquares[r][d][c] = 0.0;
      }
  return;
}

void HmpidEquipment::resetErrors()
{
  for (int i = 0; i < MAXERRORS; i++)
    mErrors[i] = 0;
  return;
}

void HmpidEquipment::setError(int ErrType)
{
  mErrors[ErrType]++;
  mErrorsCounter++;
  return;
}

void HmpidEquipment::setPad(int col, int dil, int cha, int charge)
{
  mPadSamples[col][dil][cha]++;
  mPadSum[col][dil][cha] += (double) charge;
  mPadSquares[col][dil][cha] += (double) charge * (double) charge;
  return;
}

int HmpidEquipment::getEquipmentId(int cru, int link)
{
  if (cru == mCruId && link == mLinkId)
    return (mEquipmentId);
  else
    return (-1);
}

namespace Equipment {
// =========================================================
// Functions to translate coordinates
// Digit coordinates " Mod,Row,Col := Mod = {0..6}  Row = {0..159}  Col = {0..143}
//                    (0,0) Left Bottom
//
// Hardware coordinates  Equ,Col,Dil,Cha := Equ = {0..13}  Col = {0..23}  Dil = {0..9}  Cha = {0..47}
//
//                    (0,0,0,0) Right Top   (1,0,0,0) Left Bottom
//

void hmpidCoordsModule2Equipment(int Mod, int Col, int Row, int *Equi, int *Colu, int *Dilo, int *Chan)
{
  if (Row > 79) {
    *Equi = Mod * 2 + 1;
    Row = Row - 80;
  } else {
    *Equi = Mod * 2;
    Row = 79 - Row;
    Col = 143 - Col;
  }
  *Dilo = Row / 8;
  *Colu = Col / 6;
  *Chan = (Row % 8) * 6 + (Col % 6);
  return;
}

void hmpidCoordsEquipment2Module(int Equi, int Colu, int Dilo, int Chan, int *Mod, int *Col, int *Row)
{
  *Mod = Equi / 2;
  *Row = Dilo * 8 + Chan / 8;
  *Col = (Colu * 6) + Chan % 6;

  if (Equi % 2 == 1) {
    *Row += 80;
  } else {
    *Row = 79 - *Row;
    *Col = 143 - *Col;
  }
  return;
}

}
