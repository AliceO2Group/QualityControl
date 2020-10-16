///
/// \file   Decoding.cxx
/// \author Andrea Ferrero
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "Headers/RAWDataHeader.h"
#include "DetectorsRaw/RDHUtils.h"
#include "QualityControl/QcInfoLogger.h"
#include "MCH/Decoding.h"
#include "MCHBase/Digit.h"
#define __STDC_FORMAT_MACROS
#include <cinttypes>
#include <fstream>

using namespace std;

static int gPattern;
static int gNbErrors;
static int gNbWarnings;

struct CRUheader {
  uint8_t header_version;
  uint8_t header_size;
  uint16_t block_length;
  uint16_t fee_id;
  uint8_t priority_bit;
  uint8_t reserved_1;
  uint16_t next_packet_offset;
  uint16_t memory_size;
  uint8_t link_id;
  uint8_t packet_counter;
  uint16_t cru_id : 12;
  uint8_t dpw_id : 4;
  uint32_t hb_orbit;
  //uint16_t cru_id;
  //uint8_t dummy1;
  //uint64_t dummy2;
};

enum decode_state_t {
  DECODE_STATE_UNKNOWN,
  DECODE_STATE_SYNC_FOUND,
  DECODE_STATE_HEADER_FOUND,
  DECODE_STATE_CSIZE_FOUND,
  DECODE_STATE_CTIME_FOUND,
  DECODE_STATE_SAMPLE_FOUND,
  DECODE_STATE_END_OF_CLUSTER,
  DECODE_STATE_END_OF_PACKET
};

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

using RDH = o2::header::RDHAny;

bool BXCNT_compare(long int c1, long int c2)
{
  return true;
  const int64_t MAX = 0xFFFFF;
  //int64_t diff = c1 - c2;
  //if(diff >= MAX) diff -= MAX;
  //if(diff <= -MAX) diff += MAX;
  int64_t c1p = (c1 + 1) & MAX;
  int64_t c2p = (c2 + 1) & MAX;
  if (c1 == c2)
    return true;
  if (c1p == c2)
    return true;
  if (c2p == c1)
    return true;
  return false;
}

void DualSampaInit(DualSampa* ds)
{
  ds->status = notSynchronized;
  ds->data = 0;
  ds->bit = 0;
  ds->powerMultiplier = 1;
  ds->nsyn2Bits = 0;
  ds->bxc[0] = ds->bxc[1] = -1;
  ds->chan_addr[0] = 0;
  ds->chan_addr[1] = 0;
  for (int j = 0; j < 2; j++) {
    for (int k = 0; k < 32; k++) {
      ds->ndata[j][k] = 0;
      ds->nclus[j][k] = 0;
      ds->pedestal[j][k] = 0;
      ds->noise[j][k] = 0;
      ds->delta[j][k] = 0;
    }
  }
}

void DualSampaReset(DualSampa* ds)
{
  ds->status = notSynchronized;
  ds->data = 0;
  ds->bit = 0;
  ds->powerMultiplier = 1;
  ds->nsyn2Bits = 0;
  ds->bxc[0] = ds->bxc[1] = -1;
  ds->chan_addr[0] = 0;
  ds->chan_addr[1] = 0;
}

void DualSampaGroupInit(DualSampaGroup* dsg)
{
  dsg->bxc = -1;
}

void DualSampaGroupReset(DualSampaGroup* dsg)
{
  dsg->bxc = -1;
}

int CheckDataParity(uint64_t data)
{
  //    int parity,bit;
  //    parity = data&0x1;
  //    for (int i = 1; i <= 50; i++)
  //    {
  //        bit = (data>>i)&0x1;
  //        //        parity = (parity || bit) && (!(parity && bit));
  //        parity += bit; // Count the number of bits = 1
  //    }
  int parity, bit;
  parity = data & 0x1;
  for (int i = 1; i < 50; i++) {
    bit = (data >> i) & 0x1;
    parity = (parity || bit) && (!(parity && bit)); // XOR of all bits
  }
  return parity;
}

