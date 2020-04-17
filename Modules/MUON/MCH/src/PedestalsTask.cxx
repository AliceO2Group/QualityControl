///
/// \file   PedestalsTask.cxx
/// \author Andrea Ferrero
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TFile.h>

#include "Headers/RAWDataHeader.h"
#include "Framework/CallbackService.h"
#include "Framework/ControlService.h"
#include "Framework/Task.h"
#include "DPLUtils/DPLRawParser.h"
#include "QualityControl/QcInfoLogger.h"
#include "MCH/PedestalsTask.h"
#include "MCHBase/Digit.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingInterface/CathodeSegmentation.h"
#include "MCHRawElecMap/Mapper.h"
#ifdef MCH_HAS_MAPPING_FACTORY
#include "MCHMappingFactory/CreateSegmentation.h"
#endif
//#define QC_MCH_SAVE_TEMP_ROOTFILE

using namespace std;
using namespace o2::framework;

static FILE* flog = NULL;

struct CRUheader {
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

enum decode_state_t {
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
PedestalsTask::PedestalsTask() : TaskInterface()
{
  flog = nullptr;
}

PedestalsTask::~PedestalsTask()
{
  printf("~PedestalsTask() called\n");
  if (flog)
    fclose(flog);
}

void PedestalsTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize PedestalsTask" << AliceO2::InfoLogger::InfoLogger::endm;
  if (true) {

    for (int c = 0; c < MCH_MAX_CRU_IN_FLP; c++) {
      for (int l = 0; l < 24; l++) {
        for (int i = 0; i < 40; i++) {
          for (int j = 0; j < 64; j++) {
            nhits[c][l][i][j] = 0;
            pedestal[c][l][i][j] = noise[c][l][i][j] = 0;
          }
        }
      }
    }

    for (int de = 0; de < 1100; de++) {
      for (int padid = 0; padid < 1500; padid++) {
        nhitsDigits[de][padid] = 0;
        pedestalDigits[de][padid] = noiseDigits[de][padid] = 0;
      }
    }

    mDecoder.initialize();

    uint32_t dsid;
    std::vector<int> DEs;
    for (int cruid = 0; cruid < 31; cruid++) {

      for (int linkid = 0; linkid < 24; linkid++) {

        int index = 24 * cruid + linkid;
        mHistogramPedestals[index] = new TH2F(TString::Format("QcMuonChambers_Pedestals_CRU%01d_LINK%02d", cruid, linkid),
                                              TString::Format("QcMuonChambers - Pedestals (CRU %01d, link %02d)", cruid, linkid), 40, 0, 40, 64, 0, 64);
        //getObjectsManager()->startPublishing(mHistogramPedestals[index]);
        //getObjectsManager()->addCheck(mHistogramPedestals[index], "checkFromMuonChambers",
        //    "o2::quality_control_modules::muonchambers::MCHCheckPedestals", "QcMuonChambers");

        mHistogramNoise[index] =
          new TH2F(TString::Format("QcMuonChambers_Noise_CRU%01d_LINK%02d", cruid, linkid),
                   TString::Format("QcMuonChambers - Noise (CRU %01d link %02d)", cruid, linkid), 40, 0, 40, 64, 0, 64);
        //getObjectsManager()->startPublishing(mHistogramNoise[index]);

        int32_t link_id = mDecoder.getMapCRU(cruid, linkid);
        if (link_id == -1)
          continue;

        for (int ds_addr = 0; ds_addr < 40; ds_addr++) {

          uint32_t de;
          int32_t ret = mDecoder.getMapFEC(link_id, ds_addr, de, dsid);
          if (ret < 0)
            continue;

          if ((std::find(DEs.begin(), DEs.end(), de)) == DEs.end()) {
            DEs.push_back(de);
            TH2F* hPedDE = new TH2F(TString::Format("QcMuonChambers_Pedestals_DE%03d", de),
                                    TString::Format("QcMuonChambers - Pedestals (DE%03d)", de), 2000, 0, 2000, 64, 0, 64);
            mHistogramPedestalsDE.insert(make_pair(de, hPedDE));
            //getObjectsManager()->startPublishing(hPedDE);
            TH2F* hNoiseDE = new TH2F(TString::Format("QcMuonChambers_Noise_DE%03d", de),
                                      TString::Format("QcMuonChambers - Noise (DE%03d)", de), 2000, 0, 2000, 64, 0, 64);
            mHistogramNoiseDE.insert(make_pair(de, hNoiseDE));
            //getObjectsManager()->startPublishing(hNoiseDE);

            for (int pi = 0; pi < 5; pi++) {
              TH1F* hNoiseDE = new TH1F(TString::Format("QcMuonChambers_Noise_Distr_DE%03d_b_%d", de, pi),
                                        TString::Format("QcMuonChambers - Noise distribution (DE%03d B, %d)", de, pi), 1000, 0, 10);
              mHistogramNoiseDistributionDE[pi][0].insert(make_pair(de, hNoiseDE));
              hNoiseDE = new TH1F(TString::Format("QcMuonChambers_Noise_Distr_DE%03d_nb_%d", de, pi),
                                  TString::Format("QcMuonChambers - Noise distribution (DE%03d NB, %d)", de, pi), 1000, 0, 10);
              mHistogramNoiseDistributionDE[pi][1].insert(make_pair(de, hNoiseDE));
            }

            float Xsize = 50 * 5;
            float Xsize2 = Xsize / 2;
            float Ysize = 50;
            float Ysize2 = Ysize / 2;
            {
              TH2F* hPedXY = new TH2F(TString::Format("QcMuonChambers_Pedestals_XYb_%03d", de),
                                      TString::Format("QcMuonChambers - Pedestals XY (DE%03d B)", de), Xsize * 2, -Xsize2, Xsize2, Ysize * 2, -Ysize2, Ysize2);
              mHistogramPedestalsXY[0].insert(make_pair(de, hPedXY));
              getObjectsManager()->startPublishing(hPedXY);
              TH2F* hNoiseXY = new TH2F(TString::Format("QcMuonChambers_Noise_XYb_%03d", de),
                                        TString::Format("QcMuonChambers - Noise XY (DE%03d B)", de), Xsize * 2, -Xsize2, Xsize2, Ysize * 2, -Ysize2, Ysize2);
              mHistogramNoiseXY[0].insert(make_pair(de, hNoiseXY));
              getObjectsManager()->startPublishing(hNoiseXY);
            }
            {
              TH2F* hPedXY = new TH2F(TString::Format("QcMuonChambers_Pedestals_XYnb_%03d", de),
                                      TString::Format("QcMuonChambers - Pedestals XY (DE%03d NB)", de), Xsize * 2, -Xsize2, Xsize2, Ysize * 2, -Ysize2, Ysize2);
              mHistogramPedestalsXY[1].insert(make_pair(de, hPedXY));
              getObjectsManager()->startPublishing(hPedXY);
              TH2F* hNoiseXY = new TH2F(TString::Format("QcMuonChambers_Noise_XYnb_%03d", de),
                                        TString::Format("QcMuonChambers - Noise XY (DE%03d NB)", de), Xsize * 2, -Xsize2, Xsize2, Ysize * 2, -Ysize2, Ysize2);
              mHistogramNoiseXY[1].insert(make_pair(de, hNoiseXY));
              getObjectsManager()->startPublishing(hNoiseXY);
            }
          }
        }
      }
    }
  }

