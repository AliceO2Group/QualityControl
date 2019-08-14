///
/// \file   RawDataProcessor.cxx
/// \author Andrea Ferrero
///

#include "QualityControl/QcInfoLogger.h"
#include "MuonChambers/MuonChambersMapping.h"

using namespace std;


#define MCH_PAD_ADDR_MAX 100000


namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{


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
  for(int i = 0; i < MCH_DE_MAX; i++) {
    mPadMap[i] = NULL; //new MapPad[MCH_PAD_ADDR_MAX];
  }
}


bool MapCRU::addDSMapping(uint32_t link_id, uint32_t ds_addr, uint32_t de, uint32_t dsid)
{
  MapDualSampa& m = mDsMap[link_id][ds_addr];
  m.mDE = de;
  m.mIndex = dsid;
  m.mBad = 0;
}




bool MapCRU::readPadMapping(uint32_t de, std::string bMapfile, std::string nbMapfile, bool newMapping)
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
  char dsbfile[256], dsnbfile[256];

  if( mPadMap[de] == NULL ) {
    mPadMap[de] = new MapPad[MCH_PAD_ADDR_MAX];
  }

  //    sprintf(dsbfile,"slat330000N.Bending.map");
  //    sprintf(dsnbfile,"slat330000N.NonBending.map");

  std::cout<<"ReadPadMapping for DE "<<de<<" from file "<<bMapfile<<std::endl;
  filebend.open(bMapfile);
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
    mPadMap[de][address].fAddress = address;
    mPadMap[de][address].fPadx = padx;
    mPadMap[de][address].fPady = pady;
    mPadMap[de][address].fX = x;
    mPadMap[de][address].fY = y;
    mPadMap[de][address].fSizeX = 10;
    mPadMap[de][address].fSizeY = 0.5;
    mPadMap[de][address].fCathode = 'b';
    //    cout<<"Bend Manu id "<<dsid<<" ch "<<dsch<<endl;
  }
  filebend.close();

  std::cout<<"ReadPadMapping for DE "<<de<<" from file "<<nbMapfile<<std::endl;
  filenbend.open(nbMapfile);
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
    mPadMap[de][address].fAddress = address;
    mPadMap[de][address].fPadx = padx;
    mPadMap[de][address].fPady = pady;
    mPadMap[de][address].fX = x;
    mPadMap[de][address].fY = y;
    mPadMap[de][address].fCathode = 'n';
    //    cout<<"NB Manu id "<<dsid<<" ch "<<dsch<<endl;
  }
  filenbend.close();

  return true;
}



bool MapCRU::getPad(uint32_t cru_link, uint32_t dsid, uint32_t dsch, MapPad& pad)
{
  if( mDsMap[cru_link][dsid].mBad == 1 ) return false;
  int32_t de = mDsMap[cru_link][dsid].mDE;
  int32_t dsidx = mDsMap[cru_link][dsid].mIndex;

  int32_t address = dsch + (dsidx<<6);
  pad = mPadMap[de][address];
  return true;
}


} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
