/*
 * HmpidDecoder.cpp
 *
 *  Created on: 24 set 2020
 *      Author: Antonio Franco - INFN Sez. BARI Italy
 *      v.1.1
 *
 *
 */

#include "HMPID/HmpidDecoder.h"

using namespace Equipment;

// ============= HmpidDecoder Class implementation =======

char HmpidDecoder::sErrorDescription[MAXERRORS][MAXDESCRIPTIONLENGHT] = { "Word that I don't known !",
    "Row Marker Word with 0 words", "Duplicated Pad Word !", "Row Marker Wrong/Lost -> to EoE",
    "Row Marker Wrong/Lost -> to EoE", "Row Marker reports an ERROR !", "Lost EoE Marker !", "Double EoE marker",
    "Wrong size definition in EoE Marker", "Double Mark Word", "Wrong Size in Segment Marker", "Lost EoS Marker !",
    "HMPID Header Errors" };

char HmpidDecoder::sHmpidErrorDescription[MAXHMPIDERRORS][MAXDESCRIPTIONLENGHT] = { "L0 Missing,"
    "L1 is received without L0", "L1A signal arrived before the L1 Latency", "L1A signal arrived after the L1 Latency",
    "L1A is missing or L1 timeout", "L1A Message is missing or L1 Message" };

HmpidDecoder::HmpidDecoder(int *EqIds, int *CruIds, int *LinkIds, int numOfEquipments)
{
  mNumberOfEquipments = numOfEquipments;
  for (int i = 0; i < mNumberOfEquipments; i++) {
    mTheEquipments[i] = new HmpidEquipment(EqIds[i], CruIds[i], LinkIds[i]);
  }
}

HmpidDecoder::HmpidDecoder(int numOfEquipments)
{
  // The standard definition of HMPID equipments at P2
  int EqIds[] = { 0, 1, 2, 3, 4, 5, 8, 9, 6, 7, 10, 11, 12, 13 };
  int CruIds[] = { 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3 };
  int LinkIds[] = { 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 0, 1, 2 };

  mNumberOfEquipments = numOfEquipments;
  for (int i = 0; i < mNumberOfEquipments; i++) {
    mTheEquipments[i] = new HmpidEquipment(EqIds[i], CruIds[i], LinkIds[i]);
  }
}

HmpidDecoder::~HmpidDecoder()
{
  for (int i = 0; i < mNumberOfEquipments; i++) {
    delete mTheEquipments[i];
  }
}

void HmpidDecoder::init()
{
  mVerbose = 0;
  mHeEvent = 0;
  mHeBusy = 0;
  mNumberWordToRead = 0;
  mPayloadTail = 0;

  mHeFEEID = 0;
  mHeSize = 0;
  mHeVer = 0;
  mHePrior = 0;
  mHeStop = 0;
  mHePages = 0;
  mEquipment = 0;

  mHeOffsetNewPack = 0;
  mHeMemorySize = 0;

  mHeDetectorID = 0;
  mHeDW = 0;
  mHeCruID = 0;
  mHePackNum = 0;
  mHePAR = 0;
  mHePageNum = 0;
  mHeLinkNum = 0;
  mHeFirmwareVersion = 0;
  mHeHmpidError = 0;
  mHeBCDI = 0;
  mHeORBIT = 0;
  mHeTType = 0;

  mActualStreamPtr = 0;
  mEndStreamPtr = 0;
  mStartStreamPtr = 0;

}

// Returns the Equipment Index (Pointer of the array with Hardware coords
int HmpidDecoder::getEquipmentIndex(int CruId, int LinkId)
{
  for (int i = 0; i < mNumberOfEquipments; i++) {
    if (mTheEquipments[i]->getEquipmentId(CruId, LinkId) != -1) {
      return (i);
    }
  }
  return (-1);
}

// Returns the Equipment Index (Pointer of the array with Equipment ID
int HmpidDecoder::getEquipmentIndex(int EquipmentId)
{
  for (int i = 0; i < mNumberOfEquipments; i++) {
    if (mTheEquipments[i]->getEquipmentId() == EquipmentId) {
      return (i);
    }
  }
  return (-1);
}

// Returns the Equipment Index (Pointer of the array with Hardware coords
int HmpidDecoder::getEquipmentID(int CruId, int LinkId)
{
  for (int i = 0; i < mNumberOfEquipments; i++) {
    if (mTheEquipments[i]->getEquipmentId(CruId, LinkId) != -1) {
      return (mTheEquipments[i]->getEquipmentId());
    }
  }
  return (-1);
}

