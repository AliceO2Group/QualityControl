///
/// \file   MuonChambersDataDecoder.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "Headers/RAWDataHeader.h"
#include "QualityControl/QcInfoLogger.h"
#include "MuonChambers/MuonChambersDataDecoder.h"

using namespace std;



static int gPrintLevel;
static int gPattern;
static int gNbErrors;
static int gNbWarnings;

static FILE* flog = NULL;

struct CRUheader
{
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
  uint16_t source_id;
  uint32_t hb_orbit;
  //uint16_t cru_id;
  //uint8_t dummy1;
  //uint64_t dummy2;
};


enum decode_state_t
{
  DECODE_STATE_UNKNOWN,
  DECODE_STATE_SYNC_FOUND,
  DECODE_STATE_HEADER_FOUND,
  DECODE_STATE_CSIZE_FOUND,
  DECODE_STATE_CTIME_FOUND,
  DECODE_STATE_SAMPLE_FOUND,
  DECODE_STATE_END_OF_CLUSTER
};


namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

/*
static void SampaHeaderDump(struct Sampa::SampaHeaderStruct* sampaHeader)
{
  printf("--> SAMPA Header: HCode %d HPar %d PkgType %d 10BitWords %d ChipAdd %d ChAdd %d BX %d PPar %d",
      (int)sampaHeader->fHammingCode,(int)sampaHeader->fHeaderParity,(int)sampaHeader->fPkgType,
      (int)sampaHeader->fNbOf10BitWords,(int)sampaHeader->fChipAddress,(int)sampaHeader->fChannelAddress,
      (int)sampaHeader->fBunchCrossingCounter,(int)sampaHeader->fPayloadParity);
  printf("\n");

}
*/

bool BXCNT_compare(int64_t c1, int64_t c2)
{
  const int64_t MAX = 0xFFFFF;
  //int64_t diff = c1 - c2;
  //if(diff >= MAX) diff -= MAX;
  //if(diff <= -MAX) diff += MAX;
  int64_t c1p = (c1+1) & MAX;
  int64_t c2p = (c2+1) & MAX;
  if( c1 == c2 ) return true;
  if( c1p == c2 ) return true;
  if( c2p == c1 ) return true;
  return false;
}

void DualSampaInit(DualSampa* ds)
{
  if (gPrintLevel >= 4) fprintf(flog,"DualSampaInit() called\n");
  ds->status = notSynchronized;
  ds->data = 0;
  ds->bit = 0;
  ds->powerMultiplier = 1;
  ds->nsyn2Bits = 0;
  ds->bxc[0] = ds->bxc[1] = -1;
  ds->chan_addr[0] = 0;
  ds->chan_addr[1] = 0;
  for(int j = 0; j < 2; j++) {
    for(int k = 0; k < 32; k++) {
      ds->ndata[j][k] = 0;
      ds->nclus[j][k] = 0;
      ds->pedestal[j][k] = 0;
      ds->noise[j][k] = 0;
    }
  }
  //ds->nbHit=-1; don't reset counter - do it externally
}