void HammingDecode(unsigned int buffer[2], bool& error, bool& uncorrectable, bool fix_data)
//
// From Arild Velure code
//
{

  // header split
  bool parityreceived[6];
  bool data_in[43];
  bool overallparity;

  for (int i = 0; i < 6; i++)
    parityreceived[i] = (buffer[0] >> i) & 0x1;
  overallparity = (buffer[0] >> 6) & 0x1;
  //for (int i = 0; i < 43; i++)
  //  data_in[i] = (header_in >> (i + 7)) & 0x1;
  for (int i = 7; i < 30; i++)
    data_in[i - 7] = (buffer[0] >> i) & 0x1;
  for (int i = 30; i < 50; i++)
    data_in[i - 7] = (buffer[1] >> (i - 30)) & 0x1;

  //calculated values
  bool corrected_out[43];
  bool overallparitycalc = 0;
  bool overallparity_out = 0;
  bool paritycalc[6];
  bool paritycorreced_out[6];

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // calculate parity
  paritycalc[0] = data_in[0] ^ data_in[1] ^ data_in[3] ^ data_in[4] ^ data_in[6] ^
                  data_in[8] ^ data_in[10] ^ data_in[11] ^ data_in[13] ^ data_in[15] ^
                  data_in[17] ^ data_in[19] ^ data_in[21] ^ data_in[23] ^ data_in[25] ^
                  data_in[26] ^ data_in[28] ^ data_in[30] ^ data_in[32] ^ data_in[34] ^
                  data_in[36] ^ data_in[38] ^ data_in[40] ^ data_in[42];

  paritycalc[1] = data_in[0] ^ data_in[2] ^ data_in[3] ^ data_in[5] ^ data_in[6] ^
                  data_in[9] ^ data_in[10] ^ data_in[12] ^ data_in[13] ^ data_in[16] ^
                  data_in[17] ^ data_in[20] ^ data_in[21] ^ data_in[24] ^ data_in[25] ^
                  data_in[27] ^ data_in[28] ^ data_in[31] ^ data_in[32] ^ data_in[35] ^
                  data_in[36] ^ data_in[39] ^ data_in[40];

  paritycalc[2] = data_in[1] ^ data_in[2] ^ data_in[3] ^ data_in[7] ^ data_in[8] ^
                  data_in[9] ^ data_in[10] ^ data_in[14] ^ data_in[15] ^ data_in[16] ^
                  data_in[17] ^ data_in[22] ^ data_in[23] ^ data_in[24] ^ data_in[25] ^
                  data_in[29] ^ data_in[30] ^ data_in[31] ^ data_in[32] ^ data_in[37] ^
                  data_in[38] ^ data_in[39] ^ data_in[40];

  paritycalc[3] = data_in[4] ^ data_in[5] ^ data_in[6] ^ data_in[7] ^ data_in[8] ^
                  data_in[9] ^ data_in[10] ^ data_in[18] ^ data_in[19] ^ data_in[20] ^
                  data_in[21] ^ data_in[22] ^ data_in[23] ^ data_in[24] ^ data_in[25] ^
                  data_in[33] ^ data_in[34] ^ data_in[35] ^ data_in[36] ^ data_in[37] ^
                  data_in[38] ^ data_in[39] ^ data_in[40];

  paritycalc[4] = data_in[11] ^ data_in[12] ^ data_in[13] ^ data_in[14] ^ data_in[15] ^
                  data_in[16] ^ data_in[17] ^ data_in[18] ^ data_in[19] ^ data_in[20] ^
                  data_in[21] ^ data_in[22] ^ data_in[23] ^ data_in[24] ^ data_in[25] ^
                  data_in[41] ^ data_in[42];

  paritycalc[5] = data_in[26] ^ data_in[27] ^ data_in[28] ^ data_in[29] ^ data_in[30] ^
                  data_in[31] ^ data_in[32] ^ data_in[33] ^ data_in[34] ^ data_in[35] ^
                  data_in[36] ^ data_in[37] ^ data_in[38] ^ data_in[39] ^ data_in[40] ^
                  data_in[41] ^ data_in[42];
  ////////////////////////////////////////////////////////////////////////////////////////////////

  //    uint8_t syndrome = 0;
  unsigned char syndrome = 0;

  for (int i = 0; i < 6; i++)
    syndrome |= (paritycalc[i] ^ parityreceived[i]) << i;

  bool data_parity_interleaved[64];
  bool syndromeerror;

  //data_parity_interleaved[0]          =  0;
  data_parity_interleaved[1] = parityreceived[0];
  data_parity_interleaved[2] = parityreceived[1];
  data_parity_interleaved[3] = data_in[0];
  data_parity_interleaved[4] = parityreceived[2];
  for (int i = 1; i <= 3; i++)
    data_parity_interleaved[i + 5 - 1] = data_in[i];
  data_parity_interleaved[8] = parityreceived[3];
  for (int i = 4; i <= 10; i++)
    data_parity_interleaved[i + 9 - 4] = data_in[i];
  data_parity_interleaved[16] = parityreceived[4];
  for (int i = 11; i <= 25; i++)
    data_parity_interleaved[i + 17 - 11] = data_in[i];
  data_parity_interleaved[32] = parityreceived[5];
  for (int i = 26; i <= 42; i++)
    data_parity_interleaved[i + 33 - 26] = data_in[i];
  //for (int i = 50; i <= 63; i++)
  //  data_parity_interleaved[i]        =  0;

  data_parity_interleaved[syndrome] = !data_parity_interleaved[syndrome]; // correct the interleaved

  paritycorreced_out[0] = data_parity_interleaved[1];
  paritycorreced_out[1] = data_parity_interleaved[2];
  corrected_out[0] = data_parity_interleaved[3];
  paritycorreced_out[2] = data_parity_interleaved[4];
  for (int i = 1; i <= 3; i++)
    corrected_out[i] = data_parity_interleaved[i + 5 - 1];
  paritycorreced_out[3] = data_parity_interleaved[8];
  for (int i = 4; i <= 10; i++)
    corrected_out[i] = data_parity_interleaved[i + 9 - 4];
  paritycorreced_out[4] = data_parity_interleaved[16];
  for (int i = 11; i <= 25; i++)
    corrected_out[i] = data_parity_interleaved[i + 17 - 11];
  paritycorreced_out[5] = data_parity_interleaved[32];
  for (int i = 26; i <= 42; i++)
    corrected_out[i] = data_parity_interleaved[i + 33 - 26];

  // now we have the "corrected" data -> update the flags

  bool wrongparity;
  for (int i = 0; i < 43; i++)
    overallparitycalc ^= data_in[i];
  for (int i = 0; i < 6; i++)
    overallparitycalc ^= parityreceived[i];
  syndromeerror = (syndrome > 0) ? 1 : 0; // error if syndrome larger than 0
  wrongparity = (overallparitycalc != overallparity);
  overallparity_out = !syndromeerror && wrongparity ? overallparitycalc : overallparity; // If error was in parity fix parity
  error = syndromeerror | wrongparity;
  uncorrectable = (syndromeerror && (!wrongparity));

  //header_out = 0;
  //for (int i = 0; i < 43; i++)
  //  header_out |= corrected_out[i] << (i + 7);
  //header_out |= overallparity_out << 6;
  //for (int i = 0; i < 6; i++)
  //  header_out |= paritycorreced_out[i] << i;
  if (fix_data) {
    for (int i = 0; i < 6; i++)
      buffer[0] = (buffer[0] & ~(1 << i)) | (paritycorreced_out[i] << i);
    buffer[0] = (buffer[0] & ~(1 << 6)) | (overallparity_out << 6);
    for (int i = 7; i < 30; i++)
      buffer[0] = (buffer[0] & ~(1 << i)) | (corrected_out[i - 7] << i);
    for (int i = 30; i < 50; i++)
      buffer[1] = (buffer[1] & ~(1 << (i - 30))) | (corrected_out[i - 7] << (i - 30));
  }
}

