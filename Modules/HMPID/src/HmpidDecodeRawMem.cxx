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
/// \file   HmpidDecodeRawMem.cxx
/// \author Antonio Franco - INFN Bari
/// \brief Derived Class for decoding Raw Data Memory stream
/// \version 1.0
/// \date 24 set 2020

/* ------ HISTORY ---------
*/

#include "HMPID/HmpidDecodeRawMem.h"

using namespace o2::quality_control_modules::hmpid;

/// Constructor : accepts the number of equipments to define
///               The mapping is the default at P2
///               Allocates instances for all defined equipments
///               normally it is equal to 14
/// @param[in] numOfEquipments : the number of equipments to define [1..14]
HmpidDecodeRawMem::HmpidDecodeRawMem(int numOfEquipments)
  : HmpidDecoder(numOfEquipments)
{
}

/// Constructor : accepts the number of equipments to define
///               and their complete address map
///               Allocates instances for all defined equipments
///
///  The Address map is build from three array
/// @param[in] numOfEquipments : the number of equipments to define [1..14]
/// @param[in] *EqIds : the pointer to the Equipments ID array
/// @param[in] *CruIds : the pointer to the CRU ID array
/// @param[in] *LinkIds : the pointer to the Link ID array
HmpidDecodeRawMem::HmpidDecodeRawMem(int* EqIds, int* CruIds, int* LinkIds, int numOfEquipments)
  : HmpidDecoder(EqIds, CruIds, LinkIds, numOfEquipments)
{
}

/// Destructor
HmpidDecodeRawMem::~HmpidDecodeRawMem()
{
}

/// Setup the Input Stream with a Memory Pointer
/// the buffer length is in byte, some controls are done
///
/// @param[in] *Buffer : the pointer to Memory buffer
/// @param[in] BufferLen : the length of the buffer (bytes)
/// @returns True if the stream is set
/// @throws TH_NULLBUFFERPOINTER Thrown if the pointer to the buffer is NULL
/// @throws TH_BUFFEREMPTY Thrown if the buffer is empty
/// @throws TH_WRONGBUFFERDIM Thrown if the buffer len is less then one header
bool HmpidDecodeRawMem::setUpStream(void* Buffer, long BufferLen)
{
  long wordsBufferLen = BufferLen / (sizeof(int32_t) / sizeof(char)); // Converts the len in words
  if (Buffer == nullptr) {
    ILOG(Error) << "Raw data buffer null Pointer ! " << ENDM;
    throw TH_NULLBUFFERPOINTER;
  }
  if (wordsBufferLen == 0) {
    ILOG(Error) << "Raw data buffer Empty ! " << ENDM;
    throw TH_BUFFEREMPTY;
  }
  if (wordsBufferLen < 16) {
    ILOG(Error) << "Raw data buffer less then the Header Dimension = " << wordsBufferLen << ENDM;
    throw TH_WRONGBUFFERDIM;
  }

  mActualStreamPtr = (int32_t*)Buffer;                 // sets the pointer to the Buffer
  mEndStreamPtr = ((int32_t*)Buffer) + wordsBufferLen; //sets the End of buffer
  mStartStreamPtr = ((int32_t*)Buffer);

  return (true);
}

/// Gets a sized chunk from the stream. The stream pointers members are updated
/// @param[in] **streamPtr : the pointer to the memory buffer
/// @param[in] Size : the dimension of the chunk (words)
/// @returns True every time
/// @throw TH_WRONGBUFFERDIM Buffer length shorter then the requested
bool HmpidDecodeRawMem::getBlockFromStream(int32_t** streamPtr, uint32_t Size)
{
  *streamPtr = mActualStreamPtr;
  mActualStreamPtr += Size;
  if (mActualStreamPtr > mEndStreamPtr)
    throw TH_WRONGBUFFERDIM;
  return (true);
}

/// Gets the Header Block from the stream.
/// @param[in] **streamPtr : the pointer to the memory buffer
/// @returns True if the header is read
bool HmpidDecodeRawMem::getHeaderFromStream(int32_t** streamPtr)
{
  return (getBlockFromStream(streamPtr, 16));
}

/// Gets a Word from the stream.
/// @param[in] *word : the buffer for the read word
/// @returns True if the operation end well
bool HmpidDecodeRawMem::getWordFromStream(int32_t* word)
{
  int32_t* appo;
  *word = *mActualStreamPtr;
  return (getBlockFromStream(&appo, 1));
}

/// -----   Sets the Pad ! ------
/// this is an overloaded method. In this version the value of the charge
/// is used to update the statistical matrix of the base class
///
/// @param[in] *eq : the pointer to the Equipment object
/// @param[in] col : the column [0..23]
/// @param[in] dil : the dilogic [0..9]
/// @param[in] ch : the channel [0..47]
/// @param[in] charge : the value of the charge
void HmpidDecodeRawMem::setPad(HmpidEquipment* eq, int col, int dil, int ch, int charge)
{
  eq->setPad(col, dil, ch, charge);
  return;
}
