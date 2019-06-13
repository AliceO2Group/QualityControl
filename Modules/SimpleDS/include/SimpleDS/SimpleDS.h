///
/// \file   SimpleDS.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#ifndef QC_MODULE_SIMPLEDS_SIMPLEDS_H
#define QC_MODULE_SIMPLEDS_SIMPLEDS_H

#include "QualityControl/TaskInterface.h"

#include <vector>
#include <deque>
#include <memory>
#include "Rtypes.h"		// for Digitizer::Class, Double_t, ClassDef, etc
#include "TObject.h"		// for TObject
#include "FairTask.h"
#include "TPaveText.h"
#include "TGaxis.h"
#include "TEllipse.h"

#include "ITSMFTReconstruction/RawPixelReader.h"


#include "DataFormatsITSMFT/ROFRecord.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include <fstream>
#include "Framework/DataProcessorSpec.h"
#include "Framework/Task.h"
#include "ITSMFTReconstruction/Clusterer.h"
#include "uti.h"

#include "ITSBase/GeometryTGeo.h"
#include "DetectorsBase/GeometryManager.h"

#include "ITSMFTReconstruction/DigitPixelReader.h"


class TH1F;

using namespace o2::quality_control::core;

namespace o2
{
	namespace quality_control_modules
	{
		namespace simpleds
		{

			/// \brief Example Quality Control DPL Task
			/// It is final because there is no reason to derive from it. Just remove it if needed.
			/// \author Barthelemy von Haller
			/// \author Piotr Konopka
			class SimpleDS /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
			{

				using ChipPixelData = o2::itsmft::ChipPixelData;
				using PixelReader = o2::itsmft::PixelReader;


				public:
				/// \brief Constructor
				SimpleDS();
				/// Destructor
				~SimpleDS() override;

				// Definition of the methods for the template method pattern
				void initialize(o2::framework::InitContext& ctx) override;
				void startOfActivity(Activity& activity) override;
				void startOfCycle() override;
				void monitorData(o2::framework::ProcessingContext& ctx) override;
				void endOfCycle() override;
				void endOfActivity(Activity& activity) override;
				void reset() override;
				void setNChips(int n)
				{
					mChips.resize(n);
					mChipsOld.resize(n);
				}
				void ConfirmXAxis(TH1 *h);
				void ReverseYAxis(TH1 *h);



				private:

				ChipPixelData* mChipData = nullptr; 
				std::vector<ChipPixelData> mChips;
				std::vector<ChipPixelData> mChipsOld;
				o2::itsmft::PixelReader* mReader = nullptr; 
				std::unique_ptr<o2::itsmft::DigitPixelReader> mReaderMC;    
				//std::unique_ptr<o2::itsmft::RawPixelReader<o2::itsmft::ChipMappingITS>> mReaderRaw; 
				o2::itsmft::RawPixelReader<o2::itsmft::ChipMappingITS> mReaderRaw;		
				o2::itsmft::ChipInfo chipInfo;
				UInt_t mCurrROF = o2::itsmft::PixelData::DummyROF; 
				int* mCurr; // pointer on the 1st row of currently processed mColumnsX
				int* mPrev; // pointer on the 1st row of previously processed mColumnsX
				static constexpr int   NCols = 1024;
				static constexpr int   NRows = 512;
				const int NColHis = 1024;
				const int NRowHis = 512;
				int XTicks;
				int YTicks;
				int SizeReduce = 4;



				int DivisionStep = 32;
				static constexpr int   NPixels = NRows*NCols;
				const int NLay1 = 108;
				static constexpr int NLayer = 7;
				const int NEventMax[NLayer] = {150,150,150,150,150,150,150};

				int ChipBoundary[NLayer + 1] ={0,108,252,432,3120,6480,14712,24120}; 
				int NChipLay[NLayer];
				int NColStave[NLayer];
				UShort_t row;
				UShort_t col; 
				UShort_t rowCS;
				UShort_t colCS; 
				UShort_t rowLay6;
				UShort_t colLay6; 

