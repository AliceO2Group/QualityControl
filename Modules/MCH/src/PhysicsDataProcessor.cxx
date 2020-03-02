///
/// \file   PhysicsDataProcessor.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TFile.h>
#include <algorithm>

#include "Headers/RAWDataHeader.h"
#include "QualityControl/QcInfoLogger.h"
#include "MCH/PhysicsDataProcessor.h"
#include "MCHPreClustering/PreClusterFinder.h"
#include "MCHPreClustering/PreClusterBlock.h"
#include "MCHPreClustering/PreClusterFinderMapping.h"
#include "MCHClustering/ClusteringForTest.h"
#include "MCHBase/Digit.h"
#include "MCHMappingInterface/Segmentation.h"

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
PhysicsDataProcessor::PhysicsDataProcessor() : TaskInterface(), count(1) { }

PhysicsDataProcessor::~PhysicsDataProcessor() {fclose(flog);}

void PhysicsDataProcessor::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize PhysicsDataProcessor" << AliceO2::InfoLogger::InfoLogger::endm;
  fprintf(stdout,"initialize PhysicsDataProcessor\n");

  mDecoder.initialize();

 uint32_t dsid;
 std::vector<int> DEs;
  for(int cruid=0; cruid<3; cruid++){
      QcInfoLogger::GetInstance() << "JE SUIS ENTRÉ DANS LA BOUCLE CRUID " << cruid << AliceO2::InfoLogger::InfoLogger::endm;
      for(int linkid=0; linkid<24; linkid++){
          QcInfoLogger::GetInstance() << "JE SUIS ENTRÉ DANS LA BOUCLE LINKID " << linkid << AliceO2::InfoLogger::InfoLogger::endm;
          
          {
              int index = 24*cruid+linkid;
            mHistogramNhits[index] = new TH2F(TString::Format("QcMuonChambers_NHits_CRU%01d_LINK%02d", cruid, linkid),
                TString::Format("QcMuonChambers - Number of hits (CRU link %02d)", index), 40, 0, 40, 64, 0, 64);
            //mHistogramPedestals->SetDrawOption("col");
            getObjectsManager()->startPublishing(mHistogramNhits[index]);

            mHistogramADCamplitude[index] = new TH1F(TString::Format("QcMuonChambers_ADC_Amplitude_CRU%01d_LINK%02d", cruid, linkid),
                TString::Format("QcMuonChambers - ADC amplitude (CRU link %02d)", index), 5000, 0, 5000);
            //mHistogramPedestals->SetDrawOption("col");
            getObjectsManager()->startPublishing(mHistogramADCamplitude[index]);
              
          }
          
        int32_t link_id = mDecoder.getMapCRU(cruid,linkid);
        if(link_id == -1) continue;
          for(int ds_addr=0; ds_addr<40; ds_addr++){
              QcInfoLogger::GetInstance() << "JE SUIS ENTRÉ DANS LA BOUCLE DS_ADDR " << ds_addr << AliceO2::InfoLogger::InfoLogger::endm;
            uint32_t de = mDecoder.getMapFEC(link_id, ds_addr, de, dsid);
              QcInfoLogger::GetInstance() << "C'EST LA LIGNE APRÈS LE GETMAPFEC, DE " << de << AliceO2::InfoLogger::InfoLogger::endm;
              
              if(!(std::find(DEs.begin(), DEs.end(), de) != DEs.end())){
                  DEs.push_back(de);
                
                TH1F* h = new TH1F(TString::Format("QcMuonChambers_ADCamplitude_DE%03d", de),
                    TString::Format("QcMuonChambers - ADC amplitude (DE%03d)", de), 5000, 0, 5000);
                mHistogramADCamplitudeDE.insert( make_pair(de, h) );
                getObjectsManager()->startPublishing(h);

                float Xsize = 40*5;
                float Xsize2 = Xsize/2;
                float Ysize = 50;
                float Ysize2 = Ysize/2;
                  
                TH2F* h2 = new TH2F(TString::Format("QcMuonChambers_Nhits_DE%03d", de),
                    TString::Format("QcMuonChambers - Number of hits (DE%03d)", de), Xsize*2, -Xsize2, Xsize2, Ysize*2, -Ysize2, Ysize2);
                mHistogramNhitsDE.insert( make_pair(de, h2) );
                getObjectsManager()->startPublishing(h2);
                h2 = new TH2F(TString::Format("QcMuonChambers_Nhits_HighAmpl_DE%03d", de),
                    TString::Format("QcMuonChambers - Number of hits for Csum>500 (DE%03d)", de), Xsize*2, -Xsize2, Xsize2, Ysize*2, -Ysize2, Ysize2);
                mHistogramNhitsHighAmplDE.insert( make_pair(de, h2) );
                getObjectsManager()->startPublishing(h2);
//
//                  hclchg = new TH1F("hclsize", "Cluster charge distribution", 1000, 0, 3000);
//                  hclchg->GetXaxis()->SetTitle("Charge of cluster");
//                  hclchg->GetYaxis()->SetTitle("Count");
//                  mHistogramClchgDE.insert( make_pair(de, hclchg) );
//                  getObjectsManager()->startPublishing(hclchg);
//
//                  hclsize = new TH1F("hclsize", "Cluster size distribution", 20, 0, 20);
//                  hclsize->GetXaxis()->SetTitle("Size");
//                  hclsize->GetYaxis()->SetTitle("Count");
//                  mHistogramClsizeDE.insert( make_pair(de, hclsize) );
//                  getObjectsManager()->startPublishing(hclsize);
                 
              }
              
          }
      }
  }

  gPrintLevel = 5;

  flog = stdout; //fopen("/root/qc.log", "w");
  fprintf(stdout,"PhysicsDataProcessor initialization finished\n");
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
  fprintf(flog, "\n\n====================\nPhysicsDataProcessor::monitorData\n====================\n");

  printf("count: %d\n", count);
  if( (count % 1) == 0) {
    
        TFile f("/tmp/qc.root","RECREATE");
        for(int i = 0; i < 3*24; i++) {
          mHistogramNhits[i]->Write();
          mHistogramADCamplitude[i]->Write();
        }
      int nbDEs = DEs.size();
      for(int elem=0; elem<nbDEs; elem++){
        int de = DEs[elem];
        auto h = mHistogramADCamplitudeDE.find(de);
        if( (h != mHistogramADCamplitudeDE.end()) && (h->second != NULL) ) {
          h->second->Write();
          QcInfoLogger::GetInstance() << "On vient de write dans h->second ADCAmplitudeDE" << AliceO2::InfoLogger::InfoLogger::endm;
        }
        auto h2 = mHistogramNhitsDE.find(de);
        if( (h2 != mHistogramNhitsDE.end()) && (h2->second != NULL) ) {
          h2->second->Write();
          QcInfoLogger::GetInstance() << "On vient de write dans h2->second NHitsDE" << AliceO2::InfoLogger::InfoLogger::endm;
        }

        f.ls();
        f.Close();
      }
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
    //continue;

    mDecoder.processData( input.payload, header->payloadSize );

    std::vector<SampaHit>& hits = mDecoder.getHits();
    vector<int> padids_seen;
    if(gPrintLevel>=1) fprintf(flog, "hits.size()=%d\n", (int)hits.size());
      digits.clear();
    for(uint32_t i = 0; i < hits.size(); i++) {
      //continue;
        cout << "JE GERE LE HIT " << i << " MIOIUM" << endl;
      SampaHit& hit = hits[i];
      if(gPrintLevel>=1) fprintf(stdout,"hit[%d]: link_id=%d, ds_addr=%d, chan_addr=%d, CSUM=%d\n",
          i, hit.link_id, hit.ds_addr, hit.chan_addr, hit.csum);
      if(hit.link_id>=24 || hit.ds_addr>=40 || hit.chan_addr>=64 ) {
          cout << "Le continue chelou du rejet triste" << endl;
        fprintf(stdout,"hit[%d]: link_id=%d, ds_addr=%d, chan_addr=%d\n",
                  i, hit.link_id, hit.ds_addr, hit.chan_addr);
        continue;
      }
      //if( hit.csum > 500 ) {
      mHistogramNhits[hit.link_id]->Fill(hit.ds_addr, hit.chan_addr);
      mHistogramADCamplitude[hit.link_id]->Fill(hit.csum);
      //}

      int de = hit.pad.fDE;
      int dsid = hit.pad.fDsID;
      float padX = hit.pad.fX;
      float padY = hit.pad.fY;
      float padSizeX = hit.pad.fSizeX;
      float padSizeY = hit.pad.fSizeY;
        
        o2::mch::mapping::Segmentation segment(de);
        int padid = segment.findPadByFEE(dsid, hit.chan_addr);
        
        std::vector<int>::iterator itr = std::find(padids_seen.begin(), padids_seen.end(), padid);

        if (itr != padids_seen.cend()) {
            int indexfound = std::distance(padids_seen.begin(), itr);
            cout << "The padid is already present at index " <<
                        std::distance(padids_seen.begin(), itr) << endl;
            
            cout << "Update de la valeur csum du digit correspondant au padid" << padid << endl;
            mch::Digit* digit = digits.at(indexfound).get();
            int oldADC = digit->getADC();
            cout << "On passe de csum = " << oldADC << endl;
            digit->setADC(oldADC + hit.csum);
            cout << "A oldADC + hit.csum = " << oldADC << " + " << hit.csum << " = " << digit->getADC() << endl;
        }
        else {
            std::cout << "Padid not found, adding it:" << padid << endl;
            padids_seen.push_back( padid );
            
            cout << "Adding le digit correspondant à ce nouveau padid au vecteur digits" << endl;
            digits.push_back( std::make_unique<mch::Digit>() );
            mch::Digit* digit = digits.back().get();
            
            digit->setDetID(de);
            digit->setPadID(padid);
            digit->setADC(hit.csum);
        }
        

      if(gPrintLevel>=1)
        fprintf(flog, "mapping: link_id=%d ds_addr=%d chan_addr=%d  ==>  de=%d x=%f y=%f\n",
      	      hit.link_id, hit.ds_addr, hit.chan_addr, de, padX, padY);
      //if(pad.fX>=32 && pad.fX<=34 && pad.fY>=1.1 && pad.fY<=1.4)
      //  fprintf(flog, "mapping: link_id=%d ds_addr=%d chan_addr=%d  ==>  de=%d x=%f y=%f A=%d\n",
      //    hit.link_id, hit.ds_addr, hit.chan_addr, pad.fDE, pad.fX, pad.fY, hit.csum);

      auto h = mHistogramADCamplitudeDE.find(de);
      if(gPrintLevel>=1) fprintf(flog,"monitorData: h=%p\n", h->second);
      if( (h != mHistogramADCamplitudeDE.end()) && (h->second != NULL) ) {
        h->second->Fill(hit.csum);
      }

      if( hit.csum > 0 ) {
        auto h2 = mHistogramNhitsDE.find(de);
        if(gPrintLevel>=1) fprintf(flog,"monitorData: h2=%p\n", h2->second);
        if( (h2 != mHistogramNhitsDE.end()) && (h2->second != NULL) ) {
          int binx_min = h2->second->GetXaxis()->FindBin(padX-padSizeX/2+0.1);
          int binx_max = h2->second->GetXaxis()->FindBin(padX+padSizeX/2-0.1);
          int biny_min = h2->second->GetYaxis()->FindBin(padY-padSizeY/2+0.1);
          int biny_max = h2->second->GetYaxis()->FindBin(padY+padSizeY/2-0.1);
          for(int by = biny_min; by <= biny_max; by++) {
            float y = h2->second->GetYaxis()->GetBinCenter(by);
            for(int bx = binx_min; bx <= binx_max; bx++) {
              float x = h2->second->GetXaxis()->GetBinCenter(bx);
              if(gPrintLevel>=1) fprintf(flog,"monitorData: added hit to %f, %f\n", x, y);
              h2->second->Fill(x, y);
            }
          }
        }
      }
      if( hit.csum > 500 ) {
        auto h2 = mHistogramNhitsHighAmplDE.find(de);
        if(gPrintLevel>=1) fprintf(flog,"monitorData: h2=%p\n", h2->second);
        if( (h2 != mHistogramNhitsHighAmplDE.end()) && (h2->second != NULL) ) {
          int binx_min = h2->second->GetXaxis()->FindBin(padX-padSizeX/2+0.1);
          int binx_max = h2->second->GetXaxis()->FindBin(padX+padSizeX/2-0.1);
          int biny_min = h2->second->GetYaxis()->FindBin(padY-padSizeY/2+0.1);
          int biny_max = h2->second->GetYaxis()->FindBin(padY+padSizeY/2-0.1);
          for(int by = biny_min; by <= biny_max; by++) {
            float y = h2->second->GetYaxis()->GetBinCenter(by);
            for(int bx = binx_min; bx <= binx_max; bx++) {
              float x = h2->second->GetXaxis()->GetBinCenter(bx);
              if(gPrintLevel>=1) fprintf(flog,"monitorData: added hit to %f, %f\n", x, y);
              h2->second->Fill(x, y);
            }
          }
        }
      }
    }
      
      
      
      nDigits = getNumberOfDigits();
      digitsBuffer = (mch::Digit*)realloc(digitsBuffer, sizeof(mch::Digit) * nDigits);
      storeDigits(digitsBuffer);
      
        mch::PreClusterFinder preClusterFinder;
        mch::PreClusterBlock preClusterBlock;
        mch::Clustering clustering;
        
        std::string fname;
        preClusterFinder.init(fname);

        char* preClustersBuffer = NULL;
        std::vector<mch::Clustering::Cluster> clusters(0);

          // load the digits from the memory buffer and run the pre-clustering phase
          preClusterFinder.reset();
          preClusterFinder.loadDigits(digitsBuffer, nDigits);
          preClusterFinder.run();

          // get number of pre-clusters and store them into a memory buffer
          auto preClustersSize = preClusterBlock.getPreClustersBufferSize(preClusterFinder);
          printf("preClustersSize: %d\n", (int)preClustersSize);
          preClustersBuffer = (char*)realloc(preClustersBuffer, preClustersSize);
          preClusterBlock.storePreClusters(preClusterFinder, preClustersBuffer);

          //continue;
          printf("\n\n==========\nReading clusters\n\n");

          std::vector<mch::PreClusterStruct> preClusters;
          preClusterBlock.readPreClusters(preClusters, preClustersBuffer, preClustersSize);
            
            printf("\n\n==========\nRunning Clustering\n\n");
            
          //Runs the clustering of preClusters following a CenterOfGravity algorithm. Fills clusters.
      //    clustering.runFinderCOG(preClusters, clusters);
      //    printf("Number of clusters obtained and saved: %lu\n", clusters.size());
            
         clustering.runFinderSimpleFit(preClusters, clusters);
            
       //     clustering.runFinderGaussianFit(preClusters, clusters);
      
//      // Distribution of cluster size
//
//      auto hclsize = mHistogramClsizeDE.find(de);
//      for(int i=0; i<preClusters.size(); i++){
//          hclsize->Fill(preClusters[i].nDigits);
//      }
//
//      // Distrubution of cluster charge
//
//      int sizedigit;
//      mch::Digit digittmp;
//
//      auto hclchg = mHistogramClchgDE.find(de);
//      for(int i=0; i<preClusters.size(); i++){
//          const mch::Digit* ptrdigit = preClusters[i].digits;
//          float chgsum = 0;
//          for (int j=0; j<preClusters[i].nDigits; j++){
//              digittmp = *ptrdigit;
//              chgsum += digittmp.getADC();
//              sizedigit = sizeof(digittmp);
//              ptrdigit++;
//          }
//          hclchg->Fill(chgsum);
//     }
      
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

ssize_t PhysicsDataProcessor::getNumberOfDigits()
{
  return digits.size();
}


void PhysicsDataProcessor::storeDigits(void* bufferPtr)
{
  mch::Digit* ptr = (mch::Digit*)bufferPtr;
  for(unsigned int di = 0; di < digits.size(); di++) {

    memcpy(ptr, digits[di].get(), sizeof(mch::Digit));
    ptr += 1;
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
