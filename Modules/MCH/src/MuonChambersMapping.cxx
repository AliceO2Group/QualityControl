///
/// \file   RawDataProcessor.cxx
/// \author Andrea Ferrero
///

#include "QualityControl/QcInfoLogger.h"
#include "MCH/MuonChambersMapping.h"
#include "MCHMappingInterface/Segmentation.h"

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
  if (!file)
  {
    std::cerr << "Can't open file "<<mapFile<<std::endl;
    return false;
  }

  int c, l, link_id;
  char tstr[500];
  while( file.getline(tstr,499) ) {
    std::string s(tstr);
    std::istringstream line(s);
    printf("[MapCRU::readMapping]: line: %s\n", tstr);
    line >> link_id >> c >> l;
    printf("[MapCRU::readMapping]: %d %d -> %d\n", c, l, link_id);
    if( c < 0 || c >= MCH_MAX_CRU_IN_FLP ) continue;
    if( l < 0 || l >= 24 ) continue;
    printf("[MapCRU::readMapping]: added %d %d -> %d\n", c, l, link_id);
    mSolarMap[c][l].mLink = link_id;
  }
}


int32_t MapCRU::getLink(uint32_t c, uint32_t l)
{
  int32_t result = -1;
  if( c < 0 || c >= MCH_MAX_CRU_IN_FLP ) return result;
  if( l < 0 || l >= 24 ) return result;
  return mSolarMap[c][l].mLink;
}



/*
 * CRU mapping
 */
MapFEC::MapFEC()
{
  for(int i = 0; i < MCH_DE_MAX; i++) {
    mPadMap[i] = NULL; //new MapPad[MCH_PAD_ADDR_MAX];
  }
}




bool MapFEC::readDSMapping(std::string mapFile)
{
  std::ifstream file;
  file.open(mapFile);
  if (!file)
  {
    std::cerr << "Can't open file "<<mapFile<<std::endl;
    return false;
  }

  int link_id, group_id, de, ds_id[5];
  while(!file.eof())
  {
    file >> link_id >> group_id >> de >> ds_id[0] >> ds_id[1] >> ds_id[2] >> ds_id[3] >> ds_id[4];
    if( link_id < 0 || link_id > LINKID_MAX ) continue;
    for(int i = 0; i < 5; i++) {
      if(ds_id[i] <= 0) continue;
      int ds_addr = group_id*5 + i;
      if( ds_addr < 0 || ds_addr >= 40 ) continue;
      mDsMap[link_id][ds_addr].mDE = de;
      mDsMap[link_id][ds_addr].mIndex = ds_id[i];
      mDsMap[link_id][ds_addr].mBad = 0;
    }
  }
  return true;
}


bool MapFEC::readPadMapping(uint32_t de, std::string bMapfile, std::string nbMapfile, bool newMapping)
{
  int manu2ds[64]={62,61,63,60,59,55,58,57,56,54,50,46,42,39,37,41,
      35,36,33,34,32,38,43,40,45,44,47,48,49,52,51,53,
      7, 6, 5, 4, 2, 3, 1, 0, 9,11,13,15,17,19,21,23,
      31,30,29,28,27,26,25,24,22,20,18,16,14,12,10, 8};

  int manuch,dsid,dsch;
  int address;
  int padx,pady;
  float x,y;
  std::ifstream filebend,filenbend;

  filebend.open(bMapfile);
  filenbend.open(nbMapfile);
  if( !filebend || !filenbend ) return false;

  if( mPadMap[de] == NULL ) {
    mPadMap[de] = new MapPad[MCH_PAD_ADDR_MAX];
  }

  //    sprintf(dsbfile,"slat330000N.Bending.map");
  //    sprintf(dsnbfile,"slat330000N.NonBending.map");

  std::cout<<"ReadPadMapping for DE "<<de<<" from file "<<bMapfile<<std::endl;
  if (!filebend)
  {
    std::cerr << "Can't open file "<<bMapfile<<std::endl;
    return false;
  } 

  while(!filebend.eof())
  {
    filebend >> address >> padx >> pady
    >> x >> y >> dsid >> manuch;

    if (newMapping)
      dsch = manuch;
    else
      dsch = manu2ds[manuch];

    // NOT USE THE ADDRESS FIELD DIRECTLY
    address = dsch + (dsid<<6);
    mPadMap[de][address].fDE = de;
    mPadMap[de][address].fDsID = dsid;
    mPadMap[de][address].fAddress = address;
    mPadMap[de][address].fPadx = padx;
    mPadMap[de][address].fPady = pady;
    mPadMap[de][address].fX = x;
    mPadMap[de][address].fY = y;
    mPadMap[de][address].fSizeX = 10;
    mPadMap[de][address].fSizeY = 0.5;
    mPadMap[de][address].fCathode = 'b';
    cout<<"Bend Manu id "<<dsid<<" ch "<<dsch<<" de "<<de<<"  address "<<address<<endl;
  }
  filebend.close();

  std::cout<<"ReadPadMapping for DE "<<de<<" from file "<<nbMapfile<<std::endl;
  if (!filenbend)
  {
    std::cerr << "Can't open file "<<nbMapfile<<std::endl;
    return false;
  }
  while(!filenbend.eof())
  {
    filenbend >> address >> padx >> pady
    >> x >> y >> dsid >> manuch;

    if (newMapping)
      dsch = manuch;
    else
      dsch = manu2ds[manuch];

    // NOT USE THE ADDRESS FIELD DIRECTLY
    address = dsch + (dsid<<6);
    mPadMap[de][address].fDE = de;
    mPadMap[de][address].fDsID = dsid;
    mPadMap[de][address].fAddress = address;
    mPadMap[de][address].fPadx = padx;
    mPadMap[de][address].fPady = pady;
    mPadMap[de][address].fX = x;
    mPadMap[de][address].fY = y;
    mPadMap[de][address].fCathode = 'n';
    cout<<"NBend Manu id "<<dsid<<" ch "<<dsch<<" de "<<de<<"  address "<<address<<endl;
    //    cout<<"NB Manu id "<<dsid<<" ch "<<dsch<<endl;
  }
  filenbend.close();

  return true;
}