  mPrintLevel = 0;

  flog = stdout; //fopen("/root/qc.log", "w");
}

void PedestalsTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PedestalsTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PedestalsTask::fill_noise_distributions()
{
  for (int pi = 0; pi < 5; pi++) {
    for (int i = 0; i < 2; i++) {
      auto ih = mHistogramNoiseDistributionDE[pi][i].begin();
      while (ih != mHistogramNoiseDistributionDE[pi][i].end()) {
        ih->second->Reset();
        ih++;
      }
    }
  }
  auto ih = mHistogramNoiseDE.begin();
  for (; ih != mHistogramNoiseDE.end(); ih++) {
    int de = ih->first;
    if (!ih->second)
      continue;
    if (ih->second->GetEntries() < 1)
      continue;

    for (int bi = 0; bi < ih->second->GetXaxis()->GetNbins(); bi++) {
      for (int ci = 0; ci < ih->second->GetYaxis()->GetNbins(); ci++) {
        float noise = ih->second->GetBinContent(bi + 1, ci + 1);
        if (noise < 0.001)
          continue;

        int dsid = bi;
        int chan_addr = ci;

        MapPad pad;
        mDecoder.getMapFEC().getPadByDE(de, dsid, chan_addr, pad);
        float padSizeX = pad.fSizeX;
        float padSizeY = pad.fSizeY;
        int cathode = pad.fCathode;

        /*
        o2::mch::mapping::Segmentation segment(de);
        int padid = segment.findPadByFEE(dsid, chan_addr);
        if(padid < 0) {
          //fprintf(flog,"Invalid pad: %d %d %d\n", link_id, dsid, hit.chan_addr);
          continue;
        }

        float padSizeX = segment.padSizeX(padid);
        float padSizeY = segment.padSizeY(padid);
        int cathode = segment.isBendingPad(padid) ? 0 : 1;
         */
        float szmax = padSizeX;
        if (szmax < padSizeY)
          szmax = padSizeY;
        //fprintf(flog,"%d %d %d -> %d %f %f\n", de, dsid, chan_addr, padid, padSizeX, padSizeY);

        int szid = 0;
        if (fabs(szmax - 2.5) < 0.001)
          szid = 1;
        else if (fabs(szmax - 5.0) < 0.001)
          szid = 2;
        else if (fabs(szmax - 10.0) < 0.001)
          szid = 3;

        auto hNoiseDE = mHistogramNoiseDistributionDE[szid][cathode].find(de);
        if ((hNoiseDE != mHistogramNoiseDistributionDE[szid][cathode].end()) && (hNoiseDE->second != NULL)) {
          hNoiseDE->second->Fill(noise);
        }
      }
    }
  }
}

void PedestalsTask::save_histograms()
{
  TFile f("/tmp/qc.root", "RECREATE");
  fill_noise_distributions();
  for (int i = 0; i < MCH_MAX_CRU_IN_FLP * 24; i++) {
    mHistogramNoise[i]->Write();
    mHistogramPedestals[i]->Write();
  }
  for (int i = 0; i < 2; i++) {
    auto ih = mHistogramPedestalsXY[i].begin();
    while (ih != mHistogramPedestalsXY[i].end()) {
      ih->second->Write();
      ih++;
    }
  }
  for (int i = 0; i < 2; i++) {
    auto ih = mHistogramNoiseXY[i].begin();
    while (ih != mHistogramNoiseXY[i].end()) {
      ih->second->Write();
      ih++;
    }
  }
  {
    auto ih = mHistogramPedestalsDE.begin();
    while (ih != mHistogramPedestalsDE.end()) {
      ih->second->Write();
      ih++;
    }
  }
  {
    auto ih = mHistogramNoiseDE.begin();
    while (ih != mHistogramNoiseDE.end()) {
      ih->second->Write();
      ih++;
    }
  }
  for (int pi = 0; pi < 5; pi++) {
    for (int i = 0; i < 2; i++) {
      auto ih = mHistogramNoiseDistributionDE[pi][i].begin();
      while (ih != mHistogramNoiseDistributionDE[pi][i].end()) {
        ih->second->Write();
        ih++;
      }
    }
  }

  f.ls();
  f.Close();
}

void PedestalsTask::monitorDataReadout(o2::framework::ProcessingContext& ctx)
{
  //QcInfoLogger::GetInstance() << "monitorDataReadout" << AliceO2::InfoLogger::InfoLogger::endm;
  fprintf(flog, "\n================\nmonitorDataReadout\n================\n");

#ifdef QC_MCH_SAVE_TEMP_ROOTFILE
  if (count > 0 && (count % 1) == 0 /*&& count <= 5000*/) {
    save_histograms();
  }
  printf("count: %d\n", count);
  count += 1;
#endif

  // Reset the hits container
  mDecoder.clearHits();

  // For some reason the input selection doesn't work, to be investigated...
  o2::framework::DPLRawParser parser(ctx.inputs() /*, o2::framework::select("readout:MCH/RAWDATA")*/);

  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    // retrieving RDH v4
    auto const* rdh = it.get_if<o2::header::RAWDataHeaderV4>();
    if (!rdh)
      continue;
    // retrieving the raw pointer of the page
    auto const* raw = it.raw();
    // size of payload
    size_t payloadSize = it.size();
    if (payloadSize == 0)
      continue;

    //std::cout<<"\n\npayloadSize: "<<payloadSize<<std::endl;
    //std::cout<<"raw:     "<<(void*)raw<<std::endl;
    //std::cout<<"payload: "<<(void*)payload<<std::endl;

    // Run the decoder on the CRU buffer
    mDecoder.processData((const char*)raw, (size_t)(payloadSize + sizeof(o2::header::RAWDataHeaderV4)));

    std::vector<SampaHit>& hits = mDecoder.getHits();
    if (mPrintLevel >= 1)
      fprintf(flog, "hits size: %lu\n", hits.size());
    for (uint32_t i = 0; i < hits.size(); i++) {
      SampaHit& hit = hits[i];
      if (hit.link_id >= 24 || hit.ds_addr >= 40 || hit.chan_addr >= 64) {
        fprintf(stdout, "hit[%d]: link_id=%d, ds_addr=%d, chan_addr=%d\n",
                i, hit.link_id, hit.ds_addr, hit.chan_addr);
        continue;
      }

      //int ds_group_id = hit.ds_addr / 5;
      //int ds_id_in_group = hit.ds_addr % 5;
      //int ds_chan_addr_in_group = hit.chan_addr + 64 * ds_id_in_group;

      // Update the average and RMS of the pedestal values
      for (uint32_t s = 0; s < hit.samples.size(); s++) {
        int sample = hit.samples[s];

        nhits[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr] += 1;
        uint64_t N = nhits[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr];

        double p0 = pedestal[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr];
        double p = p0 + (sample - p0) / N;
        pedestal[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr] = p;

        double M0 = noise[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr];
        double M = M0 + (sample - p0) * (sample - p);
        noise[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr] = M;

        /*if (false && hit.link_id == 2 && hit.ds_addr == 37 && hit.chan_addr == 30)
        fprintf(stdout, "M0=%f  sample=%d  M=%f  nhits=%lu  rms=%f\n",
            (float)M0, sample, (float)M, nhits[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr],
            (float)std::sqrt(noise[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr] /
                nhits[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr]));*/
      }

      // Fill the histograms for each CRU link
      double rms = std::sqrt(noise[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr] /
                             nhits[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr]);

      /*if( false && hit.cru_id==0 && hit.link_id==0 && hit.ds_addr==0 && hit.chan_addr==0)
      printf("%d %d %d %d -> %d %f %f\n", (int)hit.cru_id, (int)hit.link_id, (int)hit.ds_addr, (int)hit.chan_addr,
          (int)nhits[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr],
          pedestal[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr], rms);*/

      mHistogramPedestals[hit.cru_id * 24 + hit.link_id]->SetBinContent(hit.ds_addr + 1, hit.chan_addr + 1,
                                                                        pedestal[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr]);
      mHistogramNoise[hit.cru_id * 24 + hit.link_id]->SetBinContent(hit.ds_addr + 1, hit.chan_addr + 1, rms);

      if (hit.pad.fDE < 0)
        continue;

      // Fill the histograms for each detection element
      int de = hit.pad.fDE;
      int dsid = hit.pad.fDsID;
      float padX = hit.pad.fX;
      float padY = hit.pad.fY;
      float padSizeX = hit.pad.fSizeX;
      float padSizeY = hit.pad.fSizeY;

      /*if (false) fprintf(flog, "mapping: link_id=%d ds_addr=%d chan_addr=%d  ==>  de=%d dsid=%d x=%f y=%f sx=%f sy=%f\n",
        hit.link_id, hit.ds_addr, hit.chan_addr, de, dsid, padX, padY, padSizeX, padSizeY);*/

      auto hPedDE = mHistogramPedestalsDE.find(de);
      if ((hPedDE != mHistogramPedestalsDE.end()) && (hPedDE->second != NULL)) {
        hPedDE->second->SetBinContent(dsid + 1, hit.chan_addr + 1, pedestal[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr]);
      }
      auto hNoiseDE = mHistogramNoiseDE.find(de);
      if ((hNoiseDE != mHistogramNoiseDE.end()) && (hNoiseDE->second != NULL)) {
        hNoiseDE->second->SetBinContent(dsid + 1, hit.chan_addr + 1, rms);
      }

      auto hPedXY = mHistogramPedestalsXY[hit.pad.fCathode].find(de);
      if ((hPedXY != mHistogramPedestalsXY[hit.pad.fCathode].end()) && (hPedXY->second != NULL)) {
        int binx_min = hPedXY->second->GetXaxis()->FindBin(padX - padSizeX / 2 + 0.1);
        int binx_max = hPedXY->second->GetXaxis()->FindBin(padX + padSizeX / 2 - 0.1);
        int biny_min = hPedXY->second->GetYaxis()->FindBin(padY - padSizeY / 2 + 0.1);
        int biny_max = hPedXY->second->GetYaxis()->FindBin(padY + padSizeY / 2 - 0.1);
        for (int by = biny_min; by <= biny_max; by++) {
          for (int bx = binx_min; bx <= binx_max; bx++) {
            hPedXY->second->SetBinContent(bx, by, pedestal[hit.cru_id][hit.link_id][hit.ds_addr][hit.chan_addr]);
          }
        }
      }
      auto hNoiseXY = mHistogramNoiseXY[hit.pad.fCathode].find(de);
      if ((hNoiseXY != mHistogramNoiseXY[hit.pad.fCathode].end()) && (hNoiseXY->second != NULL)) {
        int binx_min = hNoiseXY->second->GetXaxis()->FindBin(padX - padSizeX / 2 + 0.1);
        int binx_max = hNoiseXY->second->GetXaxis()->FindBin(padX + padSizeX / 2 - 0.1);
        int biny_min = hNoiseXY->second->GetYaxis()->FindBin(padY - padSizeY / 2 + 0.1);
        int biny_max = hNoiseXY->second->GetYaxis()->FindBin(padY + padSizeY / 2 - 0.1);
        for (int by = biny_min; by <= biny_max; by++) {
          for (int bx = binx_min; bx <= binx_max; bx++) {
            hNoiseXY->second->SetBinContent(bx, by, rms);
          }
        }
      }
    }
  }
}

