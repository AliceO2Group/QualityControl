///
/// \file   RawDataProcessor.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_MAPPING_H
#define QC_MODULE_MUONCHAMBERS_MAPPING_H

#include "MCHMappingInterface/Segmentation.h"

#include "QualityControl/TaskInterface.h"

#define MCH_DE_MAX 2000
#define MCH_DSID_MAX 2047
#define MCH_MAX_CRU_ID 31
#define MCH_MAX_CRU_IN_FLP 31
#define LINKID_MAX 0x7FF

using namespace o2::quality_control::core;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

class MapSolar
{
 public:
  int mLink; // link ID

  MapSolar();
  ~MapSolar();
};

class MapSolarInv
{
 public:
  int mCruId;   // CRU ID
  int mCruLink; // CRU link

  MapSolarInv();
  ~MapSolarInv();
};

class MapDualSampa
{
 public:
  int mDE;    // detector element
  int mIndex; // DS index
  int mBad;   // if = 1 bad pad (not used for analysis)

  MapDualSampa();
  ~MapDualSampa();
};

class MapDualSampaInv
{
 public:
  int mLink;    // detector element
  int mAddress; // DS index

  MapDualSampaInv();
  ~MapDualSampaInv();
};

class MapPad
{
 public:
  int fDE;      // detector element
  int fDsID;    // electronic address
  int fAddress; // electronic address
  int fPadx;    // PadX index
  int fPady;    // PadY index
  float fX;     // x coordinate (cm)
  float fY;     // y coordinate (cm)
  float fSizeX; // dimension along x (cm)
  float fSizeY; // dimension along y (cm)
  int fCathode; // bend 'b'(98), nb 'n'(110), undef 'u'(117)
  int fBad;     // if = 1 bad pad (not used for analysis)

  MapPad();
  ~MapPad();
};

class MapCRU
{

  MapSolar mSolarMap[MCH_MAX_CRU_IN_FLP][24];
  MapSolarInv mSolarMapInv[LINKID_MAX + 1];

 public:
  MapCRU();
  bool readMapping(std::string mapFile);
  int32_t getLink(int32_t c, int32_t l);
  bool getLinkInv(uint32_t link_id, int32_t& c, int32_t& l);
};

class MapFEC
{

  MapDualSampa mDsMap[LINKID_MAX + 1][40];
  MapDualSampaInv mDsMapInv[MCH_DE_MAX + 1][MCH_DSID_MAX + 1];

 public:
  MapFEC();
  bool readDSMapping(std::string mapFile);
  bool getDSMapping(uint32_t link_id, uint32_t ds_addr, uint32_t& de, uint32_t& dsid);
  bool getDSMappingInv(uint32_t de, uint32_t dsid, uint32_t& link_id, uint32_t& ds_addr);
  bool getPadByLinkID(uint32_t link_id, uint32_t ds_addr, uint32_t dsch, MapPad& pad);
  bool getPadByDE(uint32_t de, uint32_t dsid, uint32_t dsch, MapPad& pad);
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_MAPPING_H