int HmpidDecoder::checkType(int32_t wp, int *p1, int *p2, int *p3, int *p4)
{
  if ((wp & 0x0000ffff) == 0x36A8 || (wp & 0x0000ffff) == 0x32A8 || (wp & 0x0000ffff) == 0x30A0
      || (wp & 0x0800ffff) == 0x080010A0) {
    *p2 = (wp & 0x03ff0000) >> 16; // Number of words of row
    *p1 = wp & 0x0000ffff;
    return (1);
  }
  if ((wp & 0xfff00000) >> 20 == 0xAB0) {
    *p2 = (wp & 0x000fff00) >> 8; // Number of words of Segment
    *p1 = (wp & 0xfff00000) >> 20;
    *p3 = wp & 0x0000000F;
    if (*p3 < 4 && *p3 > 0) {
      return (2);
    }
  }
  // #EX MASK Raul 0x3803FF80  # ex mask 0xF803FF80 - this is EoE marker 0586800B0
  if ((wp & 0x0803FF80) == 0x08000080) {
    *p1 = (wp & 0x07c00000) >> 22;
    *p2 = (wp & 0x003C0000) >> 18;
    *p3 = (wp & 0x0000007F);
    if (*p1 < 25 && *p2 < 11) {
      return (4);
    }
  }
  if ((wp & 0x08000000) == 0) { //  # this is a pad
    // PAD:0000.0ccc.ccdd.ddnn.nnnn.vvvv.vvvv.vvvv :: c=col,d=dilo,n=chan,v=value
    *p1 = (wp & 0x07c00000) >> 22;
    *p2 = (wp & 0x003C0000) >> 18;
    *p3 = (wp & 0x0003F000) >> 12;
    *p4 = (wp & 0x00000FFF);
    if (*p1 > 0 && *p1 < 25 && *p2 > 0 && *p2 < 11 && *p3 < 48) {
      return (3);
    }
  } else {
    return (0);
  }
  return (0);
}

bool HmpidDecoder::isRowMarker(int32_t wp, int Eq, int i, int *Err, int *rowSize, int *mark)
{
  if ((wp & 0x0000ffff) == 0x36A8 || (wp & 0x0000ffff) == 0x32A8 || (wp & 0x0000ffff) == 0x30A0
      || (wp & 0x0800ffff) == 0x080010A0) {
    *rowSize = (wp & 0x03ff0000) >> 16;      // # Number of words of row
    *mark = wp & 0x0000ffff;
    *Err = false;
    return (true);
  } else {
    *Err = true;
    return (false);
  }
}

bool HmpidDecoder::isSegmentMarker(int32_t wp, int Eq, int i, int *Err, int *segSize, int *Seg, int *mark)
{
  *Err = false;
  if ((wp & 0xfff00000) >> 20 == 0xAB0) {
    *segSize = (wp & 0x000fff00) >> 8;      // # Number of words of Segment
    *mark = (wp & 0xfff00000) >> 20;
    *Seg = wp & 0x0000000F;

    if (*Seg > 3 || *Seg < 1) {
      ERRO(" E-%d [%d:%08X] > Wrong segment Marker Word, bad Number of segment %d !", Eq, i, wp, *Seg);
      *Err = true;
    }
    return (true);
  } else {
    return (false);
  }
}

bool HmpidDecoder::isPadWord(int32_t wp, int Eq, int i, int *Err, int *Col, int *Dilogic, int *Channel, int *Charge)
{
  *Err = false;
  if ((wp & 0x08000000) == 0) { //  # this is a pad
    // PAD:0000.0ccc.ccdd.ddnn.nnnn.vvvv.vvvv.vvvv :: c=col,d=dilo,n=chan,v=value
    *Col = (wp & 0x07c00000) >> 22;
    *Dilogic = (wp & 0x003C0000) >> 18;
    *Channel = (wp & 0x0003F000) >> 12;
    *Charge = (wp & 0x00000FFF);
    if (*Dilogic > 10 || *Channel > 47) {
      ERRO(" E-%d [%d:%08X] > Wrong Pad values Eq=%d  Col=%d Dil=%d Chan=%d Charge=%d", Eq, i, wp, Eq + 1, *Col,
          *Dilogic, *Channel, *Charge);
      *Err = true;
    }
    return (true);
  } else {
    return (false);
  }
}

bool HmpidDecoder::isEoEmarker(int32_t wp, int Eq, int i, int *Err, int *Col, int *Dilogic, int *Eoesize)
{
  *Err = false;
  // #EX MASK Raul 0x3803FF80  # ex mask 0xF803FF80 - this is EoE marker 0586800B0
  if ((wp & 0x0803FF80) == 0x08000080) {
    *Col = (wp & 0x07c00000) >> 22;
    *Dilogic = (wp & 0x003C0000) >> 18;
    *Eoesize = (wp & 0x0000007F);
    if (*Col > 24 || *Dilogic > 10) {
      ERRO(" E-%d [%d:%08X] > EoE size wrong definition. Col=%d Dil=%d !", Eq, i, wp, *Col, *Dilogic);
      *Err = true;
    }
    return (true);
  } else {
    return (false);
  }
}

