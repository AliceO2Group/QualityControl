// Copyright CERN and copyright holders of ALICE O2. This software is
// // distributed under the terms of the GNU General Public License v3 (GPL
// // Version 3), copied verbatim in the file "COPYING".
// //
// // See http://alice-o2.web.cern.ch/license for full licensing information.
// //
// // In applying this license CERN does not waive the privileges and immunities
// // granted to it by virtue of its status as an Intergovernmental Organization
// // or submit itself to any jurisdiction.
//

///
/// \file   ITSThresholdCalibrationTask.cxx
/// \author Artem Isakov
///

#include "QualityControl/QcInfoLogger.h"
#include "ITS/ITSThresholdCalibrationTask.h"
#include <DataFormatsITS/TrackITS.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <Framework/InputRecord.h>
#include "ReconstructionDataFormats/Vertex.h"
#include "ReconstructionDataFormats/PrimaryVertex.h"

#include <Framework/DataSpecUtils.h>

using namespace o2::itsmft;
using namespace o2::its;

namespace o2::quality_control_modules::its
{

ITSThresholdCalibrationTask::ITSThresholdCalibrationTask() : TaskInterface()
{
  // createAllHistos();
}

ITSThresholdCalibrationTask::~ITSThresholdCalibrationTask()
{
//  delete hNClusters;
}

void ITSThresholdCalibrationTask::initialize(o2::framework::InitContext& /*ctx*/)
{

  ILOG(Info, Support) << "initialize ITSThresholdCalibrationTask" << ENDM;

  mRunNumberPath = mCustomParameters["runNumberPath"];

  createAllHistos();
  publishHistos();
}

void ITSThresholdCalibrationTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
}

void ITSThresholdCalibrationTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void ITSThresholdCalibrationTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  ILOG(Info, Support) << "START DOING QC General" << ENDM;
  const auto tunString = ctx.inputs().get<gsl::span<char>>("tunestring");
  const auto runType = 	ctx.inputs().get<short int>("runtype");
  const auto scanType = ctx.inputs().get<char>("scantype");


  string inString(tunString.begin(), tunString.end());
  std::cout<<"Scan type is: "<< scanType<<std::endl;
  std::cout<<"Run type is: "<< runType<<std::endl;
  std::cout<<"Output: "<<inString<<std::endl;
/*
  for (auto inString: tunString){
   
     std::cout<<"Output: "<<inString<<std::endl;

  }

*/

/*
  std::string delimiter = "Stave";
  std::string token = inString.substr(0, inString.find(delimiter)); // token is "scott"
  
  std::cout<< "First stave: " << token << " at " << inString.find(delimiter)<<std::endl;
*/
  
  auto splitRes = split(inString,"Stave:");
  for (auto StaveStr: splitRes){
     std::cout<<"Stave !: "<<StaveStr<<std::endl;
     for (auto thr_info: split(StaveStr,",")){
         std::cout<<"info: "<< thr_info <<std::endl;
      }
  }

  std::cout<<" END!!!!!!!!!!!!!" <<std::endl;

}

void ITSThresholdCalibrationTask::endOfCycle()
{

  std::ifstream runNumberFile(mRunNumberPath.c_str());
  if (runNumberFile) {

    std::string runNumber;
    runNumberFile >> runNumber;
    if (runNumber != mRunNumber) {
      for (unsigned int iObj = 0; iObj < mPublishedObjects.size(); iObj++)
        getObjectsManager()->addMetadata(mPublishedObjects.at(iObj)->GetName(), "Run", runNumber);
      mRunNumber = runNumber;
    }
    ILOG(Info, Support) << "endOfCycle" << ENDM;
  }
}

void ITSThresholdCalibrationTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ITSThresholdCalibrationTask::reset()
{
  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
//  hNClusters->Reset();
}

void ITSThresholdCalibrationTask::createAllHistos()
{

/*
  hAngularDistribution = new TH2D("AngularDistribution", "AngularDistribution", 30, -1.5, 1.5, 60, 0, TMath::TwoPi());
  hAngularDistribution->SetTitle("AngularDistribution");
  addObject(hAngularDistribution);
  formatAxes(hAngularDistribution, "#eta", "#phi", 1, 1.10);
  hAngularDistribution->SetStats(0);
*/
}
void ITSThresholdCalibrationTask::addObject(TObject* aObject)
{
  if (!aObject) {
    ILOG(Info, Support) << " ERROR: trying to add non-existent object " << ENDM;
    return;
  } else {
    mPublishedObjects.push_back(aObject);
  }
}

void ITSThresholdCalibrationTask::formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset, float yOffset)
{
  h->GetXaxis()->SetTitle(xTitle);
  h->GetYaxis()->SetTitle(yTitle);
  h->GetXaxis()->SetTitleOffset(xOffset);
  h->GetYaxis()->SetTitleOffset(yOffset);
}

void ITSThresholdCalibrationTask::publishHistos()
{
  for (unsigned int iObj = 0; iObj < mPublishedObjects.size(); iObj++) {
    getObjectsManager()->startPublishing(mPublishedObjects.at(iObj));
    ILOG(Info, Support) << " Object will be published: " << mPublishedObjects.at(iObj)->GetName() << ENDM;
  }
}

std::vector<std::string> ITSThresholdCalibrationTask::split (std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}


} // namespace o2::quality_control_modules::its


