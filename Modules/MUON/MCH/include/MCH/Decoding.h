///
/// \file   Decoding.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_DATA_DECODER_H
#define QC_MODULE_MUONCHAMBERS_DATA_DECODER_H

#include "QualityControl/TaskInterface.h"
#include "MCH/sampa_header.h"
#include "MCH/Mapping.h"
#include "MCHBase/Digit.h"

using namespace o2::quality_control::core;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

enum DualSampaStatus {
  notSynchronized = 1,
  //synchronized = 2,
  headerToRead = 3,
  sizeToRead = 4,
  timeToRead = 5,
  dataToRead = 6
  //chargeToRead = 7,
  //OK = 8 // Data block filled (over a time window)
};

struct SampaHit {
  uint8_t cru_id, fee_id, data_path, link_id, ds_addr, chan_addr;
  int64_t bxc;
  uint32_t size, time;
  std::vector<uint16_t> samples;
  uint64_t csum;
  MapPad pad;
};

struct DualSampa {
  int id;
  DualSampaStatus status;          // Status during the data filling
  uint64_t data;                   // curent data
  int bit;                         // current position
  uint64_t powerMultiplier;        // power to convert to move bits
  int nsyn2Bits;                   // Nb of words waiting synchronization
  Sampa::SampaHeaderStruct header; // current channel header
  unsigned long bxc[2];
  uint32_t csize, ctime, cid, sample;
  int chan_addr[2];
  uint64_t packetsize;
  int nbHit;         // incremented each time a header packet is received for this card
  int nbHitChan[64]; // incremented each time a header packet for a given packet is received for this card
  int ndata[2][32];
  int nclus[2][32];
  double pedestal[2][32], noise[2][32];
  SampaHit hit;
};

struct DualSampaGroup {
  long int bxc;
};

/// \brief decoding of MCH data
/// \author Andrea Ferrero
class Decoder
{
 public:
  /// \brief Constructor
  Decoder();
  /// Destructor
  ~Decoder();

  // Definition of the methods for the template method pattern
  void initialize();
  void processData(const char* buf, size_t size);
  void decodeRaw(uint32_t* payload_buf, size_t nGBTwords, int cru_id, int link_id);
  void decodeUL(uint32_t* payload_buf, size_t nWords, int cru_id, int dpw_id);
  void clearHits();
  void clearDigits();
  std::vector<SampaHit>& getHits() { return mHits; }
  std::vector<o2::mch::Digit>& getDigits() { return mDigits; }
  void reset();

  int32_t getMapCRU(int cruid, int linkid) { return mMapCRU.getLink(cruid, linkid); }
  int32_t getMapFEC(uint32_t link_id, uint32_t ds_addr, uint32_t& de, uint32_t& dsid)
  {
    if (!mMapFEC.getDSMapping(link_id, ds_addr, de, dsid))
      return -1;
    return de;
  }
  MapFEC& getMapFEC() { return mMapFEC; }

 private:
  int hb_orbit;
  DualSampa ds[MCH_MAX_CRU_ID][24][40];
  DualSampaGroup dsg[MCH_MAX_CRU_ID][24][8];
  int ds_enable[MCH_MAX_CRU_IN_FLP][24][40];
  std::vector<SampaHit> mHits;
  std::vector<o2::mch::Digit> mDigits;
  int nFrames;
  MapCRU mMapCRU;
  MapFEC mMapFEC;
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_DATA_DECODER_H