void DecodeGBTWord(uint32_t* bufpt, uint32_t* data)
{
  uint32_t word = *(bufpt + 3);
  data[0] = (((word) >> 0) & 0x1) << 1;
  data[0] += (((word) >> 1) & 0x1);

  data[1] = (((word) >> 2) & 0x1) << 1;
  data[1] += (((word) >> 3) & 0x1);
  data[2] = (((word) >> 4) & 0x1) << 1;
  data[2] += (((word) >> 5) & 0x1);
  data[3] = (((word) >> 6) & 0x1) << 1;
  data[3] += (((word) >> 7) & 0x1);

  data[4] = (((word) >> 8) & 0x1) << 1;
  data[4] += (((word) >> 9) & 0x1);

  data[5] = (((word) >> 10) & 0x1) << 1;
  data[5] += (((word) >> 11) & 0x1);
  data[6] = (((word) >> 12) & 0x1) << 1;
  data[6] += (((word) >> 13) & 0x1);
  data[7] = (((word) >> 14) & 0x1) << 1;
  data[7] += (((word) >> 15) & 0x1);
  data[8] = (((word) >> 16) & 0x1) << 1;
  data[8] += (((word) >> 17) & 0x1);
  data[9] = (((word) >> 18) & 0x1) << 1;
  data[9] += (((word) >> 19) & 0x1);

  data[10] = (((word) >> 20) & 0x1) << 1;
  data[10] += (((word) >> 21) & 0x1);
  data[11] = (((word) >> 22) & 0x1) << 1;
  data[11] += (((word) >> 23) & 0x1);
  data[12] = (((word) >> 24) & 0x1) << 1;
  data[12] += (((word) >> 25) & 0x1);
  data[13] = (((word) >> 26) & 0x1) << 1;
  data[13] += (((word) >> 27) & 0x1);
  data[14] = (((word) >> 28) & 0x1) << 1;
  data[14] += (((word) >> 29) & 0x1);

  data[15] = (((word) >> 30) & 0x1) << 1;
  data[15] += (((word) >> 31) & 0x1);

  word = *(bufpt + 2);
  data[16] = (((word) >> 0) & 0x1) << 1;
  data[16] += (((word) >> 1) & 0x1);
  data[17] = (((word) >> 2) & 0x1) << 1;
  data[17] += (((word) >> 3) & 0x1);
  data[18] = (((word) >> 4) & 0x1) << 1;
  data[18] += (((word) >> 5) & 0x1);
  data[19] = (((word) >> 6) & 0x1) << 1;
  data[19] += (((word) >> 7) & 0x1);

  data[20] = (((word) >> 8) & 0x1) << 1;
  data[20] += (((word) >> 9) & 0x1);
  data[21] = (((word) >> 10) & 0x1) << 1;
  data[21] += (((word) >> 11) & 0x1);
  data[22] = (((word) >> 12) & 0x1) << 1;
  data[22] += (((word) >> 13) & 0x1);
  data[23] = (((word) >> 14) & 0x1) << 1;
  data[23] += (((word) >> 15) & 0x1);
  data[24] = (((word) >> 16) & 0x1) << 1;
  data[24] += (((word) >> 17) & 0x1);

  data[25] = (((word) >> 18) & 0x1) << 1;
  data[25] += (((word) >> 19) & 0x1);
  data[26] = (((word) >> 20) & 0x1) << 1;
  data[26] += (((word) >> 21) & 0x1);
  data[27] = (((word) >> 22) & 0x1) << 1;
  data[27] += (((word) >> 23) & 0x1);
  data[28] = (((word) >> 24) & 0x1) << 1;
  data[28] += (((word) >> 25) & 0x1);
  data[29] = (((word) >> 26) & 0x1) << 1;
  data[29] += (((word) >> 27) & 0x1);

  data[30] = (((word) >> 28) & 0x1) << 1;
  data[30] += (((word) >> 29) & 0x1);
  data[31] = (((word) >> 30) & 0x1) << 1;
  data[31] += (((word) >> 31) & 0x1);

  word = *(bufpt + 1);
  data[32] = (((word) >> 0) & 0x1) << 1;
  data[32] += (((word) >> 1) & 0x1);
  data[33] = (((word) >> 2) & 0x1) << 1;
  data[33] += (((word) >> 3) & 0x1);
  data[34] = (((word) >> 4) & 0x1) << 1;
  data[34] += (((word) >> 5) & 0x1);

  data[35] = (((word) >> 6) & 0x1) << 1;
  data[35] += (((word) >> 7) & 0x1);
  data[36] = (((word) >> 8) & 0x1) << 1;
  data[36] += (((word) >> 9) & 0x1);
  data[37] = (((word) >> 10) & 0x1) << 1;
  data[37] += (((word) >> 11) & 0x1);
  data[38] = (((word) >> 12) & 0x1) << 1;
  data[38] += (((word) >> 13) & 0x1);
  data[39] = (((word) >> 14) & 0x1) << 1;
  data[39] += (((word) >> 15) & 0x1);
}

