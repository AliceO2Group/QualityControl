///
/// \file   PhysicsDataProcessor.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "Headers/RAWDataHeader.h"
#include "QualityControl/QcInfoLogger.h"
#include "MuonChambers/PhysicsDataProcessor.h"

using namespace std;



static int gPrintLevel;

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
  DECODE_STATE_SAMPLE_FOUND
};


namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{
PhysicsDataProcessor::PhysicsDataProcessor() : TaskInterface() { }

PhysicsDataProcessor::~PhysicsDataProcessor() {fclose(flog);}

void PhysicsDataProcessor::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize PhysicsDataProcessor" << AliceO2::InfoLogger::InfoLogger::endm;

  mDecoder.initialize();

  int de = 819;
  mMapCRU[0].addDSMapping(0, 0, de, 4);
  mMapCRU[0].addDSMapping(0, 2, de, 3);
  mMapCRU[0].readPadMapping(de, "/home/flp/Mapping/slat330000N.Bending.map",
      "/home/flp/Mapping/slat330000N.NonBending.map", false);


  {
    TH1F* h = new TH1F(TString::Format("QcMuonChambers_ADCamplitude_DE%03d", de),
        TString::Format("QcMuonChambers - ADC amplitude (DE%03d)", de), 1000, 0, 10000);
    mHistogramADCamplitudeDE.insert( make_pair(de, h) );
    getObjectsManager()->startPublishing(h);

    float Xsize = 40*5;
    float Ysize = 50;
    float Ysize2 = Ysize/2;
    TH2F* h2 = new TH2F(TString::Format("QcMuonChambers_Nhits_DE%03d", de),
        TString::Format("QcMuonChambers - Number of hits (DE%03d)", de), Xsize*2, 0, Xsize, Ysize*2, -Ysize2, Ysize2);
    mHistogramNhitsDE.insert( make_pair(de, h2) );
    getObjectsManager()->startPublishing(h2);
  }

  gPrintLevel = 0;

  flog = stdout; //fopen("/root/qc.log", "w");
}

void PhysicsDataProcessor::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PhysicsDataProcessor::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PhysicsDataProcessor::monitorData(o2::framework::ProcessingContext& ctx)
{
  // todo: update API examples or refer to DPL README.md

  QcInfoLogger::GetInstance() << "monitorData" << AliceO2::InfoLogger::InfoLogger::endm;

  // exemplary ways of accessing inputs (incoming data), that were specified in the .ini file - e.g.:
  //  [readoutInput]
  //  inputName=readout
  //  dataOrigin=ITS
  //  dataDescription=RAWDATA

  // 1. in a loop
  for (auto&& input : ctx.inputs()) {
    const auto* header = o2::header::get<header::DataHeader*>(input.header);
    //QcInfoLogger::GetInstance() << "header: " << header << AliceO2::InfoLogger::InfoLogger::endm;
    //if(gPrintLevel>=1) fprintf(flog, "Header: %p\n", header);
    if( !header ) continue;
    //QcInfoLogger::GetInstance() << "payloadSize: " << header->payloadSize << AliceO2::InfoLogger::InfoLogger::endm;
    if(gPrintLevel>=1) fprintf(flog, "payloadSize: %d\n", (int)header->payloadSize);
    if(gPrintLevel>=1) fprintf(flog, "payload: %p\n", input.payload);
    //continue;

    mDecoder.processData( input.payload, header->payloadSize );

    std::vector<SampaHit>& hits = mDecoder.getHits();
    for(uint32_t i = 0; i < hits.size(); i++) {
      //continue;
      SampaHit& hit = hits[i];
      if(hit.link_id>=24 || hit.ds_addr>=40 || hit.chan_addr>=64 ) {
        fprintf(stdout,"hit[%d]: link_id=%d, ds_addr=%d, chan_addr=%d\n",
            i, hit.link_id, hit.ds_addr, hit.chan_addr);
        continue;
      }

      MapPad pad;
      mMapCRU[0].getPad(hit.link_id, hit.ds_addr, hit.chan_addr, pad);

      //fprintf(flog, "mapping: link_id=%d ds_addr=%d chan_addr=%d  ==>  de=%d x=%f y=%f\n",
      //	      hit.link_id, hit.ds_addr, hit.chan_addr, pad.fDE, pad.fX, pad.fY);

      auto h = mHistogramADCamplitudeDE.find(pad.fDE);
      if( (h != mHistogramADCamplitudeDE.end()) && (h->second != NULL) ) {
        h->second->Fill(hit.csum);
      }

      auto h2 = mHistogramNhitsDE.find(pad.fDE);
      if( (h2 != mHistogramNhitsDE.end()) && (h2->second != NULL) ) {
        int binx_min = h2->second->GetXaxis()->FindBin(pad.fX-pad.fSizeX/2+0.1);
        int binx_max = h2->second->GetXaxis()->FindBin(pad.fX+pad.fSizeX/2-0.1);
        int biny_min = h2->second->GetYaxis()->FindBin(pad.fY-pad.fSizeY/2+0.1);
        int biny_max = h2->second->GetYaxis()->FindBin(pad.fY+pad.fSizeY/2-0.1);
        for(int by = biny_min; by <= biny_max; by++) {
          float y = h2->second->GetYaxis()->GetBinCenter(by);
          for(int bx = binx_min; bx <= binx_max; bx++) {
            float x = h2->second->GetXaxis()->GetBinCenter(bx);
            h2->second->Fill(x, y);
          }
        }
      }
    }

    mDecoder.clearHits();
  }
}

void PhysicsDataProcessor::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PhysicsDataProcessor::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PhysicsDataProcessor::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Reseting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