bool HmpidDecoder::decodeHmpidError(int ErrorField, char *outbuf)
{
  int res = false;
  outbuf[0] = '\0';
  for (int i = 0; i < MAXHMPIDERRORS; i++) {
    if ((ErrorField & (0x01 << i)) != 0) {
      res = true;
      strcat(outbuf, sHmpidErrorDescription[i]);
    }
  }
  return (res);
}

// read & decode the header
int HmpidDecoder::decodeHeader(int32_t *streamPtrAdr, int *EquipIndex)
{
  int32_t *buffer = streamPtrAdr; // Sets the pointer to buffer

  mHeFEEID = (buffer[0] & 0x000f0000) >> 16;
  mHeSize = (buffer[0] & 0x0000ff00) >> 8;
  mHeVer = (buffer[0] & 0x000000ff);
  mHePrior = (buffer[1] & 0x000000FF);
  mHeDetectorID = (buffer[1] & 0x0000FF00) >> 8;
  mHeOffsetNewPack = (buffer[2] & 0x0000FFFF);
  mHeMemorySize = (buffer[2] & 0xffff0000) >> 16;
  mHeDW = (buffer[3] & 0xF0000000) >> 24;
  mHeCruID = (buffer[3] & 0x0FF0000) >> 16;
  mHePackNum = (buffer[3] & 0x0000FF00) >> 8;
  mHeLinkNum = (buffer[3] & 0x000000FF);
  mHeBCDI = (buffer[4] & 0x00000FFF);
  mHeORBIT = buffer[5];
  mHeTType = buffer[8];
  mHePageNum = (buffer[9] & 0x0000FFFF);
  mHeStop = (buffer[9] & 0x00ff0000) >> 16;
  mHeBusy = (buffer[12] & 0xfffffe00) >> 9;
  mHeFirmwareVersion = buffer[12] & 0x0000000f;
  mHeHmpidError = (buffer[12] & 0x000001F0) >> 4;
  mHePAR = buffer[13] & 0x0000FFFF;

  *EquipIndex = getEquipmentIndex(mHeCruID, mHeLinkNum);
//  mEquipment = (*EquipIndex != -1) ? mTheEquipments[*EquipIndex]->getEquipmentId() : -1;
  mEquipment = mHeFEEID;
  mNumberWordToRead = ((mHeMemorySize - mHeSize) / sizeof(uint32_t));
  mPayloadTail = ((mHeOffsetNewPack - mHeMemorySize) / sizeof(uint32_t));

  // ---- Event ID  : Actualy based on ORBIT NUMBER ...
  mHeEvent = mHeORBIT;

  INFO("FEE-ID=0x%X HeSize=%d HeVer=%d - HePrior=0x%X Det.Id=0x%X - HeMemorySize=%d HeOffsetNewPack=%d\n", mHeFEEID,
      mHeSize, mHeVer, mHePrior, mHeDetectorID, mHeMemorySize, mHeOffsetNewPack);
  INFO("    Equipment=%d - PakCounter=%d Link=%d CruID=0x%X DW=0x%X - BC=%d ORBIT=%d\n", mEquipment, mHePackNum,
      mHeLinkNum, mHeCruID, mHeDW, mHeBCDI, mHeORBIT);
  INFO("    TType=0x%X HeStop=0x%X PagesCounter=%d FirmVersion=0x%X BusyTime=0x%X Error=0x%X PAR=0x%X\n", mHeTType,
      mHeStop, mHePageNum, mHeFirmwareVersion, mHeBusy, mHeHmpidError, mHePAR);
  INFO("--> Payload :  Words to read %d  PailoadTail=%d \n", mNumberWordToRead, mPayloadTail);

  if (*EquipIndex == -1) {
    CRIT("ERROR ! Bad equipment Number: %d /n", mEquipment);
    throw 1;
  }
  return (true);
}

void HmpidDecoder::updateStatistics(HmpidEquipment *eq)
{
  eq->mPadsPerEventAverage = ((eq->mPadsPerEventAverage * (eq->mNumberOfEvents - 1)) + eq->mSampleNumber)
      / (eq->mNumberOfEvents);
  eq->mEventSizeAverage = ((eq->mEventSizeAverage * (eq->mNumberOfEvents - 1)) + eq->mEventSize)
      / (eq->mNumberOfEvents);
  eq->mBusyTimeAverage = ((eq->mBusyTimeAverage * eq->mBusyTimeSamples) + eq->mBusyTimeValue)
      / (++(eq->mBusyTimeSamples));
  if (eq->mSampleNumber == 0)
    eq->mNumberOfEmptyEvents += 1;
  if (eq->mErrorsCounter > 0)
    eq->mNumberOfWrongEvents += 1;
  eq->mTotalPads += eq->mSampleNumber;
  eq->mTotalErrors += eq->mErrorsCounter;
  return;
}