decode_state_t Add1BitOfData(uint32_t gbtdata, DualSampa& dsr, DualSampaGroup* dsg)
{
  decode_state_t result = DECODE_STATE_UNKNOWN;
  if (!(dsr.status == notSynchronized)) { // data is synchronized => build the data word
    dsr.data += (gbtdata & 0x1) * dsr.powerMultiplier;
    dsr.powerMultiplier *= 2;
    dsr.bit++;
  }

  DualSampa* ds = &dsr;

  switch (ds->status) {
    case notSynchronized: {
      // Looking for Sync word (2 packets)
      // Look for 10 consecutives 01 (sent 10 from the GBT)
      if (ds->bit < 50) { // Fill the word
        ds->data += (gbtdata & 0x1) * ds->powerMultiplier;
        ds->powerMultiplier *= 2;
        ds->bit++;
      } else {
        if (ds->bit == 50)
          ds->powerMultiplier /= 2; // We want to fill the bit 49
        ds->data /= 2;              // Take out the bit 0
        ds->data &= 0x1FFFFFFFFFFFF;
        ds->data += (gbtdata & 0x1) * ds->powerMultiplier; // Fill bit 49
        ds->bit++;
      }

      if (ds->data == 0x1555540f00113 && ds->bit >= 50) {
        ds->bit = 0;
        ds->data = 0;
        ds->powerMultiplier = 1;
        ds->status = headerToRead;
        ds->chan_addr[0] = 0;
        ds->chan_addr[1] = 0;
        result = DECODE_STATE_SYNC_FOUND;
      }
      break;
    }
    case headerToRead: {
      // We are waiting for a Sampa header
      // It can be preceded by an undefined number os Sync words
      if (dsr.bit < 50)
        break;
      if (dsr.data == 0x1555540f00113) {
        result = DECODE_STATE_SYNC_FOUND;
      } else {
        result = DECODE_STATE_HEADER_FOUND;
        memcpy(&(ds->header), &(ds->data), sizeof(Sampa::SampaHeaderStruct));
        if (ds->nbHit >= 0) // if chip was synchronized
          ds->nbHit++;
        else
          ds->nbHit = 1;
        if ((ds->header.fChannelAddress >= 0) && (ds->header.fChannelAddress < 32)) {
          ds->nbHitChan[ds->header.fChannelAddress + 32 * (ds->header.fChipAddress % 2)]++;
        }

        if (dsg && dsg->bxc >= 0) {
          if (!BXCNT_compare(dsg->bxc, static_cast<long int>(ds->header.fBunchCrossingCounter))) {
            gNbErrors++;
          }
        } else {
          if (dsg && ds->header.fPkgType == 4) { // physics trigger
            dsg->bxc = ds->header.fBunchCrossingCounter;
          }
        }

        ds->packetsize = 0;

        unsigned int buf[2];
        buf[0] = ds->data & 0x3FFFFFFF;
        buf[1] = ds->data >> 30;
        bool hamming_error = false;  // Is there an hamming error?
        bool hamming_uncorr = false; // Is the data correctable?
        bool hamming_enable = false; // Correct the data?
        HammingDecode(buf, hamming_error, hamming_uncorr, hamming_enable);
        if (hamming_error) {
          gNbErrors++;
          ds->status = notSynchronized;
          result = DECODE_STATE_UNKNOWN;
        } else {                          // No Hamming error
          if (ds->header.fPkgType == 4) { // Good data
            ds->status = sizeToRead;
            ds->bxc[ds->header.fChipAddress % 2] = ds->header.fBunchCrossingCounter;
          } else {
            if (ds->header.fPkgType == 1 || ds->header.fPkgType == 3) { // Data truncated
              gNbErrors++;
              if (ds->header.fNbOf10BitWords)
                ds->status = dataToRead;
              else
                ds->status = headerToRead;
            }
            if (ds->header.fPkgType == 0) { // Heartbeat: Pkg 0, NbOfWords 0 ?, ChAdd 21
              gNbErrors++;
              ds->status = headerToRead;
              ds->bxc[ds->header.fChipAddress % 2] = ds->header.fBunchCrossingCounter;
            }
            if (ds->header.fPkgType == 5) { //
              gNbErrors++;
              ds->status = headerToRead;
            }
            if (ds->header.fPkgType == 6) { //
              //ds->status = headerToRead;
              ds->status = sizeToRead;
            }
            if (ds->header.fPkgType == 2) { //
              gNbErrors++;
              ds->status = notSynchronized;
              result = DECODE_STATE_UNKNOWN;
            }
          }
        }
      }

      if (ds->status != notSynchronized) {
        ds->bit = 0;
        ds->data = 0;
        ds->powerMultiplier = 1;
      }
      break;
    }
    case sizeToRead: {
      if (ds->bit < 10)
        break;

      result = DECODE_STATE_CSIZE_FOUND;

      int chip0 = (ds->id % 5) * 2;
      int chip1 = chip0 + 1;

      if (ds->header.fChipAddress < chip0 || ds->header.fChipAddress > chip1) {
        gNbWarnings++;
      }
      if (ds->chan_addr[ds->header.fChipAddress - chip0] != ds->header.fChannelAddress) {
        gNbWarnings++;
      }
      ds->chan_addr[ds->header.fChipAddress - chip0] += 1;
      if (ds->chan_addr[ds->header.fChipAddress - chip0] > 31) {
        ds->chan_addr[ds->header.fChipAddress - chip0] = 0;
      }

      ds->csize = ds->data;
      ds->cid = 0;
      ds->packetsize += 1;
      ds->status = timeToRead;

      if (ds->status != notSynchronized) {
        ds->bit = 0;
        ds->data = 0;
        ds->powerMultiplier = 1;
      }

      break;
    }
    case timeToRead: { // Read Time Count (10 bits)
      if (ds->bit < 10)
        break;
      result = DECODE_STATE_CTIME_FOUND;

      ds->ctime = ds->data;
      ds->packetsize += 1;
      ds->status = dataToRead;
      //ds->status = chargeToRead;

      if (ds->status != notSynchronized) {
        ds->bit = 0;
        ds->data = 0;
        ds->powerMultiplier = 1;
      }

      break;
    }
    case dataToRead: { // Read ADC data words (10 bits)
      if (ds->bit < 10)
        break;

      if (1 /*ds->header.fPkgType == 4*/) {
        if (ds->header.fPkgType == 4) { // Good data
          result = DECODE_STATE_SAMPLE_FOUND;
          ds->sample = ds->data;

          if (gPattern > 0) {
            int patt = (gPattern & 0xFF) + (gPattern << 8 & 0xFF00);
            if ((ds->data & 0x2FF) != (patt & 0x2FF)) {
              gNbWarnings++;
            }
          }
        }
        ds->cid += 1;
        ds->packetsize += 1;
        bool end_of_packet = (ds->header.fNbOf10BitWords == ds->packetsize);
        bool end_of_cluster = (ds->cid == ds->csize);
        if (end_of_packet && !end_of_cluster) {
          // That's the end of the packet, but the cluster is still being read... that's not normal
          gNbErrors++;
          ds->status = headerToRead;
        } else if (end_of_cluster) {
          if (ds->header.fPkgType == 4) { // Good data
            ds->nclus[ds->header.fChipAddress % 2][ds->header.fChannelAddress] += 1;
            result = DECODE_STATE_END_OF_CLUSTER;
          }
          if (ds->header.fNbOf10BitWords > ds->packetsize)
            ds->status = sizeToRead;
          else {
            ds->packetsize = 0;
            ds->status = headerToRead;
          }
        }
      } else {
        if (ds->header.fPkgType == 1 || ds->header.fPkgType == 3) { // Data truncated
          gNbWarnings++;
          if (ds->header.fNbOf10BitWords - 1)
            ds->header.fNbOf10BitWords--;
          else
            ds->status = headerToRead;
        }
      }

      if (ds->status != notSynchronized) {
        ds->bit = 0;
        ds->data = 0;
        ds->powerMultiplier = 1;
      }

      break;
    }
    default:
      break;
  }

  return result;
}

