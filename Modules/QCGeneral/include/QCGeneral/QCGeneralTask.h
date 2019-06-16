///
/// \file   QCGeneralTask.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#ifndef QC_MODULE_QCGENERAL_QCGENERALTASK_H
#define QC_MODULE_QCGENERAL_QCGENERALTASK_H

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
		namespace qcgeneral
		{

			/// \brief Example Quality Control DPL Task
			/// It is final because there is no reason to derive from it. Just remove it if needed.
			/// \author Barthelemy von Haller
			/// \author Piotr Konopka
			class QCGeneralTask /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
			{
				public:
					/// \brief Constructor
					QCGeneralTask();
					/// Destructor
					~QCGeneralTask() override;

					// Definition of the methods for the template method pattern
					void initialize(o2::framework::InitContext& ctx) override;
					void startOfActivity(Activity& activity) override;
					void startOfCycle() override;
					void monitorData(o2::framework::ProcessingContext& ctx) override;
					void endOfCycle() override;
					void endOfActivity(Activity& activity) override;
					void reset() override;

				private:
					TH1D * InfoCanvas = new TH1D("InfoCanvas","InfoCanvas",3,-0.5,2.5);
					TEllipse *bulb = new TEllipse(0.2,0.75,0.30,0.20);	
					TPaveText * ptFileName;
					TPaveText * ptNFile;
					TPaveText * ptNEvent;
					TPaveText * bulbGreen;
					TPaveText * bulbRed;
					TPaveText * bulbYellow;
					UShort_t ChipID; 
					UShort_t row;
					UShort_t col; 
					int NEvent;
					int RunIDPre;
					int FileIDPre;	
					int NEventPre;
					int FileRest;
					int TotalFileDone;	
			};

		} // namespace qcgeneral
	} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_QCGENERAL_QCGENERALTASK_H