bool MapFEC::readPadMapping2(uint32_t de, bool newMapping)
{
  int manu2ds[64]={62,61,63,60,59,55,58,57,56,54,50,46,42,39,37,41,
      35,36,33,34,32,38,43,40,45,44,47,48,49,52,51,53,
      7, 6, 5, 4, 2, 3, 1, 0, 9,11,13,15,17,19,21,23,
      31,30,29,28,27,26,25,24,22,20,18,16,14,12,10, 8};

  int dsid,dsch;
  int address;
  int padx,pady;
  float x,y;

  if( mPadMap[de] == NULL ) {
    mPadMap[de] = new MapPad[MCH_PAD_ADDR_MAX];
  }

  o2::mch::mapping::Segmentation segment(de);
  for(int pad=0; pad < MCH_PAD_ADDR_MAX; pad++){

    if(!segment.isValid(pad)) continue;

    std::cout<<"Il existe un pad "<<pad<<std::endl;

    dsid = segment.padDualSampaId(pad);
    dsch = segment.padDualSampaChannel(pad);
    padx = segment.padSizeX(pad);
    pady = segment.padSizeY(pad);
    x = segment.padPositionX(pad);
    y = segment.padPositionY(pad);

    if(dsch < 0) continue;

    //    sprintf(dsbfile,"slat330000N.Bending.map");
    //    sprintf(dsnbfile,"slat330000N.NonBending.map");

    std::cout<<"Pad "<<pad<<" dsid "<<dsid<<" dsch "<<dsch<<" padx "<<padx<<" pady "<<pady<< " x "<<x<<" y "<<y<<std::endl;

    // NOT USE THE ADDRESS FIELD DIRECTLY
    if(segment.isBendingPad(pad)){
      address = dsch + (dsid<<6);
      mPadMap[de][address].fDE = de;
      mPadMap[de][address].fDsID = dsid;
      mPadMap[de][address].fAddress = address;
      mPadMap[de][address].fPadx = padx;
      mPadMap[de][address].fPady = pady;
      mPadMap[de][address].fX = x;
      mPadMap[de][address].fY = y;
      mPadMap[de][address].fSizeX = 10;
      mPadMap[de][address].fSizeY = 0.5;
      mPadMap[de][address].fCathode = 'b';
      cout<<"Bend Manu id "<<dsid<<" ch "<<dsch<<" de "<<de<<"  address "<<address<<endl;
    }

    // NOT USE THE ADDRESS FIELD DIRECTLY
    if(!segment.isBendingPad(pad)){
      address = dsch + (dsid<<6);
      mPadMap[de][address].fDE = de;
      mPadMap[de][address].fDsID = dsid;
      mPadMap[de][address].fAddress = address;
      mPadMap[de][address].fPadx = padx;
      mPadMap[de][address].fPady = pady;
      mPadMap[de][address].fX = x;
      mPadMap[de][address].fY = y;
      mPadMap[de][address].fCathode = 'n';
      cout<<"NBend Manu id "<<dsid<<" ch "<<dsch<<" de "<<de<<"  address "<<address<<endl;
      //    cout<<"NB Manu id "<<dsid<<" ch "<<dsch<<endl;
    }

  }
  return true;
}


bool MapFEC::getDSMapping(uint32_t link_id, uint32_t ds_addr, uint32_t& de, uint32_t& dsid)
{
  if( mDsMap[link_id][ds_addr].mBad == 1 ) return false;
  de = mDsMap[link_id][ds_addr].mDE;
  dsid = mDsMap[link_id][ds_addr].mIndex;
  return true;
}



bool MapFEC::getPad(uint32_t cru_link, uint32_t dsid, uint32_t dsch, MapPad& pad)
{
  //printf("getPad: link=%d  dsid=%d  bad=%d\n", cru_link, dsid, mDsMap[cru_link][dsid].mBad);
  if( mDsMap[cru_link][dsid].mBad == 1 ) return false;
  int32_t de = mDsMap[cru_link][dsid].mDE;
  int32_t dsidx = mDsMap[cru_link][dsid].mIndex;
  //printf("getPad: de=%d  dsidx=%d  mPadMap[de]=%p\n", de, dsidx, mPadMap[de]);

  if( mPadMap[de] == NULL ) return false;

  int32_t address = dsch + (dsidx<<6);
  pad = mPadMap[de][address];
  //printf("getPad: address=%d  pad.fDE=%d\n", address, pad.fDE);
  return true;
}


} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
