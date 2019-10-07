///
/// \file   RawDataProcessor.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_MAPPING_H
#define QC_MODULE_MUONCHAMBERS_MAPPING_H

#include "MCHMappingInterface/Segmentation.h"

#include "QualityControl/TaskInterface.h"

#define MCH_DE_MAX 2000

using namespace o2::quality_control::core;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{


class MapDualSampa
{
public:
  int mDE;                  // detector element
  int mIndex;               // DS index
  int mBad;                 // if = 1 bad pad (not used for analysis)

  MapDualSampa();
  ~MapDualSampa();
};


class MapPad
{
public:
  int fDE;                  // detector element
  int fDsID;             // electronic address
  int fAddress;             // electronic address
  int fPadx;                // PadX index
  int fPady;                // PadY index
  float fX;                 // x coordinate (cm)
  float fY;                 // y coordinate (cm)
  float fSizeX;             // dimension along x (cm)
  float fSizeY;             // dimension along y (cm)
  char fCathode;            // bend 'b'(98), nb 'n'(110), undef 'u'(117)
  int fBad;                 // if = 1 bad pad (not used for analysis)

  MapPad();
  ~MapPad();
};


class MapCRU
{

  MapDualSampa mDsMap[24][40];
  MapPad* mPadMap[MCH_DE_MAX];

public:
  MapCRU();
  bool addDSMapping(uint32_t link_id, uint32_t ds_addr, uint32_t de, uint32_t dsid);
  bool readDSMapping(uint32_t cru_id, std::string mapFile);
  bool getDSMapping(uint32_t link_id, uint32_t ds_addr, uint32_t& de, uint32_t& dsid);
  bool readPadMapping(uint32_t de, std::string bMapfile, std::string nbMapfile, bool newMapping);
  bool getPad(uint32_t cru_link, uint32_t dsid, uint32_t dsch, MapPad& pad);
};


} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_MAPPING_H
