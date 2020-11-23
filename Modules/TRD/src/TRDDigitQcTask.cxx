
///
/// \file   TRDDigitQcTask.cxx
/// \author Tokozani Mtetwa
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include "TRDBase/Digit.h"
#include "DataFormatsTRD/Constants.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/TaskInterface.h"
#include "QualityControl/QcInfoLogger.h"
#include "TRD/TRDDigitQcTask.h"

namespace o2::quality_control_modules::trd
{
  TRDDigitQcTask::~TRDDigitQcTask()
  {
    if (mADC) {
      delete mADC;
    }
    if (mADCperTimeBinAllDetectors) {
      delete mADCperTimeBinAllDetectors;
    }
    if (mprofADCperTimeBinAllDetectors) {
      delete mprofADCperTimeBinAllDetectors;
    }
  }

  void TRDDigitQcTask::initialize(o2::framework::InitContext& /*ctx*/)
  {
    ILOG(Info) << "initialize SimpleTrdTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

    // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
    if (auto param = mCustomParameters.find("trdDigits"); param != mCustomParameters.end()) {
      ILOG(Info) << "Custom parameter - trdDigits: " << param->second << ENDM;
    }

    mADC = new TH1F("hADC", ";ADC value;Counts", 1024, 0, 1023);
    getObjectsManager()->startPublishing(mADC);
    //getObjectsManager()->setDisplayHint(mADC->GetName(), "LOGY");
    getObjectsManager()->addMetadata(mADC->GetName(), "custom", "34");

    mADCperTimeBinAllDetectors = new TH2F("ADCperTimeBinAllDetectors", "ADC distribution for all chambers for each time bin;Time bin;ADC", 31, -0.5, 30.5 , 1014, 0, 1023);
    getObjectsManager()->startPublishing(mADCperTimeBinAllDetectors);
    //getObjectsManager()->setDefaultDrawOptions(mADCperTimeBinAllDetectors->GetName(), "LEGO2");
    getObjectsManager()->addMetadata(mADCperTimeBinAllDetectors->GetName(), "custom", "34");

    mprofADCperTimeBinAllDetectors = new TProfile("profADCperTimeBinAllDetectors", " prof ADC distribution for all chambers for each time bin;Time bin;ADC", 31, -0.5, 30.5);
    getObjectsManager()->startPublishing(mprofADCperTimeBinAllDetectors);
    //getObjectsManager()->setDefaultDrawOptions(mprofADCperTimeBinAllDetectors->GetName(), "");
    getObjectsManager()->addMetadata(mprofADCperTimeBinAllDetectors->GetName(), "custom", "34");
  }

  void TRDDigitQcTask::startOfActivity(Activity& /*activity*/)
  {
    ILOG(Info) << "startOfActivity" << ENDM;

    mADC->Reset();
    mADCperTimeBinAllDetectors->Reset();
    mprofADCperTimeBinAllDetectors->Reset();
  } //set stats/stacs

  void TRDDigitQcTask::startOfCycle()
  {
    ILOG(Info) << "startOfCycle" << ENDM;
  }

  void TRDDigitQcTask::monitorData(o2::framework::ProcessingContext& ctx)
  {
    for (auto&& input : ctx.inputs()) {
      // get message header
      if (input.header != nullptr && input.payload != nullptr) {
        const auto* header = header::get<header::DataHeader*>(input.header);
        // get payload of a specific input, which is a char array.
        ILOG(Info) << "payload size: " << (header->payloadSize) << ENDM;
        mHistogram->Fill(header->payloadSize);

        const auto inputDigits = ctx.inputs().get<gsl::span<o2::trd::Digit>>("random");
        std::vector<o2::trd::Digit> msgDigits(inputDigits.begin(), inputDigits.end());
        for(auto digit : msgDigits )
        {
          int det = digit.getDetector();
          int row = digit.getRow();
          int pad = digit.getPad();
          auto adcs = digit.getADC();

          ILOG(Info) << " Detectors" << det <<ENDM;

          for (int tb = 0; tb < o2::trd::constants::TIMEBINS; ++tb) {
              int adc = adcs[tb];

              mADC->Fill(adc);
              mADCperTimeBinAllDetectors->Fill(tb, adc);
              mprofADCperTimeBinAllDetectors->Fill(tb, adc);
          }
          mADC->Draw();
          mADCperTimeBinAllDetectors->Draw();
          mprofADCperTimeBinAllDetectors->Draw();
         }
       }
     }
   }

   void TRDDigitQcTask::endOfCycle()
   {
     ILOG(Info) << "endOfCycle" << ENDM;
   }

   void TRDDigitQcTask::endOfActivity(Activity& /*activity*/)
   {
     ILOG(Info) << "endOfActivity" << ENDM;
   }

   void TRDDigitQcTask::reset()
   {
     // clean all the monitor objects here
     ILOG(Info) << "Resetting the histogram" << ENDM;
     mADC->Reset();
     mADCperTimeBinAllDetectors->Reset();
     mprofADCperTimeBinAllDetectors->Reset();
   }
 } // namespace o2::quality_control_modules::trd
