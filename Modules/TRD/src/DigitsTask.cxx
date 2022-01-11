
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TLine.h>

#include <TProfile.h>
#include "DataFormatsTRD/Constants.h"
#include "DataFormatsTRD/Digit.h"
#include "DataFormatsTRD/Tracklet64.h"
#include "DataFormatsTRD/TriggerRecord.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/TaskInterface.h"
#include "QualityControl/QcInfoLogger.h"
#include "TRD/DigitsTask.h"
#include <Framework/InputRecord.h>

namespace o2::quality_control_modules::trd
{
  DigitsTask::~DigitsTask()
  {
  }

  void DigitsTask::drawLinesMCM(TH2F* histo)
  {

    TLine* l;
    Int_t nPos[o2::trd::constants::NSTACK - 1] = { 16, 32, 44, 60 };

    for (Int_t iStack = 0; iStack < o2::trd::constants::NSTACK - 1; ++iStack) {
      l = new TLine(nPos[iStack] - 0.5, -0.5, nPos[iStack] - 0.5, 47.5);
      l->SetLineStyle(2);
      histo->GetListOfFunctions()->Add(l);
    }

    for (Int_t iLayer = 0; iLayer < o2::trd::constants::NLAYER; ++iLayer) {
      l = new TLine(0.5, iLayer * 8 - 0.5, 75.5, iLayer * 8 - 0.5);
      l->SetLineStyle(2);
      histo->GetListOfFunctions()->Add(l);
    }
  }