HmpidEquipment* HmpidDecoder::evaluateHeaderContents(int EquipmentIndex)
{
  HmpidEquipment *eq = mTheEquipments[EquipmentIndex];
  if (mHeEvent != eq->mEventNumber) { // Is a new event
    if (eq->mEventNumber != -1) { // skip the first
      updateStatistics(eq); // update previous statistics
    }
    eq->mNumberOfEvents++;
    eq->mEventNumber = mHeEvent;
    eq->mBusyTimeValue = mHeBusy * 0.00000005;
    eq->mEventSize = 0;    // reset the event
    eq->mSampleNumber = 0;
    eq->mErrorsCounter = 0;
  }
  eq->mEventSize += mNumberWordToRead * sizeof(uint32_t); // Calculate the size in bytes
  if (mHeHmpidError != 0) {
    CRIT("HMPID Header reports an error : %d \n", mHeHmpidError);
    dumpHmpidError(mHeHmpidError);
    eq->setError(ERR_HMPID);
  }
  return (eq);
}

// --- read Raw data buffer ...
bool HmpidDecoder::decodeBuffer()
{
  // ---------resets the PAdMap-----------
  for (int i = 0; i < mNumberOfEquipments; i++) {
    mTheEquipments[i]->init();
    mTheEquipments[i]->resetPadMap();
    mTheEquipments[i]->resetErrors();
  }

  int type;
  int equipmentIndex = -1;
  int isIt;
  HmpidEquipment *eq;
  int32_t *streamBuf;
  DEBU("Enter decoding !");
  while (true) {
    try {
      getHeaderFromStream(&streamBuf);
    }
    catch (int e) {
      DEBU("End main decoding loop !");
      break;
    }
    try {
      decodeHeader(streamBuf, &equipmentIndex);
    }
    catch (int e) {
      CRIT("Failed to decode the Header ! %d", 0);
      throw 0;
    }

    eq = evaluateHeaderContents(equipmentIndex);

    int wpprev = 0;
    int wp = 0;
    int newOne = true;
    int p1, p2, p3, p4;
    int error;

    int payIndex = 0;
    while (payIndex < mNumberWordToRead) {  //start the main loop
      if (newOne == true) {
        wpprev = wp;
        if (!getWordFromStream(&wp)) { // end the stream
          break;
        }

        type = checkType(wp, &p1, &p2, &p3, &p4);
        if (type == 0) {
          if (eq->mWillBePad == true) { // try to recover the first pad !
            type = checkType((wp & 0xF7FFFFFF), &p1, &p2, &p3, &p4);
            if (type == 3 && p3 == 0 && eq->mWordsPerDilogicCounter == 0) {
              newOne = false; // # reprocess as pad
              continue;
            }
          }
          eq->setError(ERR_NOTKNOWN);
          ERRO(" E-%d [%d:%08X] %s >%08X< \n", mEquipment, payIndex, wp, sErrorDescription[ERR_NOTKNOWN], wp);
          eq->mWordsPerRowCounter++;
          eq->mWordsPerSegCounter++;
          payIndex++;
          continue;
        }
      }

      if (eq->mWillBeRowMarker == true) { // #shoud be a Row Marker
        if (type == 1) {
          eq->mColumnCounter++;
          eq->mWordsPerSegCounter++;
          eq->mRowSize = p2;
          switch (p2) {
            case 0: // Empty column
              eq->setError(ERR_ROWMARKEMPTY);
              ERRO(" E-%d [%d:%08X] %s column = %d ! %X\n", mEquipment, payIndex, wp,
                  sErrorDescription[ERR_ROWMARKEMPTY], (eq->mSegment) * 8 + eq->mColumnCounter, p1);
              eq->mWillBeRowMarker = true;
              break;
            case 0x3FF: // Error in column
              eq->setError(ERR_ROWMARKERROR);
              ERRO(" E-%d [%d:%08X] %s column = %d ! %X\n", mEquipment, payIndex, wp,
                  sErrorDescription[ERR_ROWMARKERROR], (eq->mSegment) * 8 + eq->mColumnCounter, p1);
              eq->mWillBeRowMarker = true;
              break;
            case 0x3FE: // Masked column
              ERRO(" W-%d [%d:%08X] The column = %d is MASKED ! %X\n", mEquipment, payIndex, wp,
                  (eq->mSegment) * 8 + eq->mColumnCounter, p1);
              eq->mWillBeRowMarker = true;
              break;
            default:
              DEBU(" I-%d [%d:%08X] > Row Marker %09X  row size = %d ,column = %d\n", mEquipment, payIndex, wp, p1, p2,
                  (eq->mSegment) * 8 + eq->mColumnCounter);
              eq->mWillBeRowMarker = false;
              eq->mWillBePad = true;
              break;
          }
          newOne = true;
        } else {
          if (wp == wpprev) {
            eq->setError(ERR_DUPLICATEPAD);
            ERRO(" E-%d [%d:%08X] %s column = %d ! %X\n", mEquipment, payIndex, wp, sErrorDescription[ERR_DUPLICATEPAD],
                (eq->mSegment) * 8 + eq->mColumnCounter, p1);
            newOne = true;
          } else if (type == 4) { // # Could be a EoE
            eq->mColumnCounter++;
            eq->setError(ERR_ROWMARKWRONG);
            eq->mWillBeRowMarker = false;
            eq->mWillBePad = true;
            newOne = true;
          } else if (type == 3) { //# Could be a PAD
            eq->mColumnCounter++;
            eq->setError(ERR_ROWMARKLOST);
            ERRO(" E-%d [%d:%08X] %s column = %d ! %X\n", mEquipment, payIndex, wp, sErrorDescription[ERR_ROWMARKLOST],
                (eq->mSegment) * 8 + eq->mColumnCounter, p1);
            eq->mWillBeRowMarker = false;
            eq->mWillBePad = true;
            newOne = true;
          } else if (type == 2) { // # Could be a EoS
            eq->mWillBeRowMarker = false;
            eq->mWillBeSegmentMarker = true;
            newOne = false;
          } else {
            eq->mColumnCounter++;
            eq->setError(ERR_ROWMARKLOST);
            ERRO(" E-%d [%d:%08X] %s column = %d ! %X\n", mEquipment, payIndex, wp, sErrorDescription[ERR_ROWMARKLOST],
                (eq->mSegment) * 8 + eq->mColumnCounter, p1);
            eq->mWillBeRowMarker = false;
            eq->mWillBePad = true;
            newOne = true;
          }
        }
      } else if (eq->mWillBePad == true) { // # We expect a pad
        //# PAD:0000.0ccc.ccdd.ddnn.nnnn.vvvv.vvvv.vvvv :: c=col,d=dilo,n=chan,v=value
        //   c = 1..24   d = 1..10  n = 0..47
        if (type == 3) {
          newOne = true;
          if (wp == wpprev) {
            eq->setError(ERR_DUPLICATEPAD);
            ERRO(" E-%d [%d:%08X] %s column = %d ! %X\n", mEquipment, payIndex, wp, sErrorDescription[ERR_DUPLICATEPAD],
                (eq->mSegment) * 8 + eq->mColumnCounter, p1);
          } else if (p1 != (eq->mSegment * 8 + eq->mColumnCounter)) {        // # Manage
            // We try to recover the RowMarker misunderstanding
            isIt = isRowMarker(wp, mEquipment, payIndex, &error, &p2, &p1);
            if (isIt == true && error == false) {
              type = 1;
              newOne = false;              // # reprocess as pad
              eq->mWillBeEoE = true;
              eq->mWillBePad = false;
            } else {
              WARN(" W-%d [%d:%08X] > Mismatch in column %d => %d ! %X\n", mEquipment, payIndex, wp, p1,
                  (eq->mSegment) * 8 + eq->mColumnCounter, p1);
              eq->mColumnCounter = p1 % 8;
            }
          } else {
            DEBU(" I-%d [%d:%08X] > Pad Eq=%d  Col=%d Dil=%d Chan=%d Charge=%d\n", mEquipment, payIndex, wp, mEquipment,
                p1, p2, p3, p4);
            setPad(eq, p1 - 1, p2 - 1, p3, p4);
            eq->mWordsPerDilogicCounter++;
            eq->mSampleNumber++;
            if (p3 == 47) {
              eq->mWillBeEoE = true;
              eq->mWillBePad = false;
            }
          }
          eq->mWordsPerRowCounter++;
          eq->mWordsPerSegCounter++;
        } else if (type == 4) { //# the pads are end ok
          eq->mWillBeEoE = true;
          eq->mWillBePad = false;
          newOne = false;
        } else if (type == 1) { // # We Lost the EoE !
          // We try to recover the PAD misunderstanding
          isIt = isPadWord(wp, mEquipment, payIndex, &error, &p1, &p2, &p3, &p4);
          if (isIt == true && error == false) {
            type = 3;
            newOne = false;            // # reprocess as pad
          } else {
            eq->setError(ERR_LOSTEOEMARK);
            ERRO(" E-%d [%d:%08X] %s column = %d ! %X\n", mEquipment, payIndex, wp, sErrorDescription[ERR_LOSTEOEMARK],
                (eq->mSegment) * 8 + eq->mColumnCounter, p1);
            eq->mWillBeRowMarker = true;
            eq->mWillBePad = false;
            newOne = false;
          }
        } else if (type == 2) {            // # We Lost the EoE !
          eq->setError(ERR_LOSTEOEMARK);
          ERRO(" E-%d [%d:%08X] %s column = %d ! %X\n", mEquipment, payIndex, wp, sErrorDescription[ERR_LOSTEOEMARK],
              (eq->mSegment) * 8 + eq->mColumnCounter, p1);
          eq->mWillBeSegmentMarker = true;
          eq->mWillBePad = false;
          newOne = false;
        }
      } else if (eq->mWillBeEoE == true) {            // # We expect a EoE
        if (type == 4) {
          eq->mWordsPerRowCounter++;
          eq->mWordsPerSegCounter++;
          if (wpprev == wp) {
            eq->setError(ERR_DOUBLEEOEMARK);
            ERRO(" E-%d [%d:%08X] %s Col=%d Dil=%d  Expected size=%d, Counted size=%d !\n", mEquipment, payIndex, wp,
                sErrorDescription[ERR_DOUBLEEOEMARK], p1, p2, p3, eq->mWordsPerDilogicCounter);
          } else if (p3 != eq->mWordsPerDilogicCounter) {
            eq->setError(ERR_WRONGSIZEINEOE);
            ERRO(" E-%d [%d:%08X] %s Col=%d Dil=%d  Expected size=%d, Counted size=%d !\n", mEquipment, payIndex, wp,
                sErrorDescription[ERR_WRONGSIZEINEOE], p1, p2, p3, eq->mWordsPerDilogicCounter);
          } else {
            DEBU(" I-%d [%d:%08X] > EoE Marker %X Col=%d Dil=%d Size=%d\n", mEquipment, payIndex, wp, wp, p1, p2, p3);
          }
          eq->mWordsPerDilogicCounter = 0;
          if (p2 == 10) {
            if (p1 % 8 != 0) {            // # we expect the Row Marker
              eq->mWillBeRowMarker = true;
            } else {
              eq->mWillBeSegmentMarker = true;
            }
          } else {
            eq->mWillBePad = true;
          }
          eq->mWillBeEoE = false;
          newOne = true;
        } else if (type == 2) {            // We Lost the EoE !
          eq->setError(ERR_LOSTEOEMARK);
          ERRO(" E-%d [%d:%08X] %s (1) column = %d ! %X\n", mEquipment, payIndex, wp,
              sErrorDescription[ERR_LOSTEOEMARK], (eq->mSegment) * 8 + eq->mColumnCounter, p1);
          eq->mWillBeSegmentMarker = true;
          eq->mWillBeEoE = false;
          newOne = false;
        } else if (type == 1) { //# We Lost the EoE !
          eq->setError(ERR_LOSTEOEMARK);
          ERRO(" E-%d [%d:%08X] %s (2) column = %d ! %X\n", mEquipment, payIndex, wp,
              sErrorDescription[ERR_LOSTEOEMARK], (eq->mSegment) * 8 + eq->mColumnCounter, p1);
          eq->mWillBeRowMarker = true;
          eq->mWillBeEoE = false;
          newOne = false;
        } else if (type == 3) { // # We Lost the EoE !
          int typb, p1b, p2b, p3b, p4b;
          typb = checkType((wp | 0x08000000), &p1b, &p2b, &p3b, &p4b);
          if (typb == 4 && p3b == 48) {
            type = typb;
            p1 = p1b;
            p2 = p2b;
            p3 = p3b;
            p4 = p4b;
            newOne = false; // # reprocess as EoE
          } else {
            eq->setError(ERR_LOSTEOEMARK);
            ERRO(" E-%d [%d:%08X] %s (3) column = %d ! %X\n", mEquipment, payIndex, wp,
                sErrorDescription[ERR_LOSTEOEMARK], (eq->mSegment) * 8 + eq->mColumnCounter, p1);
            eq->mWillBePad = true;
            eq->mWillBeEoE = false;
            newOne = false;
          }
        }
      } else if (eq->mWillBeSegmentMarker == true) { // # We expect a EoSegment
        if (wpprev == wp) {
          eq->setError(ERR_DOUBLEMARKWORD);
          ERRO(" E-%d [%d:%08X] %s Col=%d Dil=%d  Expected size=%d, Counted size=%d !\n", mEquipment, payIndex, wp,
              sErrorDescription[ERR_DOUBLEMARKWORD], (eq->mSegment) * 8 + eq->mColumnCounter, p1, p2,
              eq->mWordsPerDilogicCounter);
          newOne = true;
        } else if (type == 2) {
          if (abs(eq->mWordsPerSegCounter - p2) > 5) {
            eq->setError(ERR_WRONGSIZESEGMENTMARK);
            ERRO(" E-%d [%d:%08X] %s exp %d found %d !  Seg=%d\n", mEquipment, payIndex, wp,
                sErrorDescription[ERR_WRONGSIZESEGMENTMARK], p2, eq->mWordsPerSegCounter, p3);
          } else {
            DEBU(" I-%d [%d:%08X] > Seg Marker=%09X Seg=%d Size=%d\n", mEquipment, payIndex, wp, p1, p3, p2);
          }
          eq->mWordsPerSegCounter = 0;
          eq->mWordsPerRowCounter = 0;
          eq->mColumnCounter = 0;
          eq->mSegment = p3 % 3;
          eq->mWillBeRowMarker = true;
          eq->mWillBeSegmentMarker = false;
          newOne = true;
        } else {
          eq->setError(ERR_LOSTEOSMARK);
          ERRO(" E-%d [%d:%08X] %s column = %d ! %X\n", mEquipment, payIndex, wp, sErrorDescription[ERR_LOSTEOSMARK],
              (eq->mSegment) * 8 + eq->mColumnCounter, p1);
          eq->mWillBeSegmentMarker = false;
          eq->mWillBeRowMarker = true;
          newOne = false;
        }
      }
      if (newOne) {
        payIndex += 1;
      }
    }
    for (int i = 0; i < mPayloadTail; i++) { // move the pointer to skip the Paiload Tail
      getWordFromStream(&wp);
    }
  } // this is the end of stream

  for (int i = 0; i < mNumberOfEquipments; i++) {
    if (mTheEquipments[i]->mNumberOfEvents > 0) {
      updateStatistics(mTheEquipments[i]);
    }
  }
  return (true);
}