decode_state_t Add10BitsOfData(uint64_t data, DualSampa& dsr, DualSampaGroup* /*dsg*/)
{
  decode_state_t result = DECODE_STATE_UNKNOWN;
  switch (dsr.status) {
    case notSynchronized:
      dsr.data += data << dsr.bit;

      dsr.bit += 10;

      if (dsr.bit == 50) {
        if (dsr.data == 0x1555540f00113) {
          result = DECODE_STATE_SYNC_FOUND;
          dsr.status = headerToRead;
          dsr.bit = 0;
          dsr.data = 0;
          dsr.packetsize = 0;
        } else {
          dsr.data = dsr.data >> 10;
          dsr.bit -= 10;
        }
      }
      break;

    case headerToRead:
      dsr.data += data << dsr.bit;

      dsr.bit += 10;

      if (dsr.bit == 50) {
        if (dsr.data == 0x1555540f00113) {
          result = DECODE_STATE_SYNC_FOUND;
          dsr.status = headerToRead;
          dsr.bit = 0;
          dsr.data = 0;
          dsr.packetsize = 0;
        } else {
          result = DECODE_STATE_HEADER_FOUND;
          memcpy(&(dsr.header), &(dsr.data), sizeof(Sampa::SampaHeaderStruct));
          dsr.status = sizeToRead;
          dsr.bit = 0;
          dsr.data = 0;
          dsr.packetsize = 0;
        }
      }
      break;

    case sizeToRead:
      dsr.csize = data;
      dsr.cid = 0;
      dsr.packetsize += 1;
      if ((dsr.csize + 2) > dsr.header.fNbOf10BitWords) {
        dsr.status = notSynchronized;
        dsr.bit = 0;
        dsr.data = 0;
        dsr.packetsize = 0;
      } else {
        if (dsr.packetsize == dsr.header.fNbOf10BitWords) {
          dsr.status = notSynchronized;
          dsr.bit = 0;
          dsr.data = 0;
          dsr.packetsize = 0;
        } else {
          dsr.status = timeToRead;
          result = DECODE_STATE_CSIZE_FOUND;
        }
      }
      break;

    case timeToRead:
      dsr.ctime = data;
      dsr.packetsize += 1;
      if (dsr.packetsize == dsr.header.fNbOf10BitWords) {
        dsr.status = notSynchronized;
        dsr.bit = 0;
        dsr.data = 0;
        dsr.packetsize = 0;
      } else {
        dsr.status = dataToRead;
        result = DECODE_STATE_CTIME_FOUND;
      }
      break;

    case dataToRead:
      dsr.sample = data;
      dsr.cid += 1;
      dsr.packetsize += 1;
      bool end_of_packet = (dsr.header.fNbOf10BitWords == dsr.packetsize);
      bool end_of_cluster = (dsr.cid == dsr.csize);
      //printf("dataToRead: cid=%d  packetsize=%d  end_of_packet=%d  end_of_cluster=%d\n",
      //    dsr.cid, dsr.packetsize, (int)end_of_packet, (int)end_of_cluster);
      if (end_of_packet && !end_of_cluster) {
        dsr.bit = 0;
        dsr.data = 0;
        dsr.packetsize = 0;
        dsr.status = notSynchronized;
      } else {
        result = DECODE_STATE_SAMPLE_FOUND;
        dsr.status = dataToRead;
        if (end_of_cluster) {
          result = DECODE_STATE_END_OF_CLUSTER;
          if (end_of_packet) {
            dsr.bit = 0;
            dsr.data = 0;
            dsr.packetsize = 0;
            dsr.status = headerToRead;
            result = DECODE_STATE_END_OF_PACKET;
          } else {
            dsr.status = sizeToRead;
          }
        }
      }
      break;
  }

  return result;
}