  void DigitsTask::buildHistograms()
  {
    mDigitHCID.reset(new TH1F("digithcid", "Digit distribution over Halfchambers", 1080, 0, 1080));
    getObjectsManager()->startPublishing(mDigitHCID.get());
    mDigitsPerEvent.reset(new TH1F("digitsperevent", "Digits per Event", 10000, 0, 10000));
    getObjectsManager()->startPublishing(mDigitsPerEvent.get());

    mClsAmpCh.reset(new TH1F("ClsAmpCh", "Reconstructed mean amplitude;Amplitude (ADC);# chambers", 100, 25, 125));
    mClsAmpCh->GetXaxis()->SetTitle("Amplitude (ADC)");
    mClsAmpCh->GetYaxis()->SetTitle("# chambers");
    getObjectsManager()->startPublishing(mClsAmpCh.get());

    mClsNTb.reset(new TH1F("ClsNTb", "ClsNTb", 30, -0.5, 29.5));
    mClsNTb->SetTitle("Number of clusters;Timebin");
    mClsNTb->GetXaxis()->SetTitle("Number Of Clusters");
    mClsNTb->GetYaxis()->SetTitle("Timebin");
    getObjectsManager()->startPublishing(mClsNTb.get());

    mClsChargeFirst.reset(new TH2F("ClsChargeFirst", "ClsChargeFirst", 100, 0, 1000, 101, -0.2, 0.2));
    mClsChargeFirst->GetXaxis()->SetTitle("Timebin of the maximum signal");
    mClsChargeFirst->GetYaxis()->SetTitle("Timebin");
    getObjectsManager()->startPublishing(mClsChargeFirst.get());
    getObjectsManager()->setDefaultDrawOptions(mClsChargeFirst->GetName(), "COLZ");

    mClsChargeTb.reset(new TH1F("ClsChargeTb", "ClsChargeTb", 30, -0.5, 29.5));
    getObjectsManager()->startPublishing(mClsChargeTb.get());

    mClsChargeTbCycle.reset(new TH1F("ClsChargeTbCycle", "ClsChargeTbCycle", 30, -0.5, 29.5));
    getObjectsManager()->startPublishing(mClsChargeTbCycle.get());

    mClsAmp.reset(new TH1F("ClsAmp", "Amplitude of clusters", 200, -0.5, 1999.5));
    mClsAmp->GetXaxis()->SetTitle("Amplitude (ADC)");
    mClsAmp->GetYaxis()->SetTitle("Counts");
    getObjectsManager()->startPublishing(mClsAmp.get());

    mClsAmpDrift.reset(new TH1F("ClsAmpDrift", "Amplitude of clusters in drift region", 500, -0.5, 999.5));
    mClsAmpDrift->GetXaxis()->SetTitle("Amplitude (ADC)");
    mClsAmpDrift->GetYaxis()->SetTitle("Counts");
    getObjectsManager()->startPublishing(mClsAmpDrift.get());

    for (Int_t layer = 0; layer < o2::trd::constants::NLAYER; ++layer) {
      std::string label = fmt::format("NClsLayer_{0}", layer);
      std::string title = fmt::format("Illumination layer {0};Sectors;Padrows (z)", layer);
      mNClsLayer[layer].reset(new TH2F(label.c_str(), title.c_str(), 18 * 8, -0.5, 17.5, 76, -0.5, 75.5));
      getObjectsManager()->startPublishing(mNClsLayer[layer].get());
      getObjectsManager()->setDefaultDrawOptions(label.c_str(), "COLZ");
    }
    mADCvalue.reset(new TH1D("ADCvalue", "ADC value distribution;ADC value;Counts", 500, -0.5, 499.5));
    getObjectsManager()->startPublishing(mADCvalue.get());
    char buffer[256];
    for (Int_t iSM = 0; iSM < o2::trd::constants::NSECTOR; ++iSM) {

      std::string label = fmt::format("HCMCM_{0}", iSM);
      std::string title = fmt::format("MCM in Digits data stream for sector {0}", iSM);
      mHCMCM[iSM].reset(new TH2F(label.c_str(), title.c_str(), 76, -0.5, 75.5, 8 * 6, -0.5, 8 * 6 - 0.5));
      mHCMCM[iSM]->GetXaxis()->SetTitle("PadRow");
      mHCMCM[iSM]->GetYaxis()->SetTitle("PadCol");
      getObjectsManager()->startPublishing(mHCMCM[iSM].get());
      getObjectsManager()->setDefaultDrawOptions(mHCMCM[iSM]->GetName(), "COLZ");
      drawLinesMCM(mHCMCM[iSM].get());

      label = fmt::format("ClsDetAmp_{0}", iSM);
      title = fmt::format("Cluster amplitude chamberwise in SM {0}", iSM);
      mClsDetAmp[iSM].reset(new TH2F(label.c_str(), title.c_str(), 30, -0.5, 29.5, 500, -0.5, 999.5));
      mClsDetAmp[iSM]->GetXaxis()->SetTitle("Chamber");
      mClsDetAmp[iSM]->GetYaxis()->SetTitle("Amplitude");
      getObjectsManager()->startPublishing(mClsDetAmp[iSM].get());
      getObjectsManager()->setDefaultDrawOptions(mClsDetAmp[iSM]->GetName(), "COLZ");

      label = fmt::format("ADCTB_{0}", iSM);
      title = fmt::format("Signal in Timebins for sector {0}", iSM);
      mADCTB[iSM].reset(new TH2F(label.c_str(), title.c_str(), 30, -0.5, 29.5, 128, -0.5, 127.5));
      mADCTB[iSM]->GetXaxis()->SetTitle("Timebin");
      mADCTB[iSM]->GetYaxis()->SetTitle("ADC");
      getObjectsManager()->startPublishing(mADCTB[iSM].get());
      getObjectsManager()->setDefaultDrawOptions(mADCTB[iSM]->GetName(), "COLZ");

      label = fmt::format("ADCTBfull_{0}", iSM);
      title = fmt::format("Signal in Timebins full for sector {0}", iSM);
      mADCTBfull[iSM].reset(new TH2F(label.c_str(), title.c_str(), 30, -0.5, 29.5, 128, -0.5, 127.5));
      mADCTBfull[iSM]->GetXaxis()->SetTitle("Timebin");
      mADCTBfull[iSM]->GetYaxis()->SetTitle("ADC");
      getObjectsManager()->startPublishing(mADCTBfull[iSM].get());
      getObjectsManager()->setDefaultDrawOptions(mADCTBfull[iSM]->GetName(), "COLZ");

      label = fmt::format("ADC_{0}", iSM);
      title = fmt::format("ADC value distribution for sector {0}", iSM);
      mADC[iSM].reset(new TH1F(label.c_str(), title.c_str(), 500, -0.5, 499.5));
      mADC[iSM]->GetXaxis()->SetTitle("ADC value");
      getObjectsManager()->startPublishing(mADC[iSM].get());
      mADC[iSM]->GetYaxis()->SetTitle("Counts");

      // clusters
      label = fmt::format("ClsSM_{0}", iSM);
      title = fmt::format("Cluster for Sector {0}", iSM);
      mClsSM[iSM].reset(new TH1F(label.c_str(), title.c_str(), 100, 0, 200));
      mClsSM[iSM]->GetXaxis()->SetTitle("ADC value");
      mClsSM[iSM]->GetYaxis()->SetTitle("Counts");
      getObjectsManager()->startPublishing(mClsSM[iSM].get());

      label = fmt::format("ClsTbSM_{0}", iSM);
      title = fmt::format("Cluster Timbin  for sector {0}", iSM);
      mClsTbSM[iSM].reset(new TH2F(label.c_str(), title.c_str(), 30, -0.5, 29.5, 10, 0, 200));
      mClsTbSM[iSM]->GetXaxis()->SetTitle("Timebin");
      mClsTbSM[iSM]->GetYaxis()->SetTitle("ADC");
      getObjectsManager()->startPublishing(mClsTbSM[iSM].get());
      getObjectsManager()->setDefaultDrawOptions(mClsTbSM[iSM]->GetName(), "COLZ");

      label = fmt::format("ClsDetTime_{0}", iSM);
      title = fmt::format("Cluster amplitude chamberwise for sector {0}", iSM);
      mClsDetTime[iSM].reset(new TH2F(label.c_str(), title.c_str(), 30, -0.5, 29.5, 30, -0.5, 29.5));
      mClsDetTime[iSM]->GetXaxis()->SetTitle("Chamber");
      mClsDetTime[iSM]->GetYaxis()->SetTitle("Timebin");
      getObjectsManager()->startPublishing(mClsDetTime[iSM].get());
      getObjectsManager()->setDefaultDrawOptions(mClsDetTime[iSM]->GetName(), "COLZ");
    }

    //TH1F*	mNCls;//->Fill(sm);
    mNCls.reset(new TH1F("NCls", "Total number of clusters per sector", 18, -0.5, 17.5));
    mNCls->SetTitle("Total number of clusters per sector;Sector;Counts");
    mNCls->GetXaxis()->SetTitle("Sector");
    mNCls->GetYaxis()->SetTitle("Counts");
    //getObjectsManager()->startPublishing(mNCls.get());

    mClsAmp.reset(new TH1F("NClsAmp", "Total number of clusters per sector", 200, -0.5, 199.5));
    mClsAmp->GetXaxis()->SetTitle("Sector");
    mClsAmp->GetYaxis()->SetTitle("Counts");
    //getObjectsManager()->startPublishing(mClsAmp.get());

    mClsTb.reset(new TH2F("ClsTb", "Cluster  vs. timebin", 30, -0.5, 29.5, 200, 0, 2000));
    mClsTb->GetXaxis()->SetTitle("Timebin");
    mClsTb->GetYaxis()->SetTitle("Cluster");
    //getObjectsManager()->startPublishing(mClsTb.get());

    mClsAmpTb.reset(new TH1F("ClsAmpTb", "Cluster amplitude vs. timebin", 30, -0.5, 29.5));
    mClsAmpTb->GetXaxis()->SetTitle("Timebin");
    mClsAmpTb->GetYaxis()->SetTitle("Amplitude");
    // getObjectsManager()->startPublishing(mClsAmpTb.get());

    mClsSector.reset(new TH2F("ClsSector", "Cluster amplidue per sector", 18, -0.5, 17.5, 500, -0.5, 999.5));
    mClsSector->GetXaxis()->SetTitle("Sector");
    mClsSector->GetYaxis()->SetTitle("Amplitude");
    getObjectsManager()->startPublishing(mClsSector.get());

    mClsStack.reset(new TH2F("ClsStack", "Cluster amplitude per stack", 5, -0.5, 4.5, 500, -0.5, 999.5));
    mClsStack->GetXaxis()->SetTitle("Stack");
    mClsStack->GetYaxis()->SetTitle("Amplitude");
    getObjectsManager()->startPublishing(mClsStack.get());
    mClsStack->SetTitle(";Stack;Amplitude");

    // local histos
    std::string label = fmt::format("clsAmp");
    std::string title = fmt::format("Cluster Amplitude for chamber");
    mClusterAmplitudeChamber.reset(new TH2F(label.c_str(), title.c_str(), 300, -0.5, 299.5, 540, -0.5, 539.5));

    //TODO come back to figure out proper names mClusterAmplitudeChamber[det]->GetXaxis()->SetTitle("PadRow");
    mClusterAmplitudeChamber->GetYaxis()->SetTitle("Chamber");
    getObjectsManager()->startPublishing(mClusterAmplitudeChamber.get());
    for (int trgg = 0; trgg < 10; ++trgg) {
      if (trgg == 3) {
        std::string label = fmt::format("ClsChargeTbTrgg_{0}", trgg);
        std::string title = fmt::format("Total charge vs. time bin", trgg);
        // mClsChargeTbTigg[trgg]->SetDescription("This plot shows the pulse height distribution. It should have a peak between the two indicated lines. If not, you will receive a warning");
        mClsChargeTbTigg[trgg].reset(new TH1F(label.c_str(), title.c_str(), 30, -0.5, 29.5));
        mClsChargeTbTigg[trgg]->GetXaxis()->SetTitle("time bin");
        mClsChargeTbTigg[trgg]->GetYaxis()->SetTitle("total charge");
        getObjectsManager()->startPublishing(mClsChargeTbTigg[trgg].get());
      } else {
        std::string label = fmt::format("ClsChargeTbTrgg_{0}", trgg);
        std::string title = fmt::format("Total charge vs. timebin, trigger class {0}", trgg);
        mClsChargeTbTigg[trgg].reset(new TH1F(label.c_str(), title.c_str(), 30, -0.5, 29.5));
        mClsChargeTbTigg[trgg]->GetXaxis()->SetTitle("time bin");
        mClsChargeTbTigg[trgg]->GetYaxis()->SetTitle("total charge");
        getObjectsManager()->startPublishing(mClsChargeTbTigg[trgg].get());
      }
    }
  }