/*
// =========================================================
// Functions to translate coordinates
// Digit coordinates " Mod,Row,Col := Mod = {0..6}  Row = {0..159}  Col = {0..143}
//                    (0,0) Left Bottom
//
// Hardware coordinates  Equ,Col,Dil,Cha := Equ = {0..13}  Col = {0..23}  Dil = {0..9}  Cha = {0..47}
//
//                    (0,0,0,0) Right Top   (1,0,0,0) Left Bottom
//
void HmpidDecoder::convertCoords(int Mod, int Col, int Row, int *Equi, int *Colu, int *Dilo, int *Chan)
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

void HmpidDecoder::getCoords(int Equi, int Colu, int Dilo, int Chan, int *Mod, int *Col, int *Row)
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
*/
// =========================================================

// Getter methods to extract Statistic Data in Digit Coords
//
uint16_t HmpidDecoder::getPadSamples(int Module, int Column, int Row)
{
  int e, c, d, h;
  hmpidCoordsModule2Equipment(Module, Column, Row, &e, &c, &d, &h);
  int EqInd = getEquipmentIndex(e);
  if (EqInd < 0)
    return (0);
  return (mTheEquipments[EqInd]->mPadSamples[c][d][h]);
}

double HmpidDecoder::getPadSum(int Module, int Column, int Row)
{
  int e, c, d, h;
  hmpidCoordsModule2Equipment(Module, Column, Row, &e, &c, &d, &h);
  int EqInd = getEquipmentIndex(e);
  if (EqInd < 0)
    return (0);
  return (mTheEquipments[EqInd]->mPadSum[c][d][h]);
}