void PedestalsTask::monitorDataDigits(const o2::framework::DataRef& input)
{
//QcInfoLogger::GetInstance() << "monitorDataDigits" << AliceO2::InfoLogger::InfoLogger::endm;

#ifdef QC_MCH_SAVE_TEMP_ROOTFILE
  if ((count % 10) == 0 /*&& count <= 5000*/) {
    save_histograms();
    printf("count: %d\n", count);
  }
  count += 1;
#endif

  if (input.spec->binding != "digits")
    return;

  const auto* header = o2::header::get<header::DataHeader*>(input.header);
  if (mPrintLevel >= 1)
    fprintf(flog, "Header: %p\n", (void*)header);
  if (!header)
    return;
  //QcInfoLogger::GetInstance() << "payloadSize: " << header->payloadSize << AliceO2::InfoLogger::InfoLogger::endm;
  if (mPrintLevel >= 1)
    fprintf(flog, "payloadSize: %d\n", (int)header->payloadSize);
  if (mPrintLevel >= 1)
    fprintf(flog, "payload: %s\n", input.payload);

  std::vector<o2::mch::Digit> digits{ 0 };
  o2::mch::Digit* digitsBuffer = NULL;
  digitsBuffer = (o2::mch::Digit*)input.payload;
  size_t ndigits = ((size_t)header->payloadSize / sizeof(o2::mch::Digit));

  if (mPrintLevel >= 1)
    std::cout << "There are " << ndigits << " digits in the payload" << std::endl;

  o2::mch::Digit* ptr = (o2::mch::Digit*)digitsBuffer;
  for (size_t di = 0; di < ndigits; di++) {
    digits.push_back(*ptr);
    ptr += 1;
  }

  for (uint32_t i = 0; i < digits.size(); i++) {
    o2::mch::Digit& digit = digits[i];
    int ADC = digit.getADC();
    int de = digit.getDetID();
    int padid = digit.getPadID();

    //fprintf(stdout, "digit[%d]: ADC=%d, DetId=%d, PadId=%d\n",
    //        i, ADC, de, padid);
    if (ADC < 0 || de < 0 || padid < 0) {
      continue;
    }

    try {
      const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(de);

      double padX = segment.padPositionX(padid);
      double padY = segment.padPositionY(padid);
      float padSizeX = segment.padSizeX(padid);
      float padSizeY = segment.padSizeY(padid);
      int cathode = segment.isBendingPad(padid) ? 0 : 1;

      // Update the average and RMS of the pedestal values
      nhitsDigits[de][padid] += 1;
      uint64_t N = nhitsDigits[de][padid];

      double p0 = pedestalDigits[de][padid];
      double p = p0 + (ADC - p0) / N;
      pedestalDigits[de][padid] = p;

      double M0 = noiseDigits[de][padid];
      double M = M0 + (ADC - p0) * (ADC - p);
      noiseDigits[de][padid] = M;

      double rms = std::sqrt(noiseDigits[de][padid] /
                             nhitsDigits[de][padid]);

      // Fill the histograms for each detection element
      auto hPedXY = mHistogramPedestalsXY[cathode].find(de);
      if ((hPedXY != mHistogramPedestalsXY[cathode].end()) && (hPedXY->second != NULL)) {
        int binx_min = hPedXY->second->GetXaxis()->FindBin(padX - padSizeX / 2 + 0.1);
        int binx_max = hPedXY->second->GetXaxis()->FindBin(padX + padSizeX / 2 - 0.1);
        int biny_min = hPedXY->second->GetYaxis()->FindBin(padY - padSizeY / 2 + 0.1);
        int biny_max = hPedXY->second->GetYaxis()->FindBin(padY + padSizeY / 2 - 0.1);
        for (int by = biny_min; by <= biny_max; by++) {
          for (int bx = binx_min; bx <= binx_max; bx++) {
            hPedXY->second->SetBinContent(bx, by, pedestalDigits[de][padid]);
          }
        }
      }
      auto hNoiseXY = mHistogramNoiseXY[cathode].find(de);
      if ((hNoiseXY != mHistogramNoiseXY[cathode].end()) && (hNoiseXY->second != NULL)) {
        int binx_min = hNoiseXY->second->GetXaxis()->FindBin(padX - padSizeX / 2 + 0.1);
        int binx_max = hNoiseXY->second->GetXaxis()->FindBin(padX + padSizeX / 2 - 0.1);
        int biny_min = hNoiseXY->second->GetYaxis()->FindBin(padY - padSizeY / 2 + 0.1);
        int biny_max = hNoiseXY->second->GetYaxis()->FindBin(padY + padSizeY / 2 - 0.1);
        for (int by = biny_min; by <= biny_max; by++) {
          for (int bx = binx_min; bx <= binx_max; bx++) {
            hNoiseXY->second->SetBinContent(bx, by, rms);
          }
        }
      }
    } catch (const std::exception& e) {
      QcInfoLogger::GetInstance() << "[MCH] Detection Element " << de << " not found in mapping." << AliceO2::InfoLogger::InfoLogger::endm;
      return;
    }
  }
}

void PedestalsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  monitorDataReadout(ctx);
  for (auto&& input : ctx.inputs()) {
    QcInfoLogger::GetInstance() << "run PedestalsTask: input " << input.spec->binding << AliceO2::InfoLogger::InfoLogger::endm;
    if (input.spec->binding == "digits")
      monitorDataDigits(input);
  }
}

void PedestalsTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PedestalsTask::endOfActivity(Activity& /*activity*/)
{
  printf("PedestalsTask::endOfActivity() called\n");
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void PedestalsTask::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Reseting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
