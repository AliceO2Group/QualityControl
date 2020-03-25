///
/// \file   Mapping.cxx
/// \author Andrea Ferrero
///

#include "QualityControl/QcInfoLogger.h"
#include "MCH/Mapping.h"
//#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingFactory/CreateSegmentation.h"

using namespace std;

#define MCH_PAD_ADDR_MAX 100000

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

/*
 * Solar mapping
 */
MapSolar::MapSolar()
{
  mLink = -1;
}

MapSolar::~MapSolar()
{
}

/*
 * DualSampa mapping
 */
MapDualSampa::MapDualSampa()
{
  mDE = mIndex = -1;
  mBad = 1;
}

MapDualSampa::~MapDualSampa()
{
}

/*
 * Pad mapping
 */
MapPad::MapPad()
{
  fAddress = fPadx = fPady = fBad = 0;
  fX = fY = 0;
  fCathode = 'u';
}

MapPad::~MapPad()
{
}

/*
 * CRU mapping
 */
MapCRU::MapCRU()
{
}

bool MapCRU::readMapping(std::string mapFile)
{
  std::ifstream file;
  file.open(mapFile);
  printf("[MapCRU::readMapping]: opening \"%s\"\n", mapFile.c_str());
  if (!file) {
    std::cerr << "Can't open file " << mapFile << std::endl;
    return false;
  }

  int c, l, link_id;
  char tstr[500];
  while (file.getline(tstr, 499)) {
    std::string s(tstr);
    std::istringstream line(s);
    //printf("[MapCRU::readMapping]: line: %s\n", tstr);
    line >> link_id >> c >> l;
    //printf("[MapCRU::readMapping]: %d %d -> %d\n", c, l, link_id);
    if (c < 0 || c >= MCH_MAX_CRU_IN_FLP)
      continue;
    if (l < 0 || l >= 24)
      continue;
    //printf("[MapCRU::readMapping]: added %d %d -> %d\n", c, l, link_id);
    mSolarMap[c][l].mLink = link_id;
  }
}

int32_t MapCRU::getLink(uint32_t c, uint32_t l)
{
  int32_t result = -1;
  if (c < 0 || c >= MCH_MAX_CRU_IN_FLP)
    return result;
  if (l < 0 || l >= 24)
    return result;
  return mSolarMap[c][l].mLink;
}

/*
 * Electronics mapping
 */
MapFEC::MapFEC()
{
}

bool MapFEC::readDSMapping(std::string mapFile)
{
  std::ifstream file;
  file.open(mapFile);
  if (!file) {
    std::cerr << "Can't open file " << mapFile << std::endl;
    return false;
  }

  int link_id, group_id, de, ds_id[5];
  while (!file.eof()) {
    file >> link_id >> group_id >> de >> ds_id[0] >> ds_id[1] >> ds_id[2] >> ds_id[3] >> ds_id[4];
    if (link_id < 0 || link_id > LINKID_MAX)
      continue;
    for (int i = 0; i < 5; i++) {
      if (ds_id[i] <= 0)
        continue;
      int ds_addr = group_id * 5 + i;
      if (ds_addr < 0 || ds_addr >= 40)
        continue;
      mDsMap[link_id][ds_addr].mDE = de;
      mDsMap[link_id][ds_addr].mIndex = ds_id[i];
      mDsMap[link_id][ds_addr].mBad = 0;
    }
  }
  return true;
}

bool MapFEC::getDSMapping(uint32_t link_id, uint32_t ds_addr, uint32_t& de, uint32_t& dsid)
{
  //printf("getPad: link_id=%d  ds_addr=%d  bad=%d\n", link_id, ds_addr, mDsMap[link_id][ds_addr].mBad);
  if (mDsMap[link_id][ds_addr].mBad == 1)
    return false;
  de = mDsMap[link_id][ds_addr].mDE;
  dsid = mDsMap[link_id][ds_addr].mIndex;
  return true;
}

bool MapFEC::getPadByLinkID(uint32_t link_id, uint32_t ds_addr, uint32_t dsch, MapPad& pad)
{
  uint32_t de, dsid;
  if (!getDSMapping(link_id, ds_addr, de, dsid))
    return false;

  return getPadByDE(de, dsid, dsch, pad);
}

bool MapFEC::getPadByDE(uint32_t de, uint32_t dsid, uint32_t dsch, MapPad& pad)
{
  try {
    const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);

    int padid = segment.findPadByFEE(dsid, dsch);

    float padX = segment.padPositionX(padid);
    float padY = segment.padPositionY(padid);
    float padSizeX = segment.padSizeX(padid);
    float padSizeY = segment.padSizeY(padid);

    pad.fDE = de;
    pad.fDsID = dsid;
    pad.fAddress = padid;
    pad.fX = padX;
    pad.fY = padY;
    pad.fSizeX = padSizeX;
    pad.fSizeY = padSizeY;
    pad.fCathode = segment.isBendingPad(padid) ? 0 : 1;
    pad.fBad = 0;
  } catch (const std::exception& e) {
    return false;
  }

  return true;
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