double HmpidDecoder::getPadSquares(int Module, int Column, int Row)
{
  int e, c, d, h;
  hmpidCoordsModule2Equipment(Module, Column, Row, &e, &c, &d, &h);
  int EqInd = getEquipmentIndex(e);
  if (EqInd < 0)
    return (0);
  return (mTheEquipments[EqInd]->mPadSquares[c][d][h]);
}

// Getter methods to extract Statistic Data in Hardware Coords
//
uint16_t HmpidDecoder::getChannelSamples(int EquipmId, int Column, int Dilogic, int Channel)
{
  int EqInd = getEquipmentIndex(EquipmId);
  if (EqInd < 0)
    return (0);
  return (mTheEquipments[EqInd]->mPadSamples[Column][Dilogic][Channel]);
}

double HmpidDecoder::getChannelSum(int EquipmId, int Column, int Dilogic, int Channel)
{
  int EqInd = getEquipmentIndex(EquipmId);
  if (EqInd < 0)
    return (0);
  return (mTheEquipments[EqInd]->mPadSum[Column][Dilogic][Channel]);
}

double HmpidDecoder::getChannelSquare(int EquipmId, int Column, int Dilogic, int Channel)
{
  int EqInd = getEquipmentIndex(EquipmId);
  if (EqInd < 0)
    return (0);
  return (mTheEquipments[EqInd]->mPadSquares[Column][Dilogic][Channel]);
}

