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

    //  int de = 819;

    mHistogram = new TH1F("QcMuonChambers_PayloadSize", "QcMuonChambers Payload Size", 20, 0, 1000000000);
    getObjectsManager()->startPublishing(mHistogram);
    /*getObjectsManager()->addCheck(mHistogram, "checkFromMuonChambers", "o2::quality_control_modules::muonchambers::MuonChambersCheck",
                "QcMuonChambers");*/

    uint32_t dsid;
    std::vector<int> DEs;
    for(int cruid=0; cruid<3; cruid++){

      QcInfoLogger::GetInstance() << "JE SUIS ENTRÉ DANS LA BOUCLE CRUID " << cruid << AliceO2::InfoLogger::InfoLogger::endm;

      for(int linkid=0; linkid<24; linkid++){

        QcInfoLogger::GetInstance() << "JE SUIS ENTRÉ DANS LA BOUCLE LINKID " << linkid << AliceO2::InfoLogger::InfoLogger::endm;

        int index = 24*cruid + linkid;
        mHistogramPedestals[index] = new TH2F(TString::Format("QcMuonChambers_Pedestals_CRU%01d_LINK%02d", cruid, linkid),
            TString::Format("QcMuonChambers - Pedestals (CRU link %02d)", index), 40, 0, 40, 64, 0, 64);
        //mHistogramPedestals->SetDrawOption("col");
        getObjectsManager()->startPublishing(mHistogramPedestals[index]);
        getObjectsManager()->addCheck(mHistogramPedestals[index], "checkFromMuonChambers",
            "o2::quality_control_modules::muonchambers::MCHCheckPedestals", "QcMuonChambers");

        mHistogramNoise[index] =
            new TH2F(TString::Format("QcMuonChambers_Noise_CRU%01d_LINK%02d", cruid, linkid),
                TString::Format("QcMuonChambers - Noise (CRU link %02d)", index), 40, 0, 40, 64, 0, 64);
        getObjectsManager()->startPublishing(mHistogramNoise[index]);


        /*for(int j = 0; j < 8; j++) {

          QcInfoLogger::GetInstance() << "JE SUIS ENTRÉ DANS LA BOUCLE DS " << j << AliceO2::InfoLogger::InfoLogger::endm;

          mHistogramPedestalsDS[index][j] =
              new TH1F(TString::Format("QcMuonChambers_Pedestals_CRU%01d_LINK%02d_%02d", cruid, linkid, j),
                  TString::Format("QcMuonChambers - Pedestals (%02d-%02d)", index, j), 64*5, 0, 64*5);
          getObjectsManager()->startPublishing(mHistogramPedestalsDS[index][j]);
          //getObjectsManager()->addCheck(mHistogramPedestalsDS[i][j], "checkFromMuonChambers",
          //            "o2::quality_control_modules::muonchambers::MCHCheckPedestals", "QcMuonChambers");

          mHistogramNoiseDS[index][j] =
              new TH1F(TString::Format("QcMuonChambers_Noise_CRU%01d_LINK%02d_%02d", cruid, linkid, j),
                  TString::Format("QcMuonChambers - Noise (%02d-%02d)", index, j), 64*5, 0, 64*5);
          getObjectsManager()->startPublishing(mHistogramNoiseDS[index][j]);
          //getObjectsManager()->addCheck(mHistogram, "checkFromMuonChambers", "o2::quality_control_modules::muonchambers::MuonChambersCheck",
          //      "QcMuonChambers");
        }*/

        int32_t link_id = mDecoder.getMapCRU(cruid,linkid);
        if(link_id == -1) continue;

        for(int ds_addr=0; ds_addr<40; ds_addr++){

          QcInfoLogger::GetInstance() << "JE SUIS ENTRÉ DANS LA BOUCLE DS_ADDR " << ds_addr << AliceO2::InfoLogger::InfoLogger::endm;

          uint32_t de = mDecoder.getMapFEC(link_id, ds_addr, de, dsid);

          QcInfoLogger::GetInstance() << "C'EST LA LIGNE APRÈS LE GETMAPFEC, DE " << de << AliceO2::InfoLogger::InfoLogger::endm;

          if( (std::find(DEs.begin(), DEs.end(), de)) == DEs.end() ){
            DEs.push_back(de);
            //mMapFEC.readPadMapping2(de, false);
            QcInfoLogger::GetInstance() << "C'EST LA LIGNE APRÈS LE READPADMAPPING2, DE " << de << AliceO2::InfoLogger::InfoLogger::endm;

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
              {
                TH2F* hPedXY = new TH2F(TString::Format("QcMuonChambers_Pedestals_XYb_%03d", de),
                    TString::Format("QcMuonChambers - Pedestals XY (DE%03d B)", de), Xsize*2, -Xsize2, Xsize2, Ysize*2, -Ysize2, Ysize2);
                mHistogramPedestalsXY[0].insert( make_pair(de, hPedXY) );
                getObjectsManager()->startPublishing(hPedXY);
                TH2F* hNoiseXY = new TH2F(TString::Format("QcMuonChambers_Noise_XYb_%03d", de),
                    TString::Format("QcMuonChambers - Noise XY (DE%03d B)", de), Xsize*2, -Xsize2, Xsize2, Ysize*2, -Ysize2, Ysize2);
                mHistogramNoiseXY[0].insert( make_pair(de, hNoiseXY) );
                getObjectsManager()->startPublishing(hNoiseXY);
              }
              {
                TH2F* hPedXY = new TH2F(TString::Format("QcMuonChambers_Pedestals_XYnb_%03d", de),
                    TString::Format("QcMuonChambers - Pedestals XY (DE%03d NB)", de), Xsize*2, -Xsize2, Xsize2, Ysize*2, -Ysize2, Ysize2);
                mHistogramPedestalsXY[1].insert( make_pair(de, hPedXY) );
                getObjectsManager()->startPublishing(hPedXY);
                TH2F* hNoiseXY = new TH2F(TString::Format("QcMuonChambers_Noise_XYnb_%03d", de),
                    TString::Format("QcMuonChambers - Noise XY (DE%03d NB)", de), Xsize*2, -Xsize2, Xsize2, Ysize*2, -Ysize2, Ysize2);
                mHistogramNoiseXY[1].insert( make_pair(de, hNoiseXY) );
                getObjectsManager()->startPublishing(hNoiseXY);
              }
            }
          }
        }
      }
    }
  }

  gPrintLevel = 1;

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
  fprintf(flog, "\n\n====================\nRawDataProcessor::monitorData\n====================\n");

  printf("count: %d\n", count);
  if( (count % 100) == 0) {
    TFile f("/tmp/qc.root","RECREATE");
    for(int i = 0; i < 3*24; i++) {
      mHistogramNoise[i]->Write();
      mHistogramPedestals[i]->Write();
    }
    for(int i = 0; i < 2; i++) {
      auto ih = mHistogramPedestalsXY[i].begin();
      while( ih != mHistogramPedestalsXY[i].end() ) {
        ih->second->Write();
        ih++;
      }
    }
    for(int i = 0; i < 2; i++) {
      auto ih = mHistogramNoiseXY[i].begin();
      while( ih != mHistogramNoiseXY[i].end() ) {
        ih->second->Write();
        ih++;
      }
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
    QcInfoLogger::GetInstance() << "run RawDataProcessor: input " << input.spec->binding << AliceO2::InfoLogger::InfoLogger::endm;

    const auto* header = o2::header::get<header::DataHeader*>(input.header);
    //QcInfoLogger::GetInstance() << "header: " << header << AliceO2::InfoLogger::InfoLogger::endm;
    if(gPrintLevel>=1) fprintf(flog, "Header: %p\n", header);
    if( !header ) continue;
    //QcInfoLogger::GetInstance() << "payloadSize: " << header->payloadSize << AliceO2::InfoLogger::InfoLogger::endm;
    if(gPrintLevel>=1) fprintf(flog, "payloadSize: %d\n", (int)header->payloadSize);
    if(gPrintLevel>=1) fprintf(flog, "payload: %p\n", input.payload);
    mHistogram->Fill(header->payloadSize);
    //continue;

    mDecoder.processData( input.payload, header->payloadSize );

    std::vector<SampaHit>& hits = mDecoder.getHits();
    fprintf(flog,"hits size: %d\n", hits.size());
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
      fprintf(flog,"rms=%f\n",(float)rms);
      mHistogramNoise[hit.link_id]->SetBinContent(hit.ds_addr+1, hit.chan_addr+1, rms);

      fprintf(flog,"ds_group_id=%d  ds_chan_addr_in_group=%d\n",ds_group_id, ds_chan_addr_in_group);
      //mHistogramPedestalsDS[hit.link_id][ds_group_id]->SetBinContent(ds_chan_addr_in_group+1,
      //    pedestal[hit.link_id][hit.ds_addr][hit.chan_addr]);
      //mHistogramNoiseDS[hit.link_id][ds_group_id]->SetBinContent(ds_chan_addr_in_group+1, rms);

      //if( hit.ds_addr != 0 || hit.chan_addr != 0 ) continue;

      int de = hit.pad.fDE;
      int dsid = hit.pad.fDsID;
      float padX = hit.pad.fX;
      float padY = hit.pad.fY;
      float padSizeX = hit.pad.fSizeX;
      float padSizeY = hit.pad.fSizeY;

      fprintf(flog, "mapping: link_id=%d ds_addr=%d chan_addr=%d  ==>  de=%d x=%f y=%f sx=%f sy=%f\n",
          hit.link_id, hit.ds_addr, hit.chan_addr, de, padX, padY, padSizeX, padSizeY);

      if( hit.pad.fDE < 0 ) continue;

      auto hPedDE = mHistogramPedestalsDE.find(de);
      if( (hPedDE != mHistogramPedestalsDE.end()) && (hPedDE->second != NULL) ) {
        hPedDE->second->SetBinContent(dsid+1, hit.chan_addr+1, pedestal[hit.link_id][hit.ds_addr][hit.chan_addr]);
      }
      auto hNoiseDE = mHistogramNoiseDE.find(de);
      if( (hNoiseDE != mHistogramNoiseDE.end()) && (hNoiseDE->second != NULL) ) {
        hNoiseDE->second->SetBinContent(dsid+1, hit.chan_addr+1, rms);
      }

      auto hPedXY = mHistogramPedestalsXY[hit.pad.fCathode].find(de);
      if( (hPedXY != mHistogramPedestalsXY[hit.pad.fCathode].end()) && (hPedXY->second != NULL) ) {
        fprintf(flog,"Filling histograms for XY %d,%d\n", de, hit.pad.fCathode);
        int binx_min = hPedXY->second->GetXaxis()->FindBin(padX-padSizeX/2+0.1);
        int binx_max = hPedXY->second->GetXaxis()->FindBin(padX+padSizeX/2-0.1);
        int biny_min = hPedXY->second->GetYaxis()->FindBin(padY-padSizeY/2+0.1);
        int biny_max = hPedXY->second->GetYaxis()->FindBin(padY+padSizeY/2-0.1);
        fprintf(flog, "  binx_min=%f binx_max=%f\n", binx_min, binx_max);
        for(int by = biny_min; by <= biny_max; by++) {
          for(int bx = binx_min; bx <= binx_max; bx++) {
            hPedXY->second->SetBinContent(bx, by, pedestal[hit.link_id][hit.ds_addr][hit.chan_addr]);
          }
        }
      }
      auto hNoiseXY = mHistogramNoiseXY[hit.pad.fCathode].find(de);
      if( (hNoiseXY != mHistogramNoiseXY[hit.pad.fCathode].end()) && (hNoiseXY->second != NULL) ) {
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