Decoder::Decoder() {}

Decoder::~Decoder() {}

void Decoder::initialize()
{
  QcInfoLogger::GetInstance() << "initialize Decoder" << AliceO2::InfoLogger::InfoLogger::endm;
  fprintf(stdout, "initialize Decoder\n");

  hb_orbit = -1;
  nFrames = 0;

  fprintf(stdout, "initialize DS structures\n");
  for (int c = 0; c < MCH_MAX_CRU_ID; c++) {
    for (int l = 0; l < 24; l++) {
      for (int i = 0; i < 40; i++) {
        DualSampaInit(&(ds[c][l][i]));
        ds[c][l][i].id = i;
        ds[c][l][i].nbHit = -1;
        for (int j = 0; j < 64; j++) {
          ds[c][l][i].nbHitChan[j] = 0;
        }
      }
      for (int i = 0; i < 8; i++) {
        DualSampaGroupInit(&(dsg[c][l][i]));
      }
    }
  }

  mMapCRU.readMapping("cru.map");
  mMapFEC.readDSMapping("fec.map");

  fprintf(stdout, "initialize ds_enable\n");
  for (int c = 0; c < MCH_MAX_CRU_IN_FLP; c++) {
    for (int l = 0; l < 24; l++) {
      int32_t link_id = mMapCRU.getLink(c, l);
      if (link_id < 0) {
        for (int i = 0; i < 40; i++) {
          ds_enable[c][l][i] = 0;
        }
      } else {
        for (int i = 0; i < 40; i++) {
          uint32_t de, dsid;
          ds_enable[c][l][i] = mMapFEC.getDSMapping(link_id, i, de, dsid) ? 1 : 0;
        }
      }
    }
  }
  /*
  std::ifstream ds_enable_f("/tmp/board_enable.txt");
  fprintf(stdout, "Reading /tmp/board_enable.txt");
  while (!ds_enable_f.fail()) {
    int c, l, b, e;
    ds_enable_f >> c >> l >> b >> e;
    if (c < 0 || c >= MCH_MAX_CRU_IN_FLP)
      continue;
    if (l < 0 || l >= 24)
      continue;
    if (b < 0 || b >= 40)
      continue;
    ds_enable[c][l][b] = e;
    fprintf(stdout, "ds_enable[%d][%d][%d]=%d\n", c, l, b, ds_enable[c][l][b]);
  }
  */

  //else
  fprintf(stdout, "Decoder initialization finished\n");
}

void Decoder::decodeRaw(uint32_t* payload_buf, size_t nGBTwords, int cru_id, int link_id)
{
  //fprintf(stdout,"[decodeRaw] payload_buf=%p\n", (void*)payload_buf);
  uint32_t hhvalue, hlvalue, lhvalue, llvalue;
  for (size_t wi = 0; wi < nGBTwords; wi++) {
    uint32_t* ptr = payload_buf + wi * 4;
    hhvalue = ptr[3];
    hlvalue = ptr[2];
    lhvalue = ptr[1];
    llvalue = ptr[0];
    //fprintf(stdout,"wi=%d  %.8X %.8X %.8X %.8X\n", wi,
    //    hhvalue, hlvalue, lhvalue, llvalue);

    uint32_t bufpt[4] = { hhvalue, hlvalue, lhvalue, llvalue };
    uint32_t data2bits[40] = { 0 };
    DecodeGBTWord(bufpt, data2bits);

    for (int i = 0; i < 40; i++) {
      if (ds_enable[cru_id][link_id][i] == 0)
        continue;
      //fprintf(stdout,"processing board %d %d %d\n", cru_id, link_id, i);

      uint32_t group = ds[cru_id][link_id][i].id / 5;
      uint32_t bits[2] = { data2bits[i] & 0x1, (data2bits[i] >> 1) & 0x1 };

      SampaHit& hit = ds[cru_id][link_id][i].hit;
      hit.cru_id = cru_id;
      hit.data_path = link_id / 12;
      hit.fee_id = cru_id * 2 + hit.data_path;
      hit.link_id = link_id;
      hit.ds_addr = ds[cru_id][link_id][i].id;
      for (int k = 0; k < 2; k++) {
        decode_state_t state = Add1BitOfData(bits[k], (ds[cru_id][link_id][i]), &(dsg[cru_id][link_id][group]));
        switch (state) {
          case DECODE_STATE_SYNC_FOUND:
            break;
          case DECODE_STATE_HEADER_FOUND:
            uint64_t _h;
            memcpy(&_h, &(ds[cru_id][link_id][i].header), sizeof(ds[cru_id][link_id][i].header));
            break;
          case DECODE_STATE_CSIZE_FOUND: {
            Sampa::SampaHeaderStruct& header = ds[cru_id][link_id][i].header;
            SampaHit& hit = ds[cru_id][link_id][i].hit;
            hit.cru_id = cru_id;
            hit.data_path = link_id / 12;
            hit.fee_id = cru_id * 2 + hit.data_path;
            hit.link_id = link_id;
            hit.ds_addr = ds[cru_id][link_id][i].id;
            int chip_id = ds[cru_id][link_id][i].header.fChipAddress % 2;
            hit.chan_addr = header.fChannelAddress + 32 * chip_id;
            hit.bxc = header.fBunchCrossingCounter;
            hit.size = ds[cru_id][link_id][i].csize;
            hit.samples.clear();
            hit.csum = 0;
            hit.time = 0;
            for (int ci = 0; ci < 2; ci++) {
              for (int cj = 0; cj < 32; cj++) {
                ds[cru_id][link_id][i].min[ci][cj] = 0xFFFFFFFF;
                ds[cru_id][link_id][i].max[ci][cj] = 0;
              }
            }
            break;
          }
          case DECODE_STATE_CTIME_FOUND:
            ds[cru_id][link_id][i].hit.time = ds[cru_id][link_id][i].ctime;
            break;
          case DECODE_STATE_SAMPLE_FOUND:
          case DECODE_STATE_END_OF_CLUSTER: {
            SampaHit& hit = ds[cru_id][link_id][i].hit;
            hit.samples.push_back(ds[cru_id][link_id][i].sample);
            hit.csum += ds[cru_id][link_id][i].sample;

            int chipid = hit.ds_addr % 2;
            int chid = hit.chan_addr;
            if (ds[cru_id][link_id][i].min[chipid][chid] > ds[cru_id][link_id][i].sample)
              ds[cru_id][link_id][i].min[chipid][chid] = ds[cru_id][link_id][i].sample;
            if (ds[cru_id][link_id][i].max[chipid][chid] < ds[cru_id][link_id][i].sample)
              ds[cru_id][link_id][i].max[chipid][chid] = ds[cru_id][link_id][i].sample;

            if (state == DECODE_STATE_END_OF_CLUSTER) {
              int32_t deltaNew = ds[cru_id][link_id][i].max[chipid][chid] - ds[cru_id][link_id][i].min[chipid][chid];
              //if( hit.deltaMax < deltaNew )
              hit.delta = deltaNew;
              mHits.push_back(hit);
              if (hit.link_id >= 24) {
                fprintf(stdout, "hit: link_id=%d, ds_addr=%d, chan_addr=%d\n",
                        hit.link_id, hit.ds_addr, hit.chan_addr);
                getchar();
              }
              hit.size = 0;
              hit.samples.clear();
              hit.csum = 0;
              hit.time = 0;
            }
            break;
          }
          default:
            break;
        }
      }
    }
  }
}

