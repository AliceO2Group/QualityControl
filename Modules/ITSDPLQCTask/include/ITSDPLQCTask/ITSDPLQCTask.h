///
/// \file   ITSDPLQCTask.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#ifndef QC_MODULE_ITSDPLQCTASK_ITSDPLQCTASK_H
#define QC_MODULE_ITSDPLQCTASK_ITSDPLQCTASK_H

#include "QualityControl/TaskInterface.h"

#include <vector>
#include <deque>
#include <memory>
#include "Rtypes.h"		// for Digitizer::Class, Double_t, ClassDef, etc
#include "TObject.h"		// for TObject
#include "FairTask.h"





#include "DataFormatsITSMFT/ROFRecord.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include <fstream>
#include "Framework/DataProcessorSpec.h"
#include "Framework/Task.h"
#include "../../../../O2/Detectors/ITSMFT/common/reconstruction/include/ITSMFTReconstruction/DigitPixelReader.h"
#include "../../../../O2/Detectors/ITSMFT/common/reconstruction/include/ITSMFTReconstruction/RawPixelReader.h"



#include "ITSBase/GeometryTGeo.h"
#include "DetectorsBase/GeometryManager.h"
#include "ITSMFTBase/GeometryTGeo.h"

#include "uti.h"

class TH1F;

using namespace o2::quality_control::core;

namespace o2
{
	namespace quality_control_modules
	{
		namespace itsdplqctask
		{

			/// \brief Example Quality Control DPL Task
			/// It is final because there is no reason to derive from it. Just remove it if needed.
			/// \author Barthelemy von Haller
			/// \author Piotr Konopka
			class ITSDPLQCTask /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
			{

				using ChipPixelData = o2::ITSMFT::ChipPixelData;
				using PixelReader = o2::ITSMFT::PixelReader;

				public:
				/// \brief Constructor
				ITSDPLQCTask();
				/// Destructor
				~ITSDPLQCTask() override;

				// Definition of the methods for the template method pattern
				void initialize(o2::framework::InitContext& ctx) override;
				void startOfActivity(Activity& activity) override;
				void startOfCycle() override;
				void process (PixelReader& r);
				void monitorData(o2::framework::ProcessingContext& ctx) override;
				void endOfCycle() override;
				void endOfActivity(Activity& activity) override;
				void reset() override;
				std::vector<o2::ITSMFT::Digit> mDigitsArray;                     
				std::vector<o2::ITSMFT::Digit>* mDigitsArrayPtr = &mDigitsArray; 
				UInt_t getCurrROF() const { return mCurrROF; }
				void setNChips(int n)
				{
					mChips.resize(n);
					mChipsOld.resize(n);
				}



				private:
				TH1F* mHistogram;
				ChipPixelData* mChipData = nullptr; 
				std::vector<ChipPixelData> mChips;
				std::vector<ChipPixelData> mChipsOld;
				o2::ITSMFT::PixelReader* mReader = nullptr; 
				std::unique_ptr<o2::ITSMFT::DigitPixelReader> mReaderMC;    
				//std::unique_ptr<o2::ITSMFT::RawPixelReader<o2::ITSMFT::ChipMappingITS>> mReaderRaw; 
				o2::ITSMFT::RawPixelReader<o2::ITSMFT::ChipMappingITS> mReaderRaw;		
				o2::ITSMFT::ChipInfo chipInfo;
				UInt_t mCurrROF = o2::ITSMFT::PixelData::DummyROF; 
				int* mCurr; // pointer on the 1st row of currently processed mColumnsX
				int* mPrev; // pointer on the 1st row of previously processed mColumnsX
				static constexpr int   NCols = 1024;
				static constexpr int   NRows = 512;
				const int NColHis = 1024;
				const int NRowHis = 512;

				static constexpr int   NPixels = NRows*NCols;
				const int NLay1 = 108;
				double Occupancy[24120];
				static constexpr int NLayer = 7;
				const int NEventMax[NLayer] = {50,50,50,100,100,150,150};

				int ChipBoundary[NLayer + 1] ={0,108,252,432,3120,6480,14712,24120}; 
				int NChipLay[NLayer];

				int lay, sta, ssta, mod, chip;
				TH2D * ChipStave[NLayer]; 
				TH1D * ChipProj[NLayer];
				TH2D * LayEtaPhi[NLayer]; 
				TH2D * LayChipStave[NLayer]; 

				const int NStaves[NLayer] = {12,16,20,24,30,42,48};
				int NStaveChip[NLayer];
				TH2D * HIGMAP[9];
				void swapColumnBuffers()
				{
					int* tmp = mCurr;
					mCurr = mPrev;
					mPrev = tmp;
				}
				const std::vector<o2::ITSMFT::Digit>* mDigits = nullptr;
				void resetColumn(int* buff)
				{
					std::memset(buff, -1, sizeof(int) * NRows);

				}
				Int_t mIdx = 0;
				const std::string inpName = "rawits.bin";

				o2::ITS::GeometryTGeo * gm = o2::ITS::GeometryTGeo::Instance();
				double AveOcc;
				UShort_t ChipID; 
				int ActPix;
				TFile * fout;
				const int NEta = 9;
				const double EtaMin = -2.40;
				const double EtaMax = 2.40;
				const int NPhi = 12;
				const double PhiMin = -2.90;
				const double PhiMax = 2.90;
				const int NChipsSta = 9;
				const int NSta1 = NLay1/NChipsSta;
				double eta;
				double phi;
				static constexpr int  NError = 11;
				int Error[NError];
				TH1D * ErrorPlots[NError];
				TString ErrorType[NError] ={"ErrGarbageAfterPayload","ErrPageCounterDiscontinuity","ErrRDHvsGBTHPageCnt","ErrMissingGBTHeader","ErrMissingGBTTrailer","ErrNonZeroPageAfterStop","ErrUnstoppedLanes","ErrDataForStoppedLane","ErrNoDataForActiveLane","ErrIBChipLaneMismatch","ErrCableDataHeadWrong"};
			};


		} // namespace itsdplqctask
	} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_ITSDPLQCTASK_ITSDPLQCTASK_H