// Gets the Event Size Average value
float HmpidDecoder::getAverageEventSize(int EquipmId)
{
  int EqInd = getEquipmentIndex(EquipmId);
  if (EqInd < 0)
    return (0.0);
  return (mTheEquipments[EqInd]->mEventSizeAverage);
}

// Gets the Busy Time Average value
float HmpidDecoder::getAverageBusyTime(int EquipmId)
{
  int EqInd = getEquipmentIndex(EquipmId);
  if (EqInd < 0)
    return (0.0);
  return (mTheEquipments[EqInd]->mBusyTimeAverage);
}

// ===================================================
// Methods to dump info
//
void HmpidDecoder::dumpErrors(int EquipmId)
{
  int EqInd = getEquipmentIndex(EquipmId);
  if (EqInd < 0)
    return;

  std::cout << "Dump Errors for the Equipment = " << EquipmId << std::endl;
  for (int i = 0; i < MAXERRORS; i++) {
    std::cout << sErrorDescription[i] << "  = " << mTheEquipments[EqInd]->mErrors[i] << std::endl;
  }
  std::cout << " -------- " << std::endl;
  return;
}

// Dumps a Table of stat info
// type := 0 = Samples, 1 = Sum, 2 = Sum of squares
//
void HmpidDecoder::dumpPads(int EquipmId, int type)
{
  int EqInd = getEquipmentIndex(EquipmId);
  if (EqInd < 0)
    return;

  int Module = EquipmId / 2;
  int StartRow = (EquipmId % 2 == 1) ? 80 : 0;
  int EndRow = (EquipmId % 2 == 1) ? 160 : 80;
  std::cout << "Dump Pads for the Equipment = " << EquipmId << std::endl;
  for (int c = 0; c < 144; c++) {
    for (int r = StartRow; r < EndRow; r++) {
      switch (type) {
        case 0:
          std::cout << getPadSamples(Module, c, r) << ",";
          break;
        case 1:
          std::cout << getPadSum(Module, c, r) << ",";
          break;
        case 2:
          std::cout << getPadSquares(Module, c, r) << ",";
          break;
      }
    }
    std::cout << std::endl;
  }
  std::cout << " -------- " << std::endl;
  return;
}

