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
/// \brief Base Class to describe HMPID Equipment
/// \version 1.0
/// \date 24 set 2020

/* ------ HISTORY ---------
*/

#include "HMPID/HmpidEquipment.h"

using namespace o2::quality_control_modules::hmpid;

// ============= HmpidEquipment Class implementation =======

/// Constructor : map the Equipment_ID with the CRU_Id and Link_Id
///
/// @param[in] Equipment : the HMPID EquipmentId [0..13]
/// @param[in] Cru : the HMPID Cru [0..3] (FLP160 = 0,1 FLP161 = 2,3)
/// @param[in] Link : the FLP Link [0..3]
HmpidEquipment::HmpidEquipment(int Equipment, int Cru, int Link)
{
  mEquipmentId = Equipment;
  mCruId = Cru;
  mLinkId = Link;
  return;
}

/// Destructor : do nothing
HmpidEquipment::~HmpidEquipment()
{
  return;
}

/// Inits the members for the decoding
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

/// Resets the matrix that contains the results of the decoding
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

/// Resets the decoding errors statistics
void HmpidEquipment::resetErrors()
{
  for (int i = 0; i < MAXERRORS; i++)
    mErrors[i] = 0;
  return;
}

/// Setup an error by type
/// TODO : control of array boundary
/// @param[in] ErrType : the Decoding error type [0..MAXERRORS]
void HmpidEquipment::setError(int ErrType)
{
  mErrors[ErrType]++;
  mErrorsCounter++;
  return;
}

/// Set the charge value of a pad into the three statistics
/// matrix : Entries, Sum of charge, Sum of Charge squares
/// @param[in] col : column [0..23]
/// @param[in] dil : dilogic [0..9]
/// @param[in] cha : channel [0..47]
/// @param[in] charge : the value of the charge
void HmpidEquipment::setPad(int col, int dil, int cha, int charge)
{
  mPadSamples[col][dil][cha]++;
  mPadSum[col][dil][cha] += (double) charge;
  mPadSquares[col][dil][cha] += (double) charge * (double) charge;
  return;
}

/// Return the EquipmentId with the check of CRU_Id and Link_Id
/// @param[in] cru : FLP CRU Id [0..3]
/// @param[in] link : CRU Link Id [0..3]
/// @returns the Equipment Id
int HmpidEquipment::getEquipmentId(int cru, int link)
{
  if (cru == mCruId && link == mLinkId)
    return (mEquipmentId);
  else
    return (-1);
}

namespace o2::quality_control_modules::hmpid {

// =================== General Purposes HMPID Functions =======================
/// Functions to translate coordinates : from Module,Col,Row to Equipment,Col,Dilogic,Channel
/// Digit coordinates " Mod,Row,Col := Mod = {0..6}  Row = {0..159}  Col = {0..143}
///                    (0,0) Left Bottom
///
/// Hardware coordinates  Equ,Col,Dil,Cha := Equ = {0..13}  Col = {0..23}  Dil = {0..9}  Cha = {0..47}
///
///                    (0,0,0,0) Right Top   (1,0,0,0) Left Bottom
///
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

/// Functions to translate coordinates : from Equipment,Col,Dilogic,Channel to Module,Col,Row
/// Digit coordinates " Mod,Row,Col := Mod = {0..6}  Row = {0..159}  Col = {0..143}
///                    (0,0) Left Bottom
///
/// Hardware coordinates  Equ,Col,Dil,Cha := Equ = {0..13}  Col = {0..23}  Dil = {0..9}  Cha = {0..47}
///
///                    (0,0,0,0) Right Top   (1,0,0,0) Left Bottom
///
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
