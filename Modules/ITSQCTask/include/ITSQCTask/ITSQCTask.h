///
/// \file   ITSQCTask.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#ifndef QC_MODULE_ITSQCTASK_ITSQCTASK_H
#define QC_MODULE_ITSQCTASK_ITSQCTASK_H

#include "QualityControl/TaskInterface.h"

#include <vector>
#include <deque>
#include <memory>
#include "Rtypes.h"		// for Digitizer::Class, Double_t, ClassDef, etc
#include "TObject.h"		// for TObject
#include "FairTask.h"



#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/common/reconstruction/include/ITSMFTReconstruction/DigitPixelReader.h"


#include "DataFormatsITSMFT/ROFRecord.h"
#include "SimulationDataFormat/MCCompLabel.h"
//#include "/home/alidock/alice/O2/Detectors/ITSMFT/common/simulation/include/ITSMFTSimulation/Digitizer.h"
//#include "/home/alidock/alice/O2/Detectors/ITSMFT/common/base/include/ITSMFTBase/SegmentationAlpide.h"
#include <fstream>
#include "Framework/DataProcessorSpec.h"
#include "Framework/Task.h"
#include "ITSMFTReconstruction/Clusterer.h"
#include "uti.h"

#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/base/include/ITSBase/GeometryTGeo.h"
#include "DetectorsBase/GeometryManager.h"
#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/common/base/include/ITSMFTBase/GeometryTGeo.h"

#include "ITSMFTReconstruction/DigitPixelReader.h"
#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/QCWorkFlow/include/ITSQCWorkflow/HisAnalyzerSpec.h"



using namespace o2::framework;
using namespace o2::ITSMFT;

class TH1F;

using namespace o2::quality_control::core;

namespace o2
{
	namespace quality_control_modules
	{
		namespace itsqctask
		{

			/// \brief Example Quality Control DPL Task
			/// It is final because there is no reason to derive from it. Just remove it if needed.
			/// \author Barthelemy von Haller
			/// \author Piotr Konopka
			class ITSQCTask /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
			{

				using ChipPixelData = o2::ITSMFT::ChipPixelData;
				using PixelReader = o2::ITSMFT::PixelReader;	
				public:
				/// \brief Constructor
				ITSQCTask();
				/// Destructor
				~ITSQCTask() override;
				inline void setDigits (std::vector < o2::ITSMFT::Digit > *dig);
				std::vector<o2::ITSMFT::Digit> mDigitsArray;                     
				std::vector<o2::ITSMFT::Digit>* mDigitsArrayPtr = &mDigitsArray; 
				UInt_t getCurrROF() const { return mCurrROF; }
				void setNChips(int n)
				{
					mChips.resize(n);
					mChipsOld.resize(n);
				}


				// Definition of the methods for the template method pattern
				void initialize(o2::framework::InitContext& ctx) override;
				void startOfActivity(Activity& activity) override;
				void startOfCycle() override;
				void monitorData(o2::framework::ProcessingContext& ctx) override;
				void process (PixelReader& r);
				void endOfCycle() override;
				void endOfActivity(Activity& activity) override;
				void reset() override;
				//				o2::ITSMFT::GeometryTGeo* gm = o2::ITSMFT::GeometryTGeo::Instance();



				private:
				TH1F* mHistogram;

				ChipPixelData* mChipData = nullptr; 
				std::vector<ChipPixelData> mChips;
				std::vector<ChipPixelData> mChipsOld;
				o2::ITSMFT::PixelReader* mReader = nullptr; 
				std::unique_ptr<o2::ITSMFT::DigitPixelReader> mReaderMC;    
				//				std::unique_ptr<o2::ITSMFT::RawPixelReader<o2::ITSMFT::ChipMappingITS>> mReaderRaw; 
				UInt_t mCurrROF = o2::ITSMFT::PixelData::DummyROF; 
				int* mCurr; // pointer on the 1st row of currently processed mColumnsX
				int* mPrev; // pointer on the 1st row of previously processed mColumnsX
				static constexpr int   NCols = 1024;
				static constexpr int   NRows = 512;
				static constexpr int   NPixels = NRows*NCols;
				const int NLay1 = 108;
				const int NEventMax = 20;
				double Occupancy[108];
				int lay, sta, ssta, mod, chip;
				TH2D * ChipStave = new TH2D("ChipStave","ChipStave",NLay1,0,NLay1,NEventMax,0,NEventMax);
				TH1D * ChipProj = new TH1D("ChipProj","ChipProj",NLay1,0,NLay1);
				TH2D * Lay1EtaPhi = new TH2D("Lay1EtaPhi","Lay1EtaPhi",NEta,EtaMin,EtaMax,NPhi,PhiMin,PhiMax);
				TH2D * Lay1ChipStave = new TH2D("Lay1ChipStave","Lay1ChipStave",NChipsSta,0,NChipsSta,NSta1,0,NSta1);
				o2::ITS::GeometryTGeo * gm = o2::ITS::GeometryTGeo::Instance();

				void swapColumnBuffers()
				{
					int* tmp = mCurr;
					mCurr = mPrev;
					mPrev = tmp;
				}

				void resetColumn(int* buff)
				{
					std::memset(buff, -1, sizeof(int) * NRows);

				}

				const std::string inpName = "itsdigits.root";

				double AveOcc;
				UShort_t ChipID; 
				int ActPix;
				TFile * fout;
				const int NEta = 10;
				const double EtaMin = -2.5;
				const double EtaMax = 2.5;
				const int NPhi = 10;
				const double PhiMin = -3.15;
				const double PhiMax = 3.15;
				const int NChipsSta = 9;
				const int NSta1 = NLay1/NChipsSta;
				double eta;
				double phi;
			};

		} // namespace itsqctask
	} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_ITSQCTASK_ITSQCTASK_H
