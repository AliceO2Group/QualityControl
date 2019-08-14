///
/// \file   RawDataProcessor.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "Headers/RAWDataHeader.h"
#include "QualityControl/QcInfoLogger.h"
#include "MuonChambers/RawDataProcessor.h"

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
RawDataProcessor::RawDataProcessor() : TaskInterface(), mHistogram(nullptr) { mHistogram = nullptr; }

RawDataProcessor::~RawDataProcessor() {fclose(flog);}

void RawDataProcessor::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize RawDataProcessor" << AliceO2::InfoLogger::InfoLogger::endm;

  if( true ) {

    for(int c = 0; c < 24; c++) {
      for(int i = 0; i < 40; i++) {
        for(int j = 0; j < 64; j++) {
          nhits[c][i][j] = 0;
          pedestal[c][i][j] = noise[c][i][j] = 0;
        }
      }
    }

    mDecoder.initialize();

    int de = 819;
    mMapCRU[0].addDSMapping(0, 0, de, 4);
    mMapCRU[0].readPadMapping(de, "/home/flp/Mapping/slat330000N.Bending.map",
        "/home/flp/Mapping/slat330000N.NonBending.map", true);


    mHistogram = new TH1F("QcMuonChambers_PayloadSize", "QcMuonChambers Payload Size", 20, 0, 1000000000);
    getObjectsManager()->startPublishing(mHistogram);
    /*getObjectsManager()->addCheck(mHistogram, "checkFromMuonChambers", "o2::quality_control_modules::muonchambers::MuonChambersCheck",
    "QcMuonChambers");*/

    for(int i = 0; i < 24; i++) {

      mHistogramPedestals[i] = new TH2F(TString::Format("QcMuonChambers_Pedestals_%02d", i),
          TString::Format("QcMuonChambers - Pedestals (CRU link %02d)", i), 40, 0, 40, 64, 0, 64);
      //mHistogramPedestals->SetDrawOption("col");
      getObjectsManager()->startPublishing(mHistogramPedestals[i]);
      getObjectsManager()->addCheck(mHistogramPedestals[i], "checkFromMuonChambers",
          "o2::quality_control_modules::muonchambers::MCHCheckPedestals", "QcMuonChambers");

      mHistogramNoise[i] =
          new TH2F(TString::Format("QcMuonChambers_Noise_%02d", i),
              TString::Format("QcMuonChambers - Noise (CRU link %02d)", i), 40, 0, 40, 64, 0, 64);
      getObjectsManager()->startPublishing(mHistogramNoise[i]);

      for(int j = 0; j < 8; j++) {
        mHistogramPedestalsDS[i][j] =
            new TH1F(TString::Format("QcMuonChambers_Pedestals_%02d_%02d", i, j),
                TString::Format("QcMuonChambers - Pedestals (%02d-%02d)", i, j), 64*5, 0, 64*5);
        //getObjectsManager()->startPublishing(mHistogramPedestalsDS[i][j]);
        /*getObjectsManager()->addCheck(mHistogramPedestalsDS[i][j], "checkFromMuonChambers",
          "o2::quality_control_modules::muonchambers::MCHCheckPedestals", "QcMuonChambers");*/

        mHistogramNoiseDS[i][j] =
            new TH1F(TString::Format("QcMuonChambers_Noise_%02d_%02d", i, j),
                TString::Format("QcMuonChambers - Noise (%02d-%02d)", i, j), 64*5, 0, 64*5);
        //getObjectsManager()->startPublishing(mHistogramNoiseDS[i][j]);
        /*getObjectsManager()->addCheck(mHistogram, "checkFromMuonChambers", "o2::quality_control_modules::muonchambers::MuonChambersCheck",
    "QcMuonChambers");*/
      }
    }

    {
      float Xsize = 40*5;
      float Ysize = 50;
      float Ysize2 = Ysize/2;
      TH2F* hPedDE = new TH2F(TString::Format("QcMuonChambers_Pedestals_DE%03d", de),
          TString::Format("QcMuonChambers - Pedestals (DE%03d)", de), Xsize*2, 0, Xsize, Ysize*2, -Ysize2, Ysize2);
      mHistogramPedestalsDE.insert( make_pair(de, hPedDE) );
      getObjectsManager()->startPublishing(hPedDE);
      TH2F* hNoiseDE = new TH2F(TString::Format("QcMuonChambers_Noise_DE%03d", de),
          TString::Format("QcMuonChambers - Noise (DE%03d)", de), Xsize*2, 0, Xsize, Ysize*2, -Ysize2, Ysize2);
      mHistogramNoiseDE.insert( make_pair(de, hNoiseDE) );
      getObjectsManager()->startPublishing(hNoiseDE);
    }
  }

  gPrintLevel = 0;

  flog = stdout; //fopen("/root/qc.log", "w");
}

