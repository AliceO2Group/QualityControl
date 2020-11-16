/*
 * HmpidDecodeRawFile.h
 *
 *  Created on: 24 set 2020
 *      Author: fap
 */

#ifndef COMMON_HMPIDDECODERAWMEM_H_
#define COMMON_HMPIDDECODERAWMEM_H_

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>

#include "HMPID/HmpidDecoder.h"

using namespace Equipment;

class HmpidDecodeRawMem: public HmpidDecoder
{
  public:
    HmpidDecodeRawMem(int *EqIds, int *CruIds, int *LinkIds, int numOfEquipments);
    HmpidDecodeRawMem(int numOfEquipments);
    ~HmpidDecodeRawMem();

    bool setUpStream(void *Buffer, long BufferLen);

  private:
    bool getBlockFromStream(int32_t **streamPtr, uint32_t Size);
    bool getHeaderFromStream(int32_t **streamPtr);
    bool getWordFromStream(int32_t *word);
    void setPad(HmpidEquipment *eq, int col, int dil, int ch, int charge);

  private:

};

#endif /* COMMON_HMPIDDECODERAWFILE_H_ */
