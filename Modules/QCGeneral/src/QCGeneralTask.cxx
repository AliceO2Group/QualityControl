///
/// \file   QCGeneralTask.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///


#include <sstream>

#include <TStopwatch.h>
#include "DataFormatsParameters/GRPObject.h"
#include "FairLogger.h"
#include "FairRunAna.h"
#include "FairFileSource.h"
#include "FairRuntimeDb.h"
#include "FairParRootFileIo.h"
#include "FairSystemInfo.h"

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>

#include "QualityControl/FileFinish.h"
#include "QualityControl/QcInfoLogger.h"
#include "SimpleDS/SimpleDS.h"
#include "DetectorsBase/GeometryManager.h"
#include "ITSBase/GeometryTGeo.h"
#include "ITSMFTReconstruction/DigitPixelReader.h"
#include <algorithm>


using o2::itsmft::Digit;



using namespace std;
using namespace o2::itsmft;
using namespace o2::ITS;

namespace o2
{
	namespace quality_control_modules
	{
		namespace qcgeneral
		{

			QCGeneralTask::QCGeneralTask() : TaskInterface(){
			}

			QCGeneralTask::~QCGeneralTask() {
			}

			void QCGeneralTask::initialize(o2::framework::InitContext& ctx)
			{
				QcInfoLogger::GetInstance() << "initialize QCGeneralTask" << AliceO2::InfoLogger::InfoLogger::endm;

				InfoCanvas->SetTitle("QC Process Information Canvas");
				InfoCanvas->GetListOfFunctions()->Add(ptFileName);
				InfoCanvas->GetListOfFunctions()->Add(ptNFile);
				InfoCanvas->GetListOfFunctions()->Add(ptNEvent);
				InfoCanvas->GetListOfFunctions()->Add(bulb);
				InfoCanvas->GetListOfFunctions()->Add(bulbRed);	
				InfoCanvas->GetListOfFunctions()->Add(bulbYellow);
				InfoCanvas->GetListOfFunctions()->Add(bulbGreen);
				getObjectsManager()->startPublishing(InfoCanvas);
			}

			void QCGeneralTask::startOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;

			}

			void QCGeneralTask::startOfCycle()
			{
				QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void QCGeneralTask::monitorData(o2::framework::ProcessingContext& ctx)
			{
				QcInfoLogger::GetInstance() << "START DOING QC General" << AliceO2::InfoLogger::InfoLogger::endm;

				int InfoFile = ctx.inputs().get<int>("Finish");
				FileFinish = InfoFile % 10;
				FileRest = (InfoFile - FileFinish)/10;
				if(FileFinish == 0) bulb->SetFillColor(kGreen);
				if(FileFinish == 1 && FileRest > 1) bulb->SetFillColor(kYellow);
				if(FileFinish == 1 && FileRest == 1) bulb->SetFillColor(kRed);
				int RunID = ctx.inputs().get<int>("Run");
				int FileID = ctx.inputs().get<int>("File");
				TString RunName = Form("Run%d",RunID);
				TString FileName = Form("infiles/run000%d/data-link%d",RunID,FileID);

				if(RunIDPre != RunID || FileIDPre != FileID){
					QcInfoLogger::GetInstance() << "For the Moment: RunID = "  << RunID << "  FileID = " << FileID << AliceO2::InfoLogger::InfoLogger::endm;
					FileNameInfo->Fill(0.5);
					FileNameInfo->SetTitle(Form("Current File Name: %s",FileName.Data()));
					TotalFileDone = TotalFileDone + 1;
					ptFileName->Clear();
					ptNFile->Clear();
					ptFileName->AddText(Form("File Being Proccessed: %s",FileName.Data()));
					ptNFile->AddText(Form("File Processed: %d ",TotalFileDone));
				}
				RunIDPre = RunID;
				FileIDPre = FileID;

				int ResetDecision = ctx.inputs().get<int>("in");
				QcInfoLogger::GetInstance() << "Reset Histogram Decision = " << ResetDecision << AliceO2::InfoLogger::InfoLogger::endm;
				if(ResetDecision == 1) reset();

				auto digits = ctx.inputs().get<const std::vector<o2::itsmft::Digit>>("digits");

				for (auto&& pixeldata : digits) {
					ChipID = pixeldata.getChipIndex();
					col = pixeldata.getColumn();
					row = pixeldata.getRow();
					NEvent = pixeldata.getROFrame();
					if (NEvent%1000000==0 && NEvent > 0) cout << "ChipID = " << ChipID << "  col = " << col << "  row = " << row << "  NEvent = " << NEvent << endl;
					NEventPre = NEvent;
					//Add Your Analysis Here if you use digits as input//

				}
				
				digits.clear();
			}

			void QCGeneralTask::endOfCycle()
			{
				QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void QCGeneralTask::endOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void QCGeneralTask::reset()
			{
				// clean all the monitor objects here

				QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
			}

		} // namespace qcgeneral
	} // namespace quality_control_modules
} // namespace o2