void RawDataProcessor::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  mHistogram->Reset();
}

void RawDataProcessor::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void RawDataProcessor::monitorData(o2::framework::ProcessingContext& ctx)
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
    mHistogram->Fill(header->payloadSize);
    //continue;

    mDecoder.processData( input.payload, header->payloadSize );

    std::vector<SampaHit>& hits = mDecoder.getHits();
    for(uint32_t i = 0; i < hits.size(); i++) {
      //continue;
      SampaHit& hit = hits[i];
      if(hit.link_id>=24 || hit.ds_addr>=40 || hit.chan_addr>=64 )
        fprintf(stdout,"hit[%d]: link_id=%d, ds_addr=%d, chan_addr=%d\n",
            i, hit.link_id, hit.ds_addr, hit.chan_addr);
      for(uint32_t s = 0; s < hit.samples.size(); s++) {
        //continue;
        nhits[hit.link_id][hit.ds_addr][hit.chan_addr] += 1;
        uint64_t N = nhits[hit.link_id][hit.ds_addr][hit.chan_addr];
        int sample = hit.samples[s];

        double p0 = pedestal[hit.link_id][hit.ds_addr][hit.chan_addr];
        double p = p0 + (sample - p0) / N;
        pedestal[hit.link_id][hit.ds_addr][hit.chan_addr] = p;

        double M0 = noise[hit.link_id][hit.ds_addr][hit.chan_addr];
        double M = M0 + (sample - p0) * (sample - p);
        noise[hit.link_id][hit.ds_addr][hit.chan_addr] = M;
      }
      //continue;
      mHistogramPedestals[hit.link_id]->SetBinContent(hit.ds_addr+1, hit.chan_addr+1,
          pedestal[hit.link_id][hit.ds_addr][hit.chan_addr]);
      double rms = std::sqrt( noise[hit.link_id][hit.ds_addr][hit.chan_addr] /
          nhits[hit.link_id][hit.ds_addr][hit.chan_addr] );
      //fprintf(flog,"rms=%f\n",(float)rms);
      mHistogramNoise[hit.link_id]->SetBinContent(hit.ds_addr+1, hit.chan_addr+1, rms);


      MapPad pad;
      mMapCRU[0].getPad(hit.link_id, hit.ds_addr, hit.chan_addr, pad);

      //fprintf(flog, "mapping: link_id=%d ds_addr=%d chan_addr=%d  ==>  de=%d x=%f y=%f\n",
      //	      hit.link_id, hit.ds_addr, hit.chan_addr, pad.fDE, pad.fX, pad.fY);

      auto hPedDE = mHistogramPedestalsDE.find(pad.fDE);
      if( (hPedDE != mHistogramPedestalsDE.end()) && (hPedDE->second != NULL) ) {
        int binx_min = hPedDE->second->GetXaxis()->FindBin(pad.fX-pad.fSizeX/2+0.1);
        int binx_max = hPedDE->second->GetXaxis()->FindBin(pad.fX+pad.fSizeX/2-0.1);
        int biny_min = hPedDE->second->GetYaxis()->FindBin(pad.fY-pad.fSizeY/2+0.1);
        int biny_max = hPedDE->second->GetYaxis()->FindBin(pad.fY+pad.fSizeY/2-0.1);
        for(int by = biny_min; by <= biny_max; by++) {
          for(int bx = binx_min; bx <= binx_max; bx++) {
            hPedDE->second->SetBinContent(bx, by, pedestal[hit.link_id][hit.ds_addr][hit.chan_addr]);
          }
        }
      }
      auto hNoiseDE = mHistogramNoiseDE.find(pad.fDE);
      if( (hNoiseDE != mHistogramNoiseDE.end()) && (hNoiseDE->second != NULL) ) {
        int binx_min = hNoiseDE->second->GetXaxis()->FindBin(pad.fX-pad.fSizeX/2+0.1);
        int binx_max = hNoiseDE->second->GetXaxis()->FindBin(pad.fX+pad.fSizeX/2-0.1);
        int biny_min = hNoiseDE->second->GetYaxis()->FindBin(pad.fY-pad.fSizeY/2+0.1);
        int biny_max = hNoiseDE->second->GetYaxis()->FindBin(pad.fY+pad.fSizeY/2-0.1);
        for(int by = biny_min; by <= biny_max; by++) {
          for(int bx = binx_min; bx <= binx_max; bx++) {
            hNoiseDE->second->SetBinContent(bx, by, rms);
          }
        }
      }
    }

    mDecoder.clearHits();
  }
}

void RawDataProcessor::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void RawDataProcessor::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void RawDataProcessor::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Reseting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  mHistogram->Reset();
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