void HmpidDecoder::dumpHmpidError(int ErrorField)
{
  char printbuf[512];
  if (decodeHmpidError(ErrorField, printbuf) == true) {
    std::cout << "HMPID Error field =" << ErrorField << " : " << printbuf << std::endl;
  }
  return;
}

void HmpidDecoder::writeSummaryFile(char *summaryFileName)
{
  FILE *fs = fopen(summaryFileName, "w");
  if (fs == 0) {
    printf("Error opening the file %s !\n", summaryFileName);
    throw 2;
  }

  fprintf(fs, "HMPID Readout Raw Data Decoding Summary File\n");
  fprintf(fs, "Equipment Id\t");
  for (int i = 0; i < MAXEQUIPMENTS; i++)
    fprintf(fs, "%d\t", mTheEquipments[i]->getEquipmentId());
  fprintf(fs, "\n");

  fprintf(fs, "Number of events\t");
  for (int i = 0; i < MAXEQUIPMENTS; i++)
    fprintf(fs, "%d\t", mTheEquipments[i]->mNumberOfEvents);
  fprintf(fs, "\n");

  fprintf(fs, "Average Event Size\t");
  for (int i = 0; i < MAXEQUIPMENTS; i++)
    fprintf(fs, "%f\t", mTheEquipments[i]->mEventSizeAverage);
  fprintf(fs, "\n");

  fprintf(fs, "Total pads\t");
  for (int i = 0; i < MAXEQUIPMENTS; i++)
    fprintf(fs, "%d\t", mTheEquipments[i]->mTotalPads);
  fprintf(fs, "\n");

  fprintf(fs, "Average pads per event\t");
  for (int i = 0; i < MAXEQUIPMENTS; i++)
    fprintf(fs, "%f\t", mTheEquipments[i]->mPadsPerEventAverage);
  fprintf(fs, "\n");

  fprintf(fs, "Busy Time average\t");
  for (int i = 0; i < MAXEQUIPMENTS; i++)
    fprintf(fs, "%e\t", mTheEquipments[i]->mBusyTimeAverage);
  fprintf(fs, "\n");

  fprintf(fs, "Event rate\t");
  for (int i = 0; i < MAXEQUIPMENTS; i++)
    fprintf(fs, "%e\t", 1 / mTheEquipments[i]->mBusyTimeAverage);
  fprintf(fs, "\n");

  fprintf(fs, "Number of Empty Events\t");
  for (int i = 0; i < MAXEQUIPMENTS; i++)
    fprintf(fs, "%d\t", mTheEquipments[i]->mNumberOfEmptyEvents);
  fprintf(fs, "\n");

  fprintf(fs, "-------------Errors--------------------\n");
  fprintf(fs, "Wrong events\t");
  for (int i = 0; i < MAXEQUIPMENTS; i++)
    fprintf(fs, "%d\t", mTheEquipments[i]->mNumberOfWrongEvents);
  fprintf(fs, "\n");

  for (int j = 0; j < MAXERRORS; j++) {
    fprintf(fs, "%s\t", sErrorDescription[j]);
    for (int i = 0; i < MAXEQUIPMENTS; i++)
      fprintf(fs, "%d\t", mTheEquipments[i]->mErrors[j]);
    fprintf(fs, "\n");
  }

  fprintf(fs, "Total errors\t");
  for (int i = 0; i < MAXEQUIPMENTS; i++)
    fprintf(fs, "%d\t", mTheEquipments[i]->mTotalErrors);
  fprintf(fs, "\n");

  fclose(fs);
  return;
}
