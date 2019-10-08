///
/// \file   RawDataProcessor.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TFile.h>

#include "Headers/RAWDataHeader.h"
#include "QualityControl/QcInfoLogger.h"
#include "MCH/RawDataProcessor.h"

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
RawDataProcessor::RawDataProcessor() : TaskInterface(), count(1), mHistogram(nullptr) { mHistogram = nullptr; }

RawDataProcessor::~RawDataProcessor()
{
  printf("~RawDataProcessor() called\n");
  fclose(flog);
}

void RawDataProcessor::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize RawDataProcessor" << AliceO2::InfoLogger::InfoLogger::endm;
  printf("initialize RawDataProcessor\n");
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
    //mMapCRU[0].addDSMapping(0, 0, de, 111);
    //mMapCRU[0].addDSMapping(0, 2, de, 112);
    //mMapCRU[0].addDSMapping(0, 4, de, 113);
    mMapCRU[0].readDSMapping(0, "/home/flp/Mapping/cru.map");
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
        getObjectsManager()->startPublishing(mHistogramPedestalsDS[i][j]);
        /*getObjectsManager()->addCheck(mHistogramPedestalsDS[i][j], "checkFromMuonChambers",
          "o2::quality_control_modules::muonchambers::MCHCheckPedestals", "QcMuonChambers");*/

        mHistogramNoiseDS[i][j] =
            new TH1F(TString::Format("QcMuonChambers_Noise_%02d_%02d", i, j),
                TString::Format("QcMuonChambers - Noise (%02d-%02d)", i, j), 64*5, 0, 64*5);
        getObjectsManager()->startPublishing(mHistogramNoiseDS[i][j]);
        /*getObjectsManager()->addCheck(mHistogram, "checkFromMuonChambers", "o2::quality_control_modules::muonchambers::MuonChambersCheck",
    "QcMuonChambers");*/
      }
    }

    {
      TH2F* hPedDE = new TH2F(TString::Format("QcMuonChambers_Pedestals_DE%03d", de),
          TString::Format("QcMuonChambers - Pedestals (DE%03d)", de), 1000, 0, 1000, 64, 0, 64);
      mHistogramPedestalsDE.insert( make_pair(de, hPedDE) );
      getObjectsManager()->startPublishing(hPedDE);
      TH2F* hNoiseDE = new TH2F(TString::Format("QcMuonChambers_Noise_DE%03d", de),
          TString::Format("QcMuonChambers - Noise (DE%03d)", de), 1000, 0, 1000, 64, 0, 64);
      mHistogramNoiseDE.insert( make_pair(de, hNoiseDE) );
      getObjectsManager()->startPublishing(hNoiseDE);

      float Xsize = 50*5;
      float Xsize2 = Xsize/2;
      float Ysize = 50;
      float Ysize2 = Ysize/2;
      TH2F* hPedXY = new TH2F(TString::Format("QcMuonChambers_Pedestals_XY%03d", de),
          TString::Format("QcMuonChambers - Pedestals XY (DE%03d)", de), Xsize*2, -Xsize2, Xsize2, Ysize*2, -Ysize2, Ysize2);
      mHistogramPedestalsXY.insert( make_pair(de, hPedXY) );
      getObjectsManager()->startPublishing(hPedXY);
      TH2F* hNoiseXY = new TH2F(TString::Format("QcMuonChambers_Noise_XY%03d", de),
          TString::Format("QcMuonChambers - Noise XY (DE%03d)", de), Xsize*2, -Xsize2, Xsize2, Ysize*2, -Ysize2, Ysize2);
      mHistogramNoiseXY.insert( make_pair(de, hNoiseXY) );
      getObjectsManager()->startPublishing(hNoiseXY);
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

  printf("count: %d\n", count);
  if( (count % 100) == 0) {
    TFile f("/home/flp/qc.root","RECREATE");
    for(int i = 0; i < 24; i++) {
      mHistogramNoise[i]->Write();
      mHistogramPedestals[i]->Write();
    }
    f.ls();
    f.Close();
  }
  count += 1;

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
    //fprintf(stdout,"hits size: %d\n", hits.size());
    for(uint32_t i = 0; i < hits.size(); i++) {
      //continue;
      SampaHit& hit = hits[i];
      if(hit.link_id>=24 || hit.ds_addr>=40 || hit.chan_addr>=64 ) {
        fprintf(stdout,"hit[%d]: link_id=%d, ds_addr=%d, chan_addr=%d\n",
            i, hit.link_id, hit.ds_addr, hit.chan_addr);
        continue;
      }

      int ds_group_id = hit.ds_addr / 5;
      int ds_id_in_group = hit.ds_addr % 5;
      int ds_chan_addr_in_group = hit.chan_addr + 64*ds_id_in_group;

      /*
      if(hit.size != 10 || hit.time != 0 ) {
        fprintf(stdout,"hit[%d]: size=%d, time=%d\n",
            i, hit.size, hit.time);
        continue;
      }

      bool bad_hit = false;
      for(uint32_t s = 0; s < hit.samples.size(); s++) {
        int sample = hit.samples[s];
        if( sample > 100 || sample < 10 ) {
          fprintf(stdout,"hit[%d]: bad sample=%d\n", i, sample);
          bad_hit = true;
          break;
        }
      }
      if( bad_hit ) continue;
      */

      for(uint32_t s = 0; s < hit.samples.size(); s++) {
        //continue;
        int sample = hit.samples[s];

        nhits[hit.link_id][hit.ds_addr][hit.chan_addr] += 1;
        uint64_t N = nhits[hit.link_id][hit.ds_addr][hit.chan_addr];

        double p0 = pedestal[hit.link_id][hit.ds_addr][hit.chan_addr];
        double p = p0 + (sample - p0) / N;
        pedestal[hit.link_id][hit.ds_addr][hit.chan_addr] = p;

        double M0 = noise[hit.link_id][hit.ds_addr][hit.chan_addr];
        double M = M0 + (sample - p0) * (sample - p);
        noise[hit.link_id][hit.ds_addr][hit.chan_addr] = M;
        if( false && hit.link_id==2 && hit.ds_addr == 37 && hit.chan_addr == 30 )
          fprintf(stdout, "M0=%f  sample=%d  M=%f  nhits=%lu  rms=%f\n",
              (float)M0, sample, (float)M, nhits[hit.link_id][hit.ds_addr][hit.chan_addr],
              (float)std::sqrt( noise[hit.link_id][hit.ds_addr][hit.chan_addr] /
                        nhits[hit.link_id][hit.ds_addr][hit.chan_addr] ));
      }
      //continue;
      mHistogramPedestals[hit.link_id]->SetBinContent(hit.ds_addr+1, hit.chan_addr+1,
          pedestal[hit.link_id][hit.ds_addr][hit.chan_addr]);
      double rms = std::sqrt( noise[hit.link_id][hit.ds_addr][hit.chan_addr] /
          nhits[hit.link_id][hit.ds_addr][hit.chan_addr] );
      //fprintf(flog,"rms=%f\n",(float)rms);
      mHistogramNoise[hit.link_id]->SetBinContent(hit.ds_addr+1, hit.chan_addr+1, rms);

      //fprintf(flog,"ds_group_id=%d  ds_chan_addr_in_group=%d\n",ds_group_id, ds_chan_addr_in_group);
      mHistogramPedestalsDS[hit.link_id][ds_group_id]->SetBinContent(ds_chan_addr_in_group+1,
          pedestal[hit.link_id][hit.ds_addr][hit.chan_addr]);
      mHistogramNoiseDS[hit.link_id][ds_group_id]->SetBinContent(ds_chan_addr_in_group+1, rms);

/*
      uint32_t de, dsid;
      if( !mMapCRU[0].getDSMapping(hit.link_id, hit.ds_addr, de, dsid) ) continue;
      o2::mch::mapping::Segmentation segment(de);
      int padid = segment.findPadByFEE(dsid, hit.chan_addr);
      if(padid < 0) {
        fprintf(flog,"Invalid pad: %d %d\n", dsid, hit.chan_addr);
        continue;
      }

      float padX = segment.padPositionX(padid);
      float padY = segment.padPositionY(padid);
      float padSizeX = segment.padSizeX(padid);
      float padSizeY = segment.padSizeY(padid);
*/

      MapPad pad;
      if( !mMapCRU[0].getPad(hit.link_id, hit.ds_addr, hit.chan_addr, pad) ) continue;
      //if( hit.ds_addr != 0 || hit.chan_addr != 0 ) continue;

      int de = pad.fDE;
      int dsid = pad.fDsID;
      float padX = pad.fX;
      float padY = pad.fY;
      float padSizeX = pad.fSizeX;
      float padSizeY = pad.fSizeY;

      //fprintf(flog, "mapping: link_id=%d ds_addr=%d chan_addr=%d  ==>  de=%d x=%f y=%f sx=%f sy=%f\n",
      //	      hit.link_id, hit.ds_addr, hit.chan_addr, de, padX, padY, padSizeX, padSizeY);

      auto hPedDE = mHistogramPedestalsDE.find(de);
      if( (hPedDE != mHistogramPedestalsDE.end()) && (hPedDE->second != NULL) ) {
        hPedDE->second->SetBinContent(dsid+1, hit.chan_addr+1, pedestal[hit.link_id][hit.ds_addr][hit.chan_addr]);
      }
      auto hNoiseDE = mHistogramNoiseDE.find(de);
      if( (hNoiseDE != mHistogramNoiseDE.end()) && (hNoiseDE->second != NULL) ) {
        hNoiseDE->second->SetBinContent(dsid+1, hit.chan_addr+1, rms);
      }

      auto hPedXY = mHistogramPedestalsXY.find(de);
      if( (hPedXY != mHistogramPedestalsXY.end()) && (hPedXY->second != NULL) ) {
        //fprintf(stdout,"Filling histograms for XY %d\n", de);
        int binx_min = hPedXY->second->GetXaxis()->FindBin(padX-padSizeX/2+0.1);
        int binx_max = hPedXY->second->GetXaxis()->FindBin(padX+padSizeX/2-0.1);
        int biny_min = hPedXY->second->GetYaxis()->FindBin(padY-padSizeY/2+0.1);
        int biny_max = hPedXY->second->GetYaxis()->FindBin(padY+padSizeY/2-0.1);
        for(int by = biny_min; by <= biny_max; by++) {
          for(int bx = binx_min; bx <= binx_max; bx++) {
            hPedXY->second->SetBinContent(bx, by, pedestal[hit.link_id][hit.ds_addr][hit.chan_addr]);
          }
        }
      }
      auto hNoiseXY = mHistogramNoiseXY.find(de);
      if( (hNoiseXY != mHistogramNoiseXY.end()) && (hNoiseXY->second != NULL) ) {
        //fprintf(stdout,"Filling histograms for XY %d\n", de);
        int binx_min = hNoiseXY->second->GetXaxis()->FindBin(padX-padSizeX/2+0.1);
        int binx_max = hNoiseXY->second->GetXaxis()->FindBin(padX+padSizeX/2-0.1);
        int biny_min = hNoiseXY->second->GetYaxis()->FindBin(padY-padSizeY/2+0.1);
        int biny_max = hNoiseXY->second->GetYaxis()->FindBin(padY+padSizeY/2-0.1);
        for(int by = biny_min; by <= biny_max; by++) {
          for(int bx = binx_min; bx <= binx_max; bx++) {
            hNoiseXY->second->SetBinContent(bx, by, rms);
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
  printf("RawDataProcessor::endOfActivity() called\n");
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