  void DigitsTask::initialize(o2::framework::InitContext& /*ctx*/ )
  {
    ILOG(Info) << "initialize TRDDigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

    // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
    if (auto param = mCustomParameters.find("peakregionstart"); param != mCustomParameters.end()) {
      mDriftRegion.first = stof(param->second);
      ILOG(Info, Support) << "configure() : using peakregionstart = " << mDriftRegion.first << ENDM;
    } else {
      mDriftRegion.first = 7.0;
      ILOG(Info, Support) << "configure() : using default dritfregionstart = " << mDriftRegion.first << ENDM;
    }
    if (auto param = mCustomParameters.find("peakregionend"); param != mCustomParameters.end()) {
      mDriftRegion.second = stof(param->second);
      ILOG(Info, Support) << "configure() : using peakregionstart = " << mDriftRegion.second << ENDM;
    } else {
      mDriftRegion.second = 20.0;
      ILOG(Info, Support) << "configure() : using default dritfregionstart = " << mDriftRegion.second << ENDM;
    }
    if (auto param = mCustomParameters.find("pulsheightpeaklower"); param != mCustomParameters.end()) {
      mPulseHeightPeakRegion.first = stof(param->second);
      ILOG(Info, Support) << "configure() : using pulsehheightlower	= " << mPulseHeightPeakRegion.first << ENDM;
    } else {
      mPulseHeightPeakRegion.first = 1.0;
      ILOG(Info, Support) << "configure() : using default pulseheightlower = " << mPulseHeightPeakRegion.first << ENDM;
    }
    if (auto param = mCustomParameters.find("pulsheightpeakupper"); param != mCustomParameters.end()) {
      mPulseHeightPeakRegion.second = stof(param->second);
      ILOG(Info, Support) << "configure() : using pulsehheightupper	= " << mPulseHeightPeakRegion.second << ENDM;
    } else {
      mPulseHeightPeakRegion.second = 5.0;
      ILOG(Info, Support) << "configure() : using default pulseheightupper = " << mPulseHeightPeakRegion.second << ENDM;
    }
    buildHistograms();
  }