void DualSampaReset(DualSampa* ds)
{
  if (gPrintLevel >= 4) fprintf(flog,"DualSampaReset() called\n");
  ds->status = notSynchronized;
  ds->data = 0;
  ds->bit = 0;
  ds->powerMultiplier = 1;
  ds->nsyn2Bits = 0;
  ds->bxc[0] = ds->bxc[1] = -1;
  ds->chan_addr[0] = 0;
  ds->chan_addr[1] = 0;
  //ds->nbHit=-1; don't reset counter - do it externally
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
  int parity,bit;
  parity = data&0x1;
  for (int i = 1; i < 50; i++)
  {
    bit = (data>>i)&0x1;
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
    data_in[i-7] = (buffer[0] >> i) & 0x1;
  for (int i = 30; i < 50; i++)
    data_in[i-7] = (buffer[1] >> (i - 30)) & 0x1;

  //calculated values
  bool corrected_out[43];
  bool overallparitycalc = 0;
  bool overallparity_out = 0;
  bool paritycalc[6];
  bool paritycorreced_out[6];

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // calculate parity
  paritycalc[0]   =   data_in[0]  ^ data_in[1]  ^ data_in[3]  ^ data_in[4]  ^ data_in[6]  ^
      data_in[8]  ^ data_in[10] ^ data_in[11] ^ data_in[13] ^ data_in[15] ^
      data_in[17] ^ data_in[19] ^ data_in[21] ^ data_in[23] ^ data_in[25] ^
      data_in[26] ^ data_in[28] ^ data_in[30] ^ data_in[32] ^ data_in[34] ^
      data_in[36] ^ data_in[38] ^ data_in[40] ^ data_in[42];

  paritycalc[1]   =   data_in[0]  ^ data_in[2]  ^ data_in[3]  ^ data_in[5]  ^ data_in[6]  ^
      data_in[9]  ^ data_in[10] ^ data_in[12] ^ data_in[13] ^ data_in[16] ^
      data_in[17] ^ data_in[20] ^ data_in[21] ^ data_in[24] ^ data_in[25] ^
      data_in[27] ^ data_in[28] ^ data_in[31] ^ data_in[32] ^ data_in[35] ^
      data_in[36] ^ data_in[39] ^ data_in[40] ;

  paritycalc[2]   =   data_in[1]  ^ data_in[2]  ^ data_in[3]  ^ data_in[7]  ^ data_in[8]  ^
      data_in[9]  ^ data_in[10] ^ data_in[14] ^ data_in[15] ^ data_in[16] ^
      data_in[17] ^ data_in[22] ^ data_in[23] ^ data_in[24] ^ data_in[25] ^
      data_in[29] ^ data_in[30] ^ data_in[31] ^ data_in[32] ^ data_in[37] ^
      data_in[38] ^ data_in[39] ^ data_in[40] ;

  paritycalc[3]   =   data_in[4]  ^ data_in[5]  ^ data_in[6]  ^ data_in[7]  ^ data_in[8]  ^
      data_in[9]  ^ data_in[10] ^ data_in[18] ^ data_in[19] ^ data_in[20] ^
      data_in[21] ^ data_in[22] ^ data_in[23] ^ data_in[24] ^ data_in[25] ^
      data_in[33] ^ data_in[34] ^ data_in[35] ^ data_in[36] ^ data_in[37] ^
      data_in[38] ^ data_in[39] ^ data_in[40] ;

  paritycalc[4]   =   data_in[11] ^ data_in[12] ^ data_in[13] ^ data_in[14] ^ data_in[15] ^
      data_in[16] ^ data_in[17] ^ data_in[18] ^ data_in[19] ^ data_in[20] ^
      data_in[21] ^ data_in[22] ^ data_in[23] ^ data_in[24] ^ data_in[25] ^
      data_in[41] ^ data_in[42] ;

  paritycalc[5]   =   data_in[26] ^ data_in[27] ^ data_in[28] ^ data_in[29] ^ data_in[30] ^
      data_in[31] ^ data_in[32] ^ data_in[33] ^ data_in[34] ^ data_in[35] ^
      data_in[36] ^ data_in[37] ^ data_in[38] ^ data_in[39] ^ data_in[40] ^
      data_in[41] ^ data_in[42] ;
  ////////////////////////////////////////////////////////////////////////////////////////////////

  //    uint8_t syndrome = 0;
  unsigned char syndrome = 0;

  for (int i = 0; i < 6; i++)
    syndrome |= (paritycalc[i]^parityreceived[i]) << i;

  bool data_parity_interleaved[64];
  bool syndromeerror;

  //data_parity_interleaved[0]          =  0;
  data_parity_interleaved[1]          =  parityreceived[0];
  data_parity_interleaved[2]          =  parityreceived[1];
  data_parity_interleaved[3]          =  data_in[0];
  data_parity_interleaved[4]          =  parityreceived[2];
  for (int i = 1; i <= 3; i++)
    data_parity_interleaved[i+5-1]    =  data_in[i];
  data_parity_interleaved[8]          =  parityreceived[3];
  for (int i = 4; i <= 10; i++)
    data_parity_interleaved[i+9-4]    =  data_in[i];
  data_parity_interleaved[16]         =  parityreceived[4];
  for (int i = 11; i <= 25; i++)
    data_parity_interleaved[i+17-11]  =  data_in[i];
  data_parity_interleaved[32]         =  parityreceived[5];
  for (int i = 26; i <= 42; i++)
    data_parity_interleaved[i+33-26]  =  data_in[i];
  //for (int i = 50; i <= 63; i++)
  //  data_parity_interleaved[i]        =  0;

  data_parity_interleaved[syndrome] =  !data_parity_interleaved[syndrome]; // correct the interleaved

  paritycorreced_out[0] = data_parity_interleaved[1];
  paritycorreced_out[1] = data_parity_interleaved[2];
  corrected_out[0]        = data_parity_interleaved[3];
  paritycorreced_out[2]   = data_parity_interleaved[4];
  for (int i = 1; i <= 3; i++)
    corrected_out[i]      = data_parity_interleaved[i+5-1];
  paritycorreced_out[3]   = data_parity_interleaved[8];
  for (int i = 4; i <= 10; i++)
    corrected_out[i]     = data_parity_interleaved[i+9-4];
  paritycorreced_out[4]   = data_parity_interleaved[16];
  for (int i = 11; i <= 25; i++)
    corrected_out[i]    = data_parity_interleaved[i+17-11];
  paritycorreced_out[5]   = data_parity_interleaved[32];
  for (int i = 26; i <= 42; i++)
    corrected_out[i]    = data_parity_interleaved[i+33-26];

  // now we have the "corrected" data -> update the flags

  bool wrongparity;
  for (int i = 0; i < 43; i++)
    overallparitycalc ^=data_in[i];
  for (int i = 0; i < 6; i++)
    overallparitycalc ^= parityreceived[i];
  syndromeerror = (syndrome > 0) ? 1 : 0; // error if syndrome larger than 0
  wrongparity = (overallparitycalc != overallparity);
  overallparity_out = !syndromeerror &&  wrongparity ? overallparitycalc : overallparity; // If error was in parity fix parity
  error = syndromeerror |  wrongparity;
  uncorrectable = (syndromeerror && (!wrongparity));


  //header_out = 0;
  //for (int i = 0; i < 43; i++)
  //  header_out |= corrected_out[i] << (i + 7);
  //header_out |= overallparity_out << 6;
  //for (int i = 0; i < 6; i++)
  //  header_out |= paritycorreced_out[i] << i;
  if (fix_data)
  {
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
  uint32_t word = *(bufpt+3);
  //fprintf(flog,"*(bufpt+3): %.8X\n", word);
  data[0] =  (((word)>>0)&0x1)<<1;
  data[0] += (((word)>>1)&0x1);

  //return;

  data[1] =  (((word)>>2)&0x1)<<1;
  data[1] += (((word)>>3)&0x1);
  data[2] =  (((word)>>4)&0x1)<<1;
  data[2] += (((word)>>5)&0x1);
  data[3] =  (((word)>>6)&0x1)<<1;
  data[3] += (((word)>>7)&0x1);


  data[4] =  (((word)>>8)&0x1)<<1;
  data[4] += (((word)>>9)&0x1);

  //return;

  data[5] =  (((word)>>10)&0x1)<<1;
  data[5] += (((word)>>11)&0x1);
  data[6] =  (((word)>>12)&0x1)<<1;
  data[6] += (((word)>>13)&0x1);
  data[7] =  (((word)>>14)&0x1)<<1;
  data[7] += (((word)>>15)&0x1);
  data[8] =  (((word)>>16)&0x1)<<1;
  data[8] += (((word)>>17)&0x1);
  data[9] =  (((word)>>18)&0x1)<<1;
  data[9] += (((word)>>19)&0x1);

  data[10] =  (((word)>>20)&0x1)<<1;
  data[10] += (((word)>>21)&0x1);
  data[11] =  (((word)>>22)&0x1)<<1;
  data[11] += (((word)>>23)&0x1);
  data[12] =  (((word)>>24)&0x1)<<1;
  data[12] += (((word)>>25)&0x1);
  data[13] =  (((word)>>26)&0x1)<<1;
  data[13] += (((word)>>27)&0x1);
  data[14] =  (((word)>>28)&0x1)<<1;
  data[14] += (((word)>>29)&0x1);

  data[15] =  (((word)>>30)&0x1)<<1;
  data[15] += (((word)>>31)&0x1);

  word = *(bufpt+2);
  data[16] =  (((word)>>0)&0x1)<<1;
  data[16] += (((word)>>1)&0x1);
  data[17] =  (((word)>>2)&0x1)<<1;
  data[17] += (((word)>>3)&0x1);
  data[18] =  (((word)>>4)&0x1)<<1;
  data[18] += (((word)>>5)&0x1);
  data[19] =  (((word)>>6)&0x1)<<1;
  data[19] += (((word)>>7)&0x1);

  data[20] =  (((word)>>8)&0x1)<<1;
  data[20] += (((word)>>9)&0x1);
  data[21] =  (((word)>>10)&0x1)<<1;
  data[21] += (((word)>>11)&0x1);
  data[22] =  (((word)>>12)&0x1)<<1;
  data[22] += (((word)>>13)&0x1);
  data[23] =  (((word)>>14)&0x1)<<1;
  data[23] += (((word)>>15)&0x1);
  data[24] =  (((word)>>16)&0x1)<<1;
  data[24] += (((word)>>17)&0x1);

  data[25] =  (((word)>>18)&0x1)<<1;
  data[25] += (((word)>>19)&0x1);
  data[26] =  (((word)>>20)&0x1)<<1;
  data[26] += (((word)>>21)&0x1);
  data[27] =  (((word)>>22)&0x1)<<1;
  data[27] += (((word)>>23)&0x1);
  data[28] =  (((word)>>24)&0x1)<<1;
  data[28] += (((word)>>25)&0x1);
  data[29] =  (((word)>>26)&0x1)<<1;
  data[29] += (((word)>>27)&0x1);

  data[30] =  (((word)>>28)&0x1)<<1;
  data[30] += (((word)>>29)&0x1);
  data[31] =  (((word)>>30)&0x1)<<1;
  data[31] += (((word)>>31)&0x1);

  word = *(bufpt+1);
  data[32] =  (((word)>>0)&0x1)<<1;
  data[32] += (((word)>>1)&0x1);
  data[33] =  (((word)>>2)&0x1)<<1;
  data[33] += (((word)>>3)&0x1);
  data[34] =  (((word)>>4)&0x1)<<1;
  data[34] += (((word)>>5)&0x1);

  data[35] =  (((word)>>6)&0x1)<<1;
  data[35] += (((word)>>7)&0x1);
  data[36] =  (((word)>>8)&0x1)<<1;
  data[36] += (((word)>>9)&0x1);
  data[37] =  (((word)>>10)&0x1)<<1;
  data[37] += (((word)>>11)&0x1);
  data[38] =  (((word)>>12)&0x1)<<1;
  data[38] += (((word)>>13)&0x1);
  data[39] =  (((word)>>14)&0x1)<<1;
  data[39] += (((word)>>15)&0x1);
}



decode_state_t Add1BitOfData(uint32_t gbtdata, DualSampa& dsr, DualSampaGroup* dsg)
{
  decode_state_t result = DECODE_STATE_UNKNOWN;
  if( gPrintLevel >= 2 ) fprintf(flog,"ds->status=%d\n", dsr.status);
  if (!(dsr.status == notSynchronized)) { // data is synchronized => build the data word
    dsr.data += (gbtdata&0x1) * dsr.powerMultiplier;
    dsr.powerMultiplier *= 2;
    dsr.bit++;
  }

  DualSampa* ds = &dsr;

  switch(ds->status) {
  case notSynchronized: {
    // Looking for Sync word (2 packets)
    // Look for 10 consecutives 01 (sent 10 from the GBT)
    if( gPrintLevel >= 2 ) fprintf(flog,"  ds[%d]->bit=%d\n  ->powerMultiplier=%lu\n  (gbtdata&0x1)=%d\n",
        ds->id, ds->bit,ds->powerMultiplier, (int)(gbtdata&0x1));
    if (ds->bit < 50) { // Fill the word
      ds->data += (gbtdata&0x1) * ds->powerMultiplier;
      //if( gPrintLevel >= 2 ) fprintf(flog,"Add1BitOfData()\n  ds[%d]->data=%llX\n", ds->id, ds->data);
      ds->powerMultiplier *= 2;
      ds->bit++;
    } else {
      if (ds->bit == 50) ds->powerMultiplier /= 2; // We want to fill the bit 49
      ds->data /=2;  // Take out the bit 0
      ds->data &= 0x1FFFFFFFFFFFF;
      ds->data += (gbtdata&0x1) * ds->powerMultiplier;  // Fill bit 49
      //if( gPrintLevel >= 2 ) fprintf(flog,"Add1BitOfData()\n  ds[%d]->data=%llX\n", ds->data);
      ds->bit++;
    }

    if( gPrintLevel >= 2 ) fprintf(flog,"  ==> ds[%d]->data: %.16lX\n",ds->id, ds->data);
    if (ds->data == 0x1555540f00113 && ds->bit >= 50) {
      //ds->nbHit=0;
      if (gPrintLevel >= 1) fprintf(flog, "SAMPA #%d: Synchronizing... (Sync word found)\n", ds->id); // Next word of 50 bits should be a Sync Word
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
    if( gPrintLevel >= 2 ) fprintf(flog,"  ds[%d]->bit=%d\n  ->powerMultiplier=%lu\n  (gbtdata&0x1)=%d\n",
        dsr.id, dsr.bit,dsr.powerMultiplier, (int)(gbtdata&0x1));
    if( gPrintLevel >= 2 ) fprintf(flog,"  ==> ds[%d]->data: %.16lX\n",dsr.id, dsr.data);
    if(dsr.bit < 50) break;
    if (dsr.data == 0x1555540f00113) {
      if( gPrintLevel >= 2 ) fprintf(flog,"SAMPA #%d: Sync word found\n", dsr.id); // Next word of 50 bits should be a Sync Word
      //dsr.chan_addr[0] = 0;
      //dsr.chan_addr[1] = 0;
      result = DECODE_STATE_SYNC_FOUND;
    } else {
      result = DECODE_STATE_HEADER_FOUND;
      memcpy(&(ds->header),&(ds->data),sizeof(Sampa::SampaHeaderStruct));
      //if( gPrintLevel >= 1 ) fprintf(flog,"SAMPA #%d: ds->nbHit=%d\n", ds->id, ds->nbHit);
      if(ds->nbHit>=0) // if chip was synchronized
        ds->nbHit++;
      else
        ds->nbHit=1;
      if((ds->header.fChannelAddress>=0)&&(ds->header.fChannelAddress<32)) {
        //fprintf(flog,"%.2d %.2d\n",ds->header.fChipAddress,(ds->header.fChipAddress%2));
        ds->nbHitChan[ds->header.fChannelAddress+32*(ds->header.fChipAddress%2)]++;
      }
      if( gPrintLevel >= 1 || (false && ds->id==0 && ds->header.fChipAddress==0 && ds->header.fChannelAddress>=30))
        fprintf(flog,"SAMPA [%2d]: Header 0x%014lx HCode %2lu HPar %lu PkgType %lu 10BitWords %lu ChipAdd %lu ChAdd %2lu BX %lu PPar %d\n",
            ds->id,ds->data,ds->header.fHammingCode,ds->header.fHeaderParity,ds->header.fPkgType,
            ds->header.fNbOf10BitWords,ds->header.fChipAddress,ds->header.fChannelAddress,
            ds->header.fBunchCrossingCounter,(int)ds->header.fPayloadParity);
      int parity = CheckDataParity(ds->data);
      if (parity) fprintf(flog,"===> SAMPA [%2d]: WARNING Parity %d\n",ds->id,parity);

      //fprintf(flog,"SAMPA [%2d]: ChipAdd %d ChAdd %2d BX %d, expected %d\n",
      //              ds->id, ds->header.fChipAddress,ds->header.fChannelAddress,
      //              ds->header.fBunchCrossingCounter, ds->bxc);
      int link = ds->id / 5;
      if( gPrintLevel >= 1 ) fprintf(flog,"SAMPA [%2d]: BX counter for link %d is %lu\n", ds->id, link, dsg->bxc);
      if( false && dsg && dsg->bxc >= 0 ) {
        if( !BXCNT_compare(dsg->bxc, ds->header.fBunchCrossingCounter) ) {
          gNbErrors++;
          fprintf(flog,"===> ERROR SAMPA [%2d]: ChipAdd %lu ChAdd %2lu BX %lu, expected %lu, diff %lu\n",
              ds->id, ds->header.fChipAddress,ds->header.fChannelAddress,
              ds->header.fBunchCrossingCounter, dsg->bxc,
              ds->header.fBunchCrossingCounter-dsg->bxc);
        }
      } else {
        if (ds->header.fPkgType == 4) { // physics trigger
          dsg->bxc = ds->header.fBunchCrossingCounter;
          if( gPrintLevel >= 1 ) fprintf(flog,"SAMPA [%2d]: BX counter for link %d set to %lu\n", ds->id, link, dsg->bxc);
        }
      }
      //if( gPrintLevel >= 1 ) fprintf(flog,"SAMPA [%2d]: BX counter for link %d is %d (2)\n", ds->id, link, dsg->bxc);

      ds->packetsize = 0;

      unsigned int buf[2];
      buf[0] = ds->data&0x3FFFFFFF;
      buf[1] = ds->data>>30;
      bool hamming_error = false;  // Is there an hamming error?
      bool hamming_uncorr = false; // Is the data correctable?
      bool hamming_enable = false; // Correct the data?
      HammingDecode(buf, hamming_error,hamming_uncorr, hamming_enable);
      if (hamming_error) {
        gNbErrors++;
        fprintf(flog,"SAMPA [%2d]: Hamming ERROR -> Correctable: %s\n",ds->id,hamming_uncorr ? "NO" : "YES" );
        ds->status = notSynchronized;
        result = DECODE_STATE_UNKNOWN;
      } else { // No Hamming error
        if (ds->header.fPkgType == 4) { // Good data
          ds->status = sizeToRead;
          ds->bxc[ds->header.fChipAddress%2] = ds->header.fBunchCrossingCounter;
        } else {
          if (ds->header.fPkgType == 1 || ds->header.fPkgType == 3) {  // Data truncated
            gNbErrors++;
            fprintf(flog,"ERROR: Truncated data found -> skip the data\n");
            if (ds->header.fNbOf10BitWords)
              ds->status = dataToRead;
            else
              ds->status = headerToRead;
          }
          if (ds->header.fPkgType == 0) { // Heartbeat: Pkg 0, NbOfWords 0 ?, ChAdd 21
            gNbErrors++;
            fprintf(flog,"ERROR: Hearbeat word found\n");
            ds->status = headerToRead;
            ds->bxc[ds->header.fChipAddress%2] = ds->header.fBunchCrossingCounter;
          }
          if (ds->header.fPkgType == 5) { //
            gNbErrors++;
            fprintf(flog,"ERROR: Data word (?) type 5 found\n");
            ds->status = headerToRead;
          }
          if (ds->header.fPkgType == 6) { //
            if( gPrintLevel >= 1 ) fprintf(flog,"INFO: Trigger too early word found\n");
            //ds->status = headerToRead;
            ds->status = sizeToRead;
          }
          if (ds->header.fPkgType == 2) { //
            gNbErrors++;
            fprintf(flog,"ERROR: Supposed to be a SYNC!!!\n");
            fprintf(flog,"Trying to re-synchronise...\n");
            ds->status = notSynchronized;
            result = DECODE_STATE_UNKNOWN;
          }
        }
      }
    }

    if(  ds->status != notSynchronized ) {
      ds->bit = 0;
      ds->data = 0;
      ds->powerMultiplier = 1;
      //ds->status = synchronized;
    }
    break;
  }
  case sizeToRead: {
    if (ds->bit < 10) break;

    result = DECODE_STATE_CSIZE_FOUND;

    int chip0 = (ds->id%5)*2;
    int chip1 = chip0+1;

    if( gPrintLevel >= 5 )
      fprintf(flog,"SAMPA: chip addresses: %lu\n", ds->header.fChipAddress);
    if( gPrintLevel >= 5 )
      fprintf(flog,"SAMPA: channel addresses: %d, %d\n",
          ds->chan_addr[0], ds->chan_addr[1]);
    if( ds->header.fChipAddress < chip0 || ds->header.fChipAddress > chip1 ) {
      gNbWarnings++;
      fprintf(flog,"===> WARNING SAMPA [%2d]: chip address = %lu, expected = [%d,%d]\n",ds->id,
          ds->header.fChipAddress, chip0, chip1);
    }
    if( ds->chan_addr[ds->header.fChipAddress-chip0] != ds->header.fChannelAddress) {
      gNbWarnings++;
      fprintf(flog,"===> WARNING SAMPA [%2d]: channel address = %lu, expected = %d\n",ds->id,
          ds->header.fChannelAddress, ds->chan_addr[ds->header.fChipAddress-chip0]);
    }
    ds->chan_addr[ds->header.fChipAddress-chip0] += 1;
    if( ds->chan_addr[ds->header.fChipAddress-chip0] > 31 ) {
      ds->chan_addr[ds->header.fChipAddress-chip0] = 0;
    }
    if( gPrintLevel >= 5 )
      fprintf(flog,"SAMPA: next channel addresses: %d, %d\n",
          ds->chan_addr[0], ds->chan_addr[1]);

    if( gPrintLevel >= 1 ) fprintf(flog,"SAMPA [%2d]: Cluster Size 0x%lX (%lu)\n",ds->id,ds->data,ds->data);

    //        MuTrkSampaCluster* sampacluster;
    //            if (fMode == kChargeSum)
    //                sampacluster = new MuTrkSampaCluster(1,-1); // ONLY ONE DATA Time to be filled later
    //            else
    //                sampacluster = new MuTrkSampaCluster(ds->data,-1); // Time to be filled later

    //        sampacluster = new MuTrkSampaCluster(ds->data,-1);
    //        fSampaPacket[fNPackets-1]->AddCluster(sampacluster);
    ds->csize = ds->data;
    ds->cid = 0;
    ds->packetsize += 1;
    ds->status = timeToRead;

    if(  ds->status != notSynchronized ) {
      ds->bit = 0;
      ds->data = 0;
      ds->powerMultiplier = 1;
    }

    break;
  }
  case timeToRead: {  // Read Time Count (10 bits)
    if (ds->bit < 10) break;
    result = DECODE_STATE_CTIME_FOUND;
    if( gPrintLevel >= 1 ) fprintf(flog,"SAMPA [%2d]: Cluster Time 0x%lX (%lu)\n",ds->id,ds->data,ds->data);

    //          MuTrkSampaPacket* currentpacket = fSampaPacket[fNPackets-1];
    //          MuTrkSampaCluster* currentcluster = currentpacket->fCluster[currentpacket->fNClusters-1];
    //          currentcluster->fTime = ds->data;
    ds->ctime = ds->data;
    ds->packetsize += 1;
    ds->status = dataToRead;
    //ds->status = chargeToRead;

    if(  ds->status != notSynchronized ) {
      ds->bit = 0;
      ds->data = 0;
      ds->powerMultiplier = 1;
    }

    break;
  }
  case dataToRead: { // Read ADC data words (10 bits)
    if (ds->bit < 10) break;
    if( gPrintLevel >= 1 ) fprintf(flog,"SAMPA #%d Data word: 0x%lX (%lu)\n",ds->id,ds->data,ds->data);

    if (1 /*ds->header.fPkgType == 4*/) {
      //              MuTrkSampaPacket* currentpacket = fSampaPacket[fNPackets-1];
      //              MuTrkSampaCluster* currentcluster = currentpacket->fCluster[currentpacket->fNClusters-1];
      //              currentcluster->fData[(currentcluster->fDataIndex)++] = ds->data;
      if (ds->header.fPkgType == 4) { // Good data
        result = DECODE_STATE_SAMPLE_FOUND;
        //ds->pedestal[ds->header.fChipAddress%2][ds->header.fChannelAddress] += ds->data;
        //ds->noise[ds->header.fChipAddress%2][ds->header.fChannelAddress] += ds->data*ds->data;
        //ds->ndata[ds->header.fChipAddress%2][ds->header.fChannelAddress] += 1;
        ds->sample = ds->data;

        if( gPattern > 0 ) {
          int patt = (gPattern & 0xFF) + (gPattern<<8 & 0xFF00);
          if( (ds->data & 0x2FF) != (patt & 0x2FF) ) {
            gNbWarnings++;
            fprintf(flog,"===> WARNING SAMPA [%2d]: wrong data pattern 0x%lX, expected 0x%X\n",ds->id,
                ds->data & 0x2FF, (patt & 0x2FF));
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
        fprintf(flog,"===> ERROR SAMPA [%2d]: End-of-packet without End-of-cluster. packet size = %lu, cluster size = %d\n",
            ds->id, ds->header.fNbOf10BitWords, ds->csize);
        ds->status = headerToRead;
      } else if (end_of_cluster) {
        if( gPrintLevel >= 1 ) fprintf(flog,"SAMPA #%d : End of cluster found\n",ds->id);
        if (ds->header.fPkgType == 4) { // Good data
          ds->nclus[ds->header.fChipAddress%2][ds->header.fChannelAddress] += 1;
          result = DECODE_STATE_END_OF_CLUSTER;
          if( ds->id==0 && ds->header.fChipAddress==0 && ds->header.fChannelAddress>=30 ) {
            if( false ) {
              if(ds->header.fChannelAddress==31) fprintf(flog,"    ");
              fprintf(flog,"%d %lu %lu: End of cluster found (%d)\n",ds->id, ds->header.fChipAddress, ds->header.fChannelAddress,
                  ds->nclus[ds->header.fChipAddress%2][ds->header.fChannelAddress]);
            }
          }
        }
        //                    if (currentpacket->fHeader.fNbOf10BitWords > currentcluster->fSize+2)
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
        fprintf(flog,"WARNING: SAMPA PkgType = 1 or 3 (data truncated)  found\n");
        if (ds->header.fNbOf10BitWords-1)
          ds->header.fNbOf10BitWords--;
        else
          ds->status = headerToRead;
      }
    }

    if(  ds->status != notSynchronized ) {
      ds->bit = 0;
      ds->data = 0;
      ds->powerMultiplier = 1;
    }

    break;
  }
  default: break;
  }

  return result;
}



MuonChambersDataDecoder::MuonChambersDataDecoder() { }

MuonChambersDataDecoder::~MuonChambersDataDecoder() {fclose(flog);}

void MuonChambersDataDecoder::initialize()
{
  QcInfoLogger::GetInstance() << "initialize MuonChambersDataDecoder" << AliceO2::InfoLogger::InfoLogger::endm;

  hb_orbit = -1;
  nFrames = 0;

  for(int c = 0; c < 24; c++) {
    for(int i = 0; i < 40; i++) {
      DualSampaInit( &(ds[c][i]) );
      ds[c][i].id = i;
      ds[c][i].nbHit = -1;
      for(int j=0;j<64;j++) {
        ds[c][i].nbHitChan[j]=0;
      }
    }
    for(int i = 0; i < 8; i++) {
      DualSampaGroupInit( &(dsg[c][i]) );
    }
  }

  gPrintLevel = 0;

  flog = stdout; 
  //flog = fopen("/home/flp/qc.log", "w");
}

void MuonChambersDataDecoder::processData(const char* buf, size_t size)
{

  // exemplary ways of accessing inputs (incoming data), that were specified in the .ini file - e.g.:
  //  [readoutInput]
  //  inputName=readout
  //  dataOrigin=ITS
  //  dataDescription=RAWDATA
  const int RDH_BLOCK_SIZE = 8192;


  const char* rdh = buf;
  uint32_t payload_offset = 0;
  uint32_t CRUbuf[4*4];
  CRUheader CRUh;
  if( size < sizeof(CRUbuf) ) return;

  while( payload_offset < size ) {

    //fprintf(flog, "CRU payload_offset: %d\n", (int)payload_offset);

    memcpy(CRUbuf,rdh,sizeof(CRUbuf));
    memcpy(&CRUh,CRUbuf,sizeof(CRUheader));

    uint32_t* payload_buf = (uint32_t*)(rdh + 16*4);

    rdh += RDH_BLOCK_SIZE;
    payload_offset += RDH_BLOCK_SIZE;

    if(gPrintLevel>=3) fprintf(flog,"%d:  header_version: %X, header_size: %d, memory_size: %d, block_length: %d, packet: %d, link_id: %d, orbit: %d\n",
        nFrames, (int)CRUh.header_version, (int)CRUh.header_size, (int)CRUh.memory_size, (int)CRUh.block_length,
        (int)CRUh.packet_counter, (int)CRUh.link_id, (int)CRUh.hb_orbit);

    // Check RDH version and size
    if( ((int)CRUh.header_version) != 4 ) {
      fprintf(flog, "Wrong CRU header version: %d\n", (int)CRUh.header_version);
      continue;
    }
    if( ((int)CRUh.header_size) != 64 ) {
      fprintf(flog, "Wrong CRU header size: %d\n", (int)CRUh.header_size);
      continue;
    }
    // Compute size of payload inside 8kB block
    CRUh.block_length = CRUh.memory_size - CRUh.header_size;

    int cruId = CRUh.source_id & 0xFFF;
    int dpwId = (CRUh.source_id >> 12) & 0xF;

    //fprintf(flog, "CRU header: %08X %08X %08X %08X \n", CRUbuf[0], CRUbuf[1], CRUbuf[2], CRUbuf[3]);
    //fprintf(flog, "CRU header: %08X %08X %08X %08X \n", CRUbuf[4], CRUbuf[5], CRUbuf[6], CRUbuf[7]);
    //fprintf(flog, "CRU header: %08X %08X %08X %08X \n", CRUbuf[8], CRUbuf[9], CRUbuf[10], CRUbuf[11]);
    //fprintf(flog, "CRU header: %08X %08X %08X %08X \n", CRUbuf[12], CRUbuf[13], CRUbuf[14], CRUbuf[15]);
    //fprintf(flog, "CRU header version: %d\n", (int)CRUh.header_version);
    //fprintf(flog, "CRU header size: %d\n", (int)CRUh.header_size);
    //fprintf(flog, "CRU header block length: %d\n", (int)CRUh.block_length);
    if(gPrintLevel>=3) fprintf(flog, "CRU packet counter: %d\n", (int)CRUh.packet_counter);
    if(gPrintLevel>=3) fprintf(flog, "CRU orbit id: %d\n", (int)CRUh.hb_orbit);
    if(gPrintLevel>=1) fprintf(flog, "CRU link ID: %d\n", (int)CRUh.link_id);
    if(gPrintLevel>=1) fprintf(flog, "CRU ID: %d\n", (int)cruId);
    if(gPrintLevel>=1) fprintf(flog, "DPW ID: %d\n", (int)dpwId);
    //fprintf(flog, "\n");
    //continue;

    nFrames += 1;

    int cru_lid = CRUh.link_id;
    bool orbit_jump = true;
    int Dorbit1 = CRUh.hb_orbit - hb_orbit;
    int Dorbit2 = (uint32_t)( ((uint64_t)CRUh.hb_orbit) + 0x100000000 - hb_orbit );
    if( Dorbit1 >=0 && Dorbit1 <= 1 ) orbit_jump = false;
    if( Dorbit2 >=0 && Dorbit2 <= 1 ) orbit_jump = false;
    if( orbit_jump ) {
      if(gPrintLevel>=3) fprintf(flog, "Resetting decoding FSM: orbit=%d, previous=%d\n", CRUh.hb_orbit, hb_orbit);
      for(int i = 0; i < 40; i++) {
        DualSampaReset( &(ds[cru_lid][i]) );
        ds[cru_lid][i].id = i;
        ds[cru_lid][i].nbHit = -1;
        for(int j=0;j<64;j++) {
          ds[cru_lid][i].nbHitChan[j]=0;
        }
      }
      for(int i = 0; i < 8; i++) {
        DualSampaGroupReset( &(dsg[cru_lid][i]) );
      }
    }
    hb_orbit = CRUh.hb_orbit;

    //uint32_t* payload_buf = (uint32_t*)(rdh + 16*4);
    uint32_t hhvalue,hlvalue,lhvalue,llvalue;
    int nGBTwords = CRUh.block_length / 16;
    //fprintf(stdout,"nGBTwords=%d\n",nGBTwords);
    //continue;
    for( int wi = 0; wi < nGBTwords; wi++) {
      uint32_t* ptr = payload_buf + wi*4;
      hhvalue = ptr[3]; hlvalue = ptr[2]; lhvalue = ptr[1]; llvalue = ptr[0];
      //fprintf(stdout,"wi=%d  %.8X %.8X %.8X %.8X\n", wi,
      //    hhvalue, hlvalue, lhvalue, llvalue);
      //printf("%.2X\n", (llvalue&0x3));

      uint32_t bufpt[4] = {hhvalue, hlvalue, lhvalue, llvalue};
      uint32_t data2bits[40] = {0};
      DecodeGBTWord(bufpt, data2bits);
      //continue;

      //fprintf(stdout,"GBT word %d decoded\n", wi);

      //cru_lid = 0;
      for( int i = 0; i < 1; i++ ) {
        uint32_t group = ds[cru_lid][i].id / 5;
        uint32_t bits[2] = {data2bits[i]&0x1, (data2bits[i]>>1)&0x1};
        for(int k = 0; k < 2; k++) {
          decode_state_t state = Add1BitOfData( bits[k], (ds[cru_lid][i]), &(dsg[cru_lid][group]) );
          switch(state) {
          case DECODE_STATE_SYNC_FOUND: break;
          case DECODE_STATE_HEADER_FOUND:
            uint64_t _h; memcpy(&_h, &(ds[cru_lid][i].header), sizeof(ds[cru_lid][i].header));
            if(gPrintLevel>=1) fprintf(flog,"HEADER: %05lX\n", _h);
            break;
          case DECODE_STATE_CSIZE_FOUND: {
            if(gPrintLevel>=1) fprintf(flog,"CLUSTER SIZE: %d\n",ds[cru_lid][i].csize);
            Sampa::SampaHeaderStruct& header = ds[cru_lid][i].header;
            SampaHit& hit = ds[cru_lid][i].hit;
            hit.cru_id = cruId;
            hit.link_id = cru_lid;
            if(dpwId != 0) hit.link_id += 12;
            hit.ds_addr = ds[cru_lid][i].id;
            int chip_id = ds[cru_lid][i].header.fChipAddress % 2;
            hit.chan_addr = header.fChannelAddress + 32*chip_id;
            hit.bxc = header.fBunchCrossingCounter;
            hit.size = ds[cru_lid][i].csize;
            hit.samples.clear();
            hit.csum = 0;
            hit.time = 0;
            break;
          }
          case DECODE_STATE_CTIME_FOUND:
            if(gPrintLevel>=1) fprintf(flog,"CLUSTER TIME: %d\n",ds[cru_lid][i].ctime);
            ds[cru_lid][i].hit.time = ds[cru_lid][i].ctime;
            break;
          case DECODE_STATE_SAMPLE_FOUND:
          case DECODE_STATE_END_OF_CLUSTER: {
            SampaHit& hit = ds[cru_lid][i].hit;
            if(gPrintLevel>=1) fprintf(flog,"SAMPLE: %X\n",ds[cru_lid][i].sample);
            hit.samples.push_back(ds[cru_lid][i].sample);
            hit.csum += ds[cru_lid][i].sample;

            if( state == DECODE_STATE_END_OF_CLUSTER ) {
              mHits.push_back(hit);
              if(hit.link_id>=24) {
                fprintf(stdout,"hit: link_id=%d, ds_addr=%d, chan_addr=%d\n",
                    hit.link_id, hit.ds_addr, hit.chan_addr);
                getchar();
              }
              //fprintf(stdout,"mHits.size()=%d\n",(int)mHits.size());
              hit.size = 0;
              hit.samples.clear();
              hit.csum = 0;
              hit.time = 0;
            }
            break;
          }
          default: break;
          }
        }
      }
    }
  }
}

void MuonChambersDataDecoder::clearHits()
{
  mHits.clear();
}

void MuonChambersDataDecoder::reset()
{
  clearHits();
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