void Decoder::decodeUL(uint32_t* payload_buf_32, size_t nWords, int cru_id, int dpw_id)
{
  uint64_t* payload_buf = (uint64_t*)payload_buf_32;
  for (size_t wi = 0; wi < nWords; wi += 1) {

    uint64_t* ptr = payload_buf + wi;
    uint64_t value = *ptr;

    int link_id = (value >> 59) & 0x1F;
    int ds_id = (value >> 53) & 0x3F;
    link_id += 12 * dpw_id;

    if (value == 0xFFFFFFFFFFFFFFFF)
      continue;
    if (value == 0xFEEDDEEDFEEDDEED)
      continue;

    int is_incomplete = (value >> 52) & 0x1;
    if (link_id < 0 || link_id >= 24 || ds_id < 0 || ds_id >= 40) {
      continue;
    }

    bool skip = false;
    for (int b = 0; b < 50; b += 10) {

      decode_state_t state = Add10BitsOfData((value >> b) & 0x3FF, ds[cru_id][link_id][ds_id], &dsg[cru_id][link_id][ds_id / 8]);
      switch (state) {
        case DECODE_STATE_SYNC_FOUND:
          break;
        case DECODE_STATE_HEADER_FOUND:
          uint64_t _h;
          memcpy(&_h, &(ds[cru_id][link_id][ds_id].header), sizeof(ds[cru_id][link_id][ds_id].header));
          break;
        case DECODE_STATE_CSIZE_FOUND: {
          Sampa::SampaHeaderStruct& header = ds[cru_id][link_id][ds_id].header;
          SampaHit& hit = ds[cru_id][link_id][ds_id].hit;
          hit.cru_id = cru_id;
          hit.link_id = link_id;
          hit.ds_addr = ds[cru_id][link_id][ds_id].id;
          int chip_id = ds[cru_id][link_id][ds_id].header.fChipAddress % 2;
          hit.chan_addr = header.fChannelAddress + 32 * chip_id;
          hit.bxc = header.fBunchCrossingCounter;
          hit.size = ds[cru_id][link_id][ds_id].csize;
          hit.samples.clear();
          hit.csum = 0;
          hit.time = 0;
          break;
        }
        case DECODE_STATE_CTIME_FOUND:
          ds[cru_id][link_id][ds_id].hit.time = ds[cru_id][link_id][ds_id].ctime;
          break;
        case DECODE_STATE_SAMPLE_FOUND:
        case DECODE_STATE_END_OF_CLUSTER:
        case DECODE_STATE_END_OF_PACKET: {
          SampaHit& hit = ds[cru_id][link_id][ds_id].hit;
          hit.samples.push_back(ds[cru_id][link_id][ds_id].sample);
          hit.csum += ds[cru_id][link_id][ds_id].sample;

          if (state == DECODE_STATE_END_OF_CLUSTER ||
              state == DECODE_STATE_END_OF_PACKET) {
            mHits.push_back(hit);
            if (hit.link_id >= 24) {
              fprintf(stdout, "hit: link_id=%d, ds_addr=%d, chan_addr=%d\n",
                      hit.link_id, hit.ds_addr, hit.chan_addr);
              getchar();
            }
            hit.size = 0;
            hit.samples.clear();
            hit.csum = 0;
            hit.time = 0;
          }
          if (state == DECODE_STATE_END_OF_PACKET && is_incomplete)
            skip = true;
          break;
        }
        default:
          break;
      }

      if (skip)
        break;
    }
  }
}