  void DigitsTask::startOfActivity(Activity& /*activity*/)
  {
    ILOG(Info) << "startOfActivity" << ENDM;

  } //set stats/stacs

  void DigitsTask::startOfCycle()
  {
    ILOG(Info) << "startOfCycle" << ENDM;
  }

  bool digitindexcompare(unsigned int A, unsigned int B, const std::vector<o2::trd::Digit>& originalDigits)
  {
    // sort into ROC:padrow
    const o2::trd::Digit *a, *b;
    a = &originalDigits[A];
    b = &originalDigits[B];
    if (a->getDetector() < b->getDetector()) {
      return 1;
    }
    if (a->getDetector() > b->getDetector()) {
      return 0;
    }
    if (a->getPadRow() < b->getPadRow()) {
      return 1;
    }
    if (a->getPadRow() > b->getPadRow()) {
      return 0;
    }
    if (a->getPadCol() > b->getPadCol()) {
      return 0;
    }
    return 0;
  }

  void DigitsTask::monitorData(o2::framework::ProcessingContext& ctx)
  {
    for (auto&& input : ctx.inputs()) {
      if (input.header != nullptr && input.payload != nullptr) {

        auto digits = ctx.inputs().get<gsl::span<o2::trd::Digit>>("digits");
        std::vector<o2::trd::Digit> digitv(digits.begin(), digits.end());

        if (digitv.size() == 0)
          continue;
        auto tracklets = ctx.inputs().get<gsl::span<o2::trd::Tracklet64>>("tracklets"); // still deciding if we will ever need the tracklets here.
        auto triggerrecords = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");
        // create a vector indexing into the digits in question
        std::vector<unsigned int> digitsIndex(digitv.size());
        std::iota(digitsIndex.begin(), digitsIndex.end(), 0);
        // we now have sorted digits, can loop sequentially and be going over det/row/pad
        for (auto& trigger : triggerrecords) {
          if (trigger.getNumberOfDigits() == 0)
            continue; //bail if we have no digits in this trigger
          //now sort digits to det,row,pad
          std::sort(std::begin(digitsIndex) + trigger.getFirstDigit(), std::begin(digitsIndex) + trigger.getFirstDigit() + trigger.getNumberOfDigits(),
                    [&digitv](unsigned int i, unsigned int j) { return digitindexcompare(i, j, digitv); });

          ///////////////////////////////////////////////////
          // Go through all chambers (using digits)
          const int adcThresh = 13;
          const int clsCutoff = 1000;
          const int startRow[5] = { 0, 16, 32, 44, 60 };
          if (trigger.getNumberOfDigits() > 10000) {
            mDigitsPerEvent->Fill(9999);
          } else {
            mDigitsPerEvent->Fill(trigger.getNumberOfDigits());
          }

          for (int currentdigit = trigger.getFirstDigit() + 1; currentdigit < trigger.getFirstDigit() + trigger.getNumberOfDigits() - 1; ++currentdigit) { // -1 and +1 as we are looking for consecutive digits pre and post the current one indexed.

            int detector = digits[digitsIndex[currentdigit]].getDetector();
            int sm = detector / 30;
            int detLoc = detector % 30;
            int layer = detector % 6;
            int istack = detLoc / 6;
            int iChamber = sm * 30 + istack * o2::trd::constants::NLAYER + layer;
            int nADChigh = 0;
            int stackoffset = istack * o2::trd::constants::NROWC1;
            if (istack >= 2)
              stackoffset -= 4; // account for stack 2 having 4 less.
            //std::cout << "padrow:" << digits[digitsIndex[currentdigit]].getPadRow() << " stackoffset: " << stackoffset << " padcol:" <<digits[digitsIndex[currentdigit]].getPadCol()<< " stack:" << istack << " layer:"<< layer << " chamber:" << iChamber << std::endl;
            mHCMCM[sm]->Fill(digits[digitsIndex[currentdigit]].getPadRow() + stackoffset, digits[digitsIndex[currentdigit]].getPadCol());
            mDigitHCID->Fill(digits[digitsIndex[currentdigit]].getHCId());
            // after updating the 2 above histograms the first and last digits are of no use, as we are looking for 3 neighbouring digits after this.

            int row = 0, col = 0;
            //do we have 3 digits next to each other:
            std::tuple<unsigned int, unsigned int, unsigned int> a, b, c;
            a = std::make_tuple(digits[digitsIndex[currentdigit - 1]].getDetector(), digits[digitsIndex[currentdigit - 1]].getPadRow(), digits[digitsIndex[currentdigit - 1]].getPadCol());
            b = std::make_tuple(digits[digitsIndex[currentdigit]].getDetector(), digits[digitsIndex[currentdigit]].getPadRow(), digits[digitsIndex[currentdigit]].getPadCol());
            c = std::make_tuple(digits[digitsIndex[currentdigit + 1]].getDetector(), digits[digitsIndex[currentdigit + 1]].getPadRow(), digits[digitsIndex[currentdigit + 1]].getPadCol());
            auto [det1, row1, col1] = a;
            auto [det2, row2, col2] = b;
            auto [det3, row3, col3] = c;
            // check we 3 consecutive adc
            bool consecutive = false;
            if (det1 == det2 && det2 == det3 && row1 == row2 && row2 == row3 && col1 + 1 == col2 && col2 + 1 == col3) {
              consecutive = true;
            }
            // illumination
            mNClsLayer[layer]->Fill(sm - 0.5 + col / 144., startRow[istack] + row);
            for (int time = 1; time < o2::trd::constants::TIMEBINS - 1; ++time) {
              int digitindex = digitsIndex[currentdigit];
              int digitindexbelow = digitsIndex[currentdigit - 1];
              int digitindexabove = digitsIndex[currentdigit + 1];
              int value = digits[digitsIndex[currentdigit]].getADC()[time];
              if (value > adcThresh)
                nADChigh++;

              mADCvalue->Fill(value);
              mADC[sm]->Fill(value);
              mADCTB[sm]->Fill(time, value);
              mADCTBfull[sm]->Fill(time, value);

              if (consecutive) {
                // clusterize
                if (value > digits[digitindexbelow].getADC()[time] &&
                    value > digits[digitindexabove].getADC()[time]) {
                  //How do we determine this? HV/LME/other?
                  //if(ChamberHasProblems(sm,istack,layer)) continue;

                  value -= 10;

                  int value = digits[digitindex].getADC()[time];
                  int valueLU = (digits[digitindexbelow].getADC()[time - 1] < 10) ? 0 : digits[digitindexbelow].getADC()[time - 1] - 10;
                  int valueRU = (digits[digitindexabove].getADC()[time - 1] < 10) ? 0 : digits[digitindexabove].getADC()[time - 1] - 10;
                  int valueU = digits[digitindex].getADC()[time - 1] - 10;

                  int valueLD = (digits[digitindexbelow].getADC()[time + 1] < 10) ? 0 : digits[digitindexbelow].getADC()[time + 1] - 10;
                  int valueRD = (digits[digitindexabove].getADC()[time + 1] < 10) ? 0 : digits[digitindexabove].getADC()[time + 1] - 10;
                  int valueD = digits[digitindex].getADC()[time + 1] - 10;

                  int valueL = (digits[digitindexbelow].getADC()[time] < 10) ? 0 : digits[digitindexbelow].getADC()[time] - 10;
                  int valueR = (digits[digitindexabove].getADC()[time] < 10) ? 0 : digits[digitindexabove].getADC()[time] - 10;

                  int sum = value + valueL + valueR;
                  int sumU = valueU + valueLU + valueRU;
                  int sumD = valueD + valueLD + valueRD;

                  if (sumU < 10 || sumD < 10)
                    continue;
                  if (TMath::Abs(1. * sum / sumU - 1) < 0.01)
                    continue;
                  if (TMath::Abs(1. * sum / sumD - 1) < 0.01)
                    continue;
                  if (TMath::Abs(1. * sumU / sumD - 1) < 0.01)
                    continue;

                  mNCls->Fill(sm);
                  mClsSM[sm]->Fill(sum);
                  mClsTbSM[sm]->Fill(time, sum);
                  mClsTb->Fill(time, sum);
                  mClsChargeFirst->Fill(sum, (1. * sum / sumU) - 1.);
                  mClsChargeFirst->Fill(sum, (1. * sum / sumD) - 1.);

                  if (sum > 10 && sum < clsCutoff) {
                    mClsChargeTb->Fill(time, sum);
                    mClsChargeTbCycle->Fill(time, sum);
                    mClsNTb->Fill(time);
                  }

                  mClsAmp->Fill(sum);

                  if (time > mDriftRegion.first && time < mDriftRegion.second) {
                    mClsAmpDrift->Fill(sum);
                    mClsDetAmp[sm]->Fill(detLoc, sum);
                    mClusterAmplitudeChamber->Fill(sum, iChamber);
                  }

                  mClsSector->Fill(sm, sum);
                  mClsStack->Fill(istack, sum);

                  // chamber by chamber drift time
                  if (sum > 10) // && sum < clsCutoff)
                  {
                    mClsDetTime[sm]->Fill(detLoc, time, sum);
                    //mClsDetTimeN[idSM]->Fill(iChamber, k);
                  }

                  // Fill pulse height plot according to demanded trigger
                  /*  TODO implement the different triggers, read the header in the cruhalfchamber this is lost on the epn,
                   * if(IsRun(PHYSICSRUN)){
                   }
                   else if(IsRun(STANDALONE))
                   else if(IsRun(CALIBRATION)) // the one with digits
                   */
                  mClsChargeTbTigg[3]->Fill(time, sum);

                } // if consecutive adc ( 3 next to each other and we are on the middle on
              }   // loop over time-bins
            }     // loop over col-pads
            //	fNumberOfSignalADCsInCycle += nADChigh;
          } // for loop over digits
        }   // loop over triggers

      } //if input is valid
    }   // for loop over inputs
  }

  void DigitsTask::endOfCycle()
  {
    ILOG(Info) << "endOfCycle" << ENDM;
    TProfile* prof = mClusterAmplitudeChamber->ProfileY();
    for (Int_t det = 0; det < 540; det++) {
      mClsAmpCh->Fill(prof->GetBinContent(det));
    }
  }

  void DigitsTask::endOfActivity(Activity& /*activity*/)
  {
    ILOG(Info) << "endOfActivity" << ENDM;
  }

  void DigitsTask::reset()
  {
    // clean all the monitor objects here
    ILOG(Info) << "Resetting the histogram" << ENDM;
    mDigitsPerEvent->Reset();
    mDigitHCID->Reset();
    //TODO reset all the other spectra
  }
  } // namespace o2::quality_control_modules::trd