				int lay, sta, ssta, mod, chip;
				//	TH2D * ChipStave[NLayer]; 
				TH1D * OccupancyPlot[NLayer];
				TH2S * LayEtaPhi[NLayer]; 
				TH2S * LayChipStave[NLayer]; 
				const int NStaves[NLayer] = {12,16,20,24,30,42,48};
				int NStaveChip[NLayer];
				TH2S * HITMAP[9];
				TH2S * Lay1HIT[12];
				TH2S * HITMAP6[18];
				int ChipIndex6;

				void swapColumnBuffers()
				{
					int* tmp = mCurr;
					mCurr = mPrev;
					mPrev = tmp;
				}
				const std::vector<o2::itsmft::Digit>* mDigits = nullptr;
				void resetColumn(int* buff)
				{
					std::memset(buff, -1, sizeof(int) * NRows);

				}
				Int_t mIdx = 0;
				//const std::string inpName = "rawits.bin";
				//const std::string inpName = "thrscan3_nchips8_ninj25_chrange0-50_rows512.raw";
				std::string inpName = "Split9.bin";

				o2::ITS::GeometryTGeo * gm = o2::ITS::GeometryTGeo::Instance();
				double AveOcc;
				UShort_t ChipID; 


				TFile * fout;
				const int NEta = 9;
				const double EtaMin = -2.40;
				const double EtaMax = 2.40;
				const int NPhi = 12;
				const double PhiMin = -2.90;
				const double PhiMax = 2.90;
				const int NChipsSta = 9;
				const int NSta1 = NLay1/NChipsSta;
				int numOfChips;
				double eta;
				double phi;
				static constexpr int  NError = 11;
				std::array<unsigned int,NError> Errors;
				std::array<unsigned int,NError> ErrorPre;
				std::array<unsigned int,NError> ErrorPerFile;
	
				//unsigned int Error[NError];
				double ErrorMax;
				TPaveText *pt[NError];
				TPaveText * ptFileName;
				TPaveText * ptNFile;
				TPaveText * ptNEvent;
				TPaveText * bulbGreen;
				TPaveText * bulbRed;
				TPaveText * bulbYellow;


				TH1D * ErrorPlots = new TH1D("ErrorPlots","ErrorPlots",NError,0.5,NError+0.5);
				TH1D * FileNameInfo = new TH1D("FileNameInfo","FileNameInfo",5,0,1);
				TString ErrorType[NError] ={"Error ID 1: ErrPageCounterDiscontinuity","Error ID 2: ErrRDHvsGBTHPageCnt","Error ID 3: ErrMissingGBTHeader","Error ID 4: ErrMissingGBTTrailer","Error ID 5: ErrNonZeroPageAfterStop","Error ID 6: ErrUnstoppedLanes","Error ID 7: ErrDataForStoppedLane","Error ID 8: ErrNoDataForActiveLane","Error ID 9: ErrIBChipLaneMismatch","Error ID 10: ErrCableDataHeadWrong","Error ID 11: Jump in RDH_packetCounter"};
				TH2S * ChipStave = new TH2S("ChipStaveCheck","ChipStaveCheck",9,0,9,100,0,1500);
				const int NFiles = 6;
				TH2I * ErrorFile = new TH2I("ErrorFile","ErrorFile",NFiles+1,-0.5,NFiles+0.5,NError,0.5,NError+0.5);
				TH1D * InfoCanvas = new TH1D("InfoCanvas","InfoCanvas",3,-0.5,2.5);
				TEllipse *bulb = new TEllipse(0.2,0.75,0.30,0.20);
				TGaxis *newXaxis;
				TGaxis *newYaxis;
	

				int TotalDigits = 0;
				int NEvent;
				int NEventInRun;
				int NEventPre;
				int OccupancyCounter;
				int ChipIDPre;
				TString FileNamePre;
				int RunIDPre;
				int FileIDPre;
				int TotalFileDone;
				int FileRest;

			};

		} // namespace simpleds
	} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_SIMPLEDS_SIMPLEDS_H

