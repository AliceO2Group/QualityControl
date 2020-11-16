/*
 * HmpidDecodeRawFile.cpp
 *
 *  Created on: 24 set 2020
 *      Author: fap
 */

//#include "HmpidEquipments.h"
//#include "HmpidDecoder.h"
#include "HMPID/HmpidDecodeRawMem.h"

using namespace Equipment;

HmpidDecodeRawMem::HmpidDecodeRawMem(int numOfEquipments)
    :
    HmpidDecoder(numOfEquipments)
{
}
HmpidDecodeRawMem::HmpidDecodeRawMem(int *EqIds, int *CruIds, int *LinkIds, int numOfEquipments)
    :
    HmpidDecoder(EqIds, CruIds, LinkIds, numOfEquipments)
{
}
HmpidDecodeRawMem::~HmpidDecodeRawMem()
{
}

bool HmpidDecodeRawMem::setUpStream(void *Buffer, long BufferLen)
{
  long wordsBufferLen = BufferLen / (sizeof(int32_t) / sizeof(char)); // Converts the len in words
  if (Buffer == nullptr) {
    CRIT("Raw data buffer null Pointer ! %d \n", 0);
    throw 4;
  }
  if (wordsBufferLen == 0) {
    CRIT("Raw data buffer Empty ! %ld /n", wordsBufferLen);
    throw 5;
  }
  if (wordsBufferLen < 16) {
    CRIT("Raw data buffer less then the Header Dimension = %ld! \n", wordsBufferLen);
    throw 6;
  }

  mActualStreamPtr = (int32_t*) Buffer; // sets the pointer to the Buffer
  mEndStreamPtr = ((int32_t*) Buffer) + wordsBufferLen; //sets the End of buffer
  mStartStreamPtr = ((int32_t*) Buffer);

  return (true);
}

bool HmpidDecodeRawMem::getBlockFromStream(int32_t **streamPtr, uint32_t Size)
{
  *streamPtr = mActualStreamPtr;
  mActualStreamPtr += Size;
  if (mActualStreamPtr > mEndStreamPtr)
    throw 1;
  return (true);
}

bool HmpidDecodeRawMem::getHeaderFromStream(int32_t **streamPtr)
{
  return (getBlockFromStream(streamPtr, 16));
}

bool HmpidDecodeRawMem::getWordFromStream(int32_t *word)
{
  int32_t *appo;
  *word = *mActualStreamPtr;
  return (getBlockFromStream(&appo, 1));
}

// col = 0..23  dil = 0..9  ch = 0..47
void HmpidDecodeRawMem::setPad(HmpidEquipment *eq, int col, int dil, int ch, int charge)
{
  eq->setPad(col, dil, ch, charge);
  return;
}

