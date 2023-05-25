// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   PulsePositionCheck.cxx
/// \author Deependra (deependra.sharma@cern.ch)
///

#include "TRD/PulsePositionCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TF1.h>
#include <TMath.h>

#include <fairlogger/Logger.h> 
#include <DataFormatsQualityControl/FlagReasons.h>
#include "CCDB/BasicCCDBManager.h"

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::trd
{

void PulsePositionCheck::configure() 
{
  if (auto param = mCustomParameters.find("ccdbtimestamp"); param != mCustomParameters.end()) {
    mTimeStamp = std::stol(mCustomParameters["ccdbtimestamp"]);
    ILOG(Debug, Support) << "configure() : using ccdbtimestamp = " << mTimeStamp << ENDM;
  } else {
    mTimeStamp = o2::ccdb::getCurrentTimestamp();
    ILOG(Debug, Support) << "configure() : using default timestam of now = " << mTimeStamp << ENDM;
  }
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setTimestamp(mTimeStamp);
  ILOG(Debug, Devel) << "initialize PulseHeight" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  if (auto param = mCustomParameters.find("pulseheightpeaklower"); param != mCustomParameters.end()) {
    mPulseHeightPeakRegion.first = stof(param->second);
    ILOG(Debug, Support) << "configure() : using pulseheightpeaklower " << mPulseHeightPeakRegion.first << ENDM;
  } else {
    mPulseHeightPeakRegion.first = 1.0;
    ILOG(Debug, Support) << "configure() : using default pulseheightpeaklower  = " << mPulseHeightPeakRegion.first << ENDM;
  }
  if (auto param = mCustomParameters.find("pulseheightpeakupper"); param != mCustomParameters.end()) {
    mPulseHeightPeakRegion.second = stof(param->second);
    ILOG(Debug, Support) << "configure() : using pulseheightpeakupper = " << mPulseHeightPeakRegion.second << ENDM;
  } else {
    mPulseHeightPeakRegion.second = 5.0;
    ILOG(Debug, Support) << "configure() : using default pulseheightpeakupper = " << mPulseHeightPeakRegion.second << ENDM;
  }
}

Quality PulsePositionCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  // you can get details about the activity via the object mActivity:
  ILOG(Debug, Trace) << "Run type: " << getActivity()->mType << ENDM;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "mPulseHeight") {

      auto* h = dynamic_cast<TH1F*>(mo->getObject());
	    
      //check max bin is in the spike on left.
      auto maxbin = h->GetMaximumBin();

      if (maxbin < mPulseHeightPeakRegion.first || maxbin > mPulseHeightPeakRegion.second) {
        // is the peak in the peak region.
        result = Quality::Bad;
        result.addReason(FlagReasonFactory::Invalid(),
                         "Amplification Peak is in the wrong position " + std::to_string(maxbin));
        return result;
      }

      // Defining Fit function
	    TF1* f1=new TF1("landaufit","((x<2) ? ROOT::Math::erf(x)*[0]:[0]) + [1]*TMath::Landau(x,[2],[3])",0.0,6.0);
	    f1->SetParameters(100000,100000,1.48,1.09);

      // Fitting Pulse Distribution with defined fit function
      h->Fit(f1,"","",0.0,4.0);

      double_t peak_value_x = f1->GetMaximumX(0.0,4.0);

      auto x0 = h->GetXaxis()->GetBinLowEdge(0);
      auto x1 = h->GetXaxis()->GetBinLowEdge(1);
      auto x2 = h->GetXaxis()->GetBinLowEdge(2);
      auto x3 = h->GetXaxis()->GetBinLowEdge(3);

      if((peak_value_x < x1)&&( peak_value_x > x0)){
        result=Quality::Bad;
        result.addReason(FlagReasonFactory::Unknown(),"pulse peak is in first bin");
        // LOG(info)<<"peak is in the first bin from left";
        return result;
      }else if((peak_value_x < x2)&&( peak_value_x > x1)){
        result=Quality::Good;
        // LOG(info)<<"peak is in the second bin from left";
      }else if((peak_value_x < x3)&&( peak_value_x > x2)){
        result=Quality::Good;
        // LOG(info)<<"peak is in the third bin from left";
      }else {
        // LOG(info)<<"peak is not in Good region";
        result=Quality::Bad;
        result.addReason(FlagReasonFactory::Unknown(),"amplification peak is not in good position. Out of ["+std::to_string(x0) + " " + std::to_string(x3)+ "] range"); 
        return result;
      }

      result.addMetadata("Peak_in", "third_Bin");
    }
  }
  return result;
}

std::string PulsePositionCheck::getAcceptedType() { return "TH1"; }

void PulsePositionCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{

  if (mo->getName() == "mPulseHeight") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red" << ENDM;
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange" << ENDM;
      h->SetFillColor(kOrange);
    } else if(checkResult ==Quality::Null){
      ILOG(Debug, Devel) << "Quality::Null, setting to Blue" << ENDM;
      h->SetFillColor(kBlue);
    } else if (checkResult == Quality::NullLevel){
      ILOG(Debug, Devel) << "Quality::Null, setting to Pink" << ENDM;
      h->SetFillColor(kPink);
    }
    h->SetLineColor(kBlack);
    h->Draw();
  }
}

} // namespace o2::quality_control_modules::trd