void Decoder::processData(const char* buf, size_t size)
{
  int RDH_BLOCK_SIZE = 8192;

  int manu2ds[64] = {
    63, 62, 61, 60, 59, 57, 56, 53, 51, 50, 47, 45, 44, 41, 38, 35,
    36, 33, 34, 37, 32, 39, 40, 42, 43, 46, 48, 49, 52, 54, 55, 58,
    7, 8, 5, 2, 6, 1, 3, 0, 4, 9, 10, 15, 17, 18, 22, 25,
    31, 30, 29, 28, 27, 26, 24, 23, 20, 21, 16, 19, 12, 14, 11, 13
  };

  int ds2manu[64];
  for (int i = 0; i < 64; i++) {
    for (int j = 0; j < 64; j++) {
      if (manu2ds[j] != i)
        continue;
      ds2manu[i] = j;
      break;
    }
  }

  const RDH* rdh = (const RDH*)buf;
  uint32_t payload_offset = 0;
  if (size < sizeof(RDH))
    return;

  while (payload_offset < size) {

    uint32_t* payload_buf = (uint32_t*)(buf + 16 * 4);
    //fprintf(stdout,"[processData] payload_buf=%p\n", (void*)payload_buf);

    auto rdhVersion = o2::raw::RDHUtils::getVersion(rdh);
    auto rdhHeaderSize = o2::raw::RDHUtils::getHeaderSize(rdh);
    auto frameSize = o2::raw::RDHUtils::getOffsetToNext(rdh);
    auto memorySize = o2::raw::RDHUtils::getMemorySize(rdh);
    auto payloadSize = memorySize - rdhHeaderSize;
    int cruId = o2::raw::RDHUtils::getCRUID(rdh);
    auto rdhLinkId = o2::raw::RDHUtils::getLinkID(rdh);
    int dpwId = o2::raw::RDHUtils::getEndPointID(rdh);
    auto rdhOrbit = o2::raw::RDHUtils::getHeartBeatOrbit(rdh);

    // Check RDH version and size
    if (rdhVersion < 4 || rdhVersion > 6) {
      QcInfoLogger::GetInstance() << "[Decoder::processData] Wrong CRU header version: " << (int)rdhVersion << AliceO2::InfoLogger::InfoLogger::endm;
      return;
    }
    if (rdhHeaderSize != 64) {
      QcInfoLogger::GetInstance() << "[Decoder::processData] Wrong CRU header size: " << (int)rdhHeaderSize << AliceO2::InfoLogger::InfoLogger::endm;
      return;
    }

    // Compute size of payload inside 8kB block
    RDH_BLOCK_SIZE = frameSize;

    rdh += RDH_BLOCK_SIZE;
    payload_offset += RDH_BLOCK_SIZE;

    nFrames += 1;

    int cru_lid = (rdhLinkId == 15) ? rdhLinkId : rdhLinkId + 12 * dpwId;
    bool is_raw = (rdhLinkId != 15);

    bool orbit_jump = true;
    int Dorbit1 = rdhOrbit - hb_orbit;
    int Dorbit2 = (uint32_t)(((uint64_t)rdhOrbit) + 0x100000000 - hb_orbit);
    if (Dorbit1 >= 0 && Dorbit1 <= 1)
      orbit_jump = false;
    if (Dorbit2 >= 0 && Dorbit2 <= 1)
      orbit_jump = false;
    if (true && orbit_jump) {
      int lid_min = (rdhLinkId == 15) ? dpwId * 12 : 0;
      int lid_max = (rdhLinkId == 15) ? 11 + dpwId * 12 : 23;
      for (int l = lid_min; l <= lid_max; l++) {
        for (int i = 0; i < 40; i++) {
          DualSampaReset(&(ds[cruId][l][i]));
          ds[cruId][l][i].id = i;
          ds[cruId][l][i].nbHit = -1;
          for (int j = 0; j < 64; j++) {
            ds[cruId][l][i].nbHitChan[j] = 0;
          }
        }
        for (int i = 0; i < 8; i++) {
          DualSampaGroupReset(&(dsg[cruId][l][i]));
        }
      }
    }
    hb_orbit = rdhOrbit;

    int nGBTwords = payloadSize / 16;
    int n64bitWords = payloadSize / 8;

    if (is_raw)
      decodeRaw(payload_buf, nGBTwords, cruId, cru_lid);
    else
      decodeUL(payload_buf, n64bitWords, cruId, dpwId);

    for (size_t ih = 0; ih < mHits.size(); ih++) {
      SampaHit& hit = mHits[ih];
      hit.pad.fDE = -1;
      hit.pad.fCathode = 0;
      int manuch = ds2manu[hit.chan_addr];
      hit.chan_addr = manuch;

      int32_t link_id = mMapCRU.getLink(hit.cru_id, hit.link_id);
      if (link_id < 0)
        continue;
      //printf("cru_id=%d link_id=%d  LID=%d\n", (int)hit.cru_id, (int)hit.link_id, (int)link_id);

      if (!mMapFEC.getPadByLinkID(link_id, hit.ds_addr, hit.chan_addr, hit.pad))
        continue;

      mDigits.emplace_back(o2::mch::Digit(hit.pad.fDE, hit.pad.fAddress, hit.csum, o2::mch::Digit::Time{}));
    }
  }
}

void Decoder::clearHits()
{
  mHits.clear();
}

void Decoder::clearDigits()
{
  mDigits.clear();
}

void Decoder::reset()
{
  clearHits();
  clearDigits();
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
