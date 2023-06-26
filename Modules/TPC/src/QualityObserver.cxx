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
/// \file     QualityObserver.cxx
/// \author   Marcel Lesch
///

#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/QualityObject.h"
#include <TPC/QualityObserver.h>
#include <boost/property_tree/ptree.hpp>
#include <TLine.h>
#include <TText.h>
#include <TCanvas.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::tpc;

void QualityObserver::configure(const boost::property_tree::ptree& config)
{
  auto& id = getID();
  mObserverName = config.get<std::string>("qc.postprocessing." + id + ".qualityObserverName");
  mViewDetails = config.get<bool>("qc.postprocessing." + id + ".observeDetails", true);
  mQualityDetailChoice = config.get<std::string>("qc.postprocessing." + id + ".qualityDetailChoice", "Null, Good, Medium, Bad");

  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + id + ".qualityObserverConfig")) {
    Config dataConfig;

    dataConfig.groupTitle = dataSourceConfig.second.get<std::string>("groupTitle");
    dataConfig.path = dataSourceConfig.second.get<std::string>("path");

    std::vector<std::string> inputQO;
    std::vector<std::string> inputQOTitle;

    if (const auto& qoSources = dataSourceConfig.second.get_child_optional("inputObjects"); qoSources.has_value()) {
      for (const auto& QOsource : qoSources.value()) {
        inputQO.push_back(QOsource.second.data());
      }
    }
    if (const auto& qoTitlesources = dataSourceConfig.second.get_child_optional("inputObjectTitles"); qoTitlesources.has_value()) {
      for (const auto& QOTitlesource : qoTitlesources.value()) {
        inputQOTitle.push_back(QOTitlesource.second.data());
      }
    }
    if (inputQO.size() != inputQOTitle.size()) {
      ILOG(Error, Devel) << "in config of group" << dataConfig.groupTitle << ": Number of QOs does not match number of qo titles!" << ENDM;
    }
    dataConfig.qo = inputQO;
    dataConfig.qoTitle = inputQOTitle;
    mConfig.push_back(dataConfig);

    inputQO.clear();
    inputQOTitle.clear();
  } // for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + name + ".ratioConfig"))
}

void QualityObserver::initialize(Trigger, framework::ServiceRegistryRef)
{
  for (const auto& config : mConfig) {
    mQualities[config.groupTitle] = std::vector<std::string>();
    mReasons[config.groupTitle] = std::vector<std::string>();
    mComments[config.groupTitle] = std::vector<std::string>();
  }
  mColors[Quality::Bad.getName()] = kRed;
  mColors[Quality::Medium.getName()] = kOrange - 3;
  mColors[Quality::Good.getName()] = kGreen + 2;
  mColors[Quality::Null.getName()] = kViolet - 6;

  mQualityDetails[Quality::Bad.getName()] = false;
  mQualityDetails[Quality::Medium.getName()] = false;
  mQualityDetails[Quality::Good.getName()] = false;
  mQualityDetails[Quality::Null.getName()] = false;
  mQualityDetails["Comment"] = false;

  if (size_t finder = mQualityDetailChoice.find("Bad"); finder != std::string::npos) {
    mQualityDetails[Quality::Bad.getName()] = true;
  }
  if (size_t finder = mQualityDetailChoice.find("Medium"); finder != std::string::npos) {
    mQualityDetails[Quality::Medium.getName()] = true;
  }
  if (size_t finder = mQualityDetailChoice.find("Good"); finder != std::string::npos) {
    mQualityDetails[Quality::Good.getName()] = true;
  }
  if (size_t finder = mQualityDetailChoice.find("Null"); finder != std::string::npos) {
    mQualityDetails[Quality::Null.getName()] = true;
  }
  if (size_t finder = mQualityDetailChoice.find("Comment"); finder != std::string::npos) {
    mQualityDetails["Comment"] = true;
  }
}

void QualityObserver::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();
  getQualities(t, qcdb);
  generatePanel();
}

void QualityObserver::finalize(Trigger t, framework::ServiceRegistryRef)
{
  generatePanel();
}

void QualityObserver::getQualities(const Trigger& t,
                                   repository::DatabaseInterface& qcdb)
{
  for (const auto& config : mConfig) {

    if (mQualities[config.groupTitle].size() > 0) {
      mQualities[config.groupTitle].clear();
      mReasons[config.groupTitle].clear();
      mComments[config.groupTitle].clear();
    }
    for (const auto& qualityobject : config.qo) {
      const auto qo = qcdb.retrieveQO(config.path + "/" + qualityobject, t.timestamp, t.activity);
      if (qo) {
        const auto quality = qo->getQuality();
        mQualities[config.groupTitle].push_back(quality.getName());
        mReasons[config.groupTitle].push_back(quality.getMetadata(quality.getName(), ""));
        mComments[config.groupTitle].push_back(quality.getMetadata("Comment", ""));
      } else {
        mQualities[config.groupTitle].push_back(Quality::Null.getName());
        mReasons[config.groupTitle].push_back("");
        mComments[config.groupTitle].push_back("");
      }
    }
  }
} // void QualityObserver::getQualities(const Trigger& t, repository::DatabaseInterface& qcdb)

void QualityObserver::generatePanel()
{
  // Delete the existing plots before regenerating them.
  if (mCanvas) {
    getObjectsManager()->stopPublishing(mObserverName);
    delete mCanvas;
    mCanvas = nullptr;
  }

  // Draw the ratio on a new canvas.
  TCanvas* c = new TCanvas();
  c->SetName(mObserverName.c_str());
  c->SetTitle(mObserverName.c_str());
  c->cd(1);

  TPaveText* pt = new TPaveText(0.05, 0.05, .95, .95);
  pt->SetLineColor(0);
  pt->SetFillColor(0);
  pt->SetBorderSize(1);

  for (const auto& config : mConfig) {

    pt->AddText(""); // Emtpy line needed. AddLine() places the line to the first entry of the for loop. Check late
    TText* GroupText = pt->AddText(config.groupTitle.data());
    ((TText*)pt->GetListOfLines()->Last())->SetTextAlign(22);
    for (int i = 0; i < config.qoTitle.size(); i++) {
      pt->AddText(Form("%s = #color[%d]{%s}", config.qoTitle.at(i).data(), mColors[mQualities[config.groupTitle].at(i).data()], mQualities[config.groupTitle].at(i).data()));
      // To-Check: SetTextAlign does currently not work in QCG
      ((TText*)pt->GetListOfLines()->Last())->SetTextAlign(12);

      if (mViewDetails && mQualityDetails[mQualities[config.groupTitle].at(i).data()]) {

        generateText(pt, true, mReasons[config.groupTitle].at(i));   // print reasons
        generateText(pt, false, mComments[config.groupTitle].at(i)); // print comments
        std::cout << "5" << std::endl;
        std::cout << "6" << std::endl;

        /* if (mReasons[config.groupTitle].at(i) != "") {
          std::string delimiter = "\n";

          if (mReasons[config.groupTitle].at(i).find(delimiter) != std::string::npos) {
            size_t pos = 0;
            std::string subText;
            while ((pos = mReasons[config.groupTitle].at(i).find(delimiter)) != std::string::npos) {
              subText = mReasons[config.groupTitle].at(i).substr(0, pos);
              pt->AddText(Form("#color[%d]{#rightarrow Reason: %s}", kGray + 2, subText.data()));
              ((TText*)pt->GetListOfLines()->Last())->SetTextAlign(12);
              mReasons[config.groupTitle].at(i).erase(0, pos + delimiter.length());
            }
          } else {
            pt->AddText(Form("#color[%d]{#rightarrow Reason: %s}", kGray + 2, mReasons[config.groupTitle].at(i).data()));
            ((TText*)pt->GetListOfLines()->Last())->SetTextAlign(12);
          }
        }
        if (mComments[config.groupTitle].at(i) != "") {
          pt->AddText(Form("#color[%d]{#rightarrow Comment: %s}", kGray + 2, mComments[config.groupTitle].at(i).data()));
          ((TText*)pt->GetListOfLines()->Last())->SetTextAlign(12);
        }*/
      }
      if (mViewDetails && mQualityDetails["Comment"]) {
        // generateText(pt, false, mComments[config.groupTitle].at(i)); //print comments
      }
    }

    // To-Check: AddLine broken for qcg. Does not simply append line
    pt->AddLine();
    ((TLine*)pt->GetListOfLines()->Last())->SetLineWidth(1);
    ((TLine*)pt->GetListOfLines()->Last())->SetLineStyle(9);
  }

  pt->Draw();
  mCanvas = c;
  getObjectsManager()->startPublishing(c);

} // void QualityObserver::generatePanel()

void QualityObserver::generateText(TPaveText* pt, bool isReason, std::string qoMetaText)
{

  ILOG(Warning, Support) << "Entering generateText" << ENDM;

  std::string infoType = "Reason";
  if (!isReason) {
    infoType = "Comment";
  }
  if (qoMetaText != "") {
    std::string delimiter = "\n";

    if (qoMetaText.find(delimiter) != std::string::npos) {
      size_t pos = 0;
      std::string subText;
      while ((pos = qoMetaText.find(delimiter)) != std::string::npos) { // cut string into the different reasons/comments
        subText = qoMetaText.substr(0, pos);
        qoMetaText.erase(0, pos + delimiter.length());

        breakText(pt, infoType, subText); // break up reason/comment for better visualisation on qcg
        // pt->AddText(Form("#color[%d]{#rightarrow %s: %s}", kGray + 2, infoType.data(), subText.data()));
        //((TText*)pt->GetListOfLines()->Last())->SetTextAlign(12);
      }
    } else {
      breakText(pt, infoType, qoMetaText);
      // pt->AddText(Form("#color[%d]{#rightarrow %s: %s}", kGray + 2, infoType.data(), qoMetaText.data()));
      //((TText*)pt->GetListOfLines()->Last())->SetTextAlign(12);
    }
  } // if (qoMetaText != "")
  ILOG(Warning, Support) << "Exiting generateText" << ENDM;
}

void QualityObserver::breakText(TPaveText* pt, std::string infoType, std::string textUnbroken)
{
  ILOG(Warning, Support) << "   Entering breakText" << ENDM;
  const size_t maxStringLength = 40; // GANESHA this has to be configurable
  std::string subLine = "";
  std::string subLineDelimiter = " ";
  size_t subpos = 0;

  if (textUnbroken.length() < maxStringLength) {
    ILOG(Warning, Support) << "   Here 0" << ENDM;
    pt->AddText(Form("#color[%d]{#rightarrow %s: %s}", kGray + 2, infoType.data(), textUnbroken.data()));
    ((TText*)pt->GetListOfLines()->Last())->SetTextAlign(12);
    ILOG(Warning, Support) << "   Leaving breakText" << ENDM;
    return;
  }
  ILOG(Warning, Support) << "   Here 1" << ENDM;
  if (textUnbroken.find(subLineDelimiter) != std::string::npos) {
    bool firstOccurance = true;
    int colorInfoType = kGray + 2;

    while ((subpos = textUnbroken.find(subLineDelimiter)) != std::string::npos) {
      std::string textFragmant = textUnbroken.substr(0, subpos);

      if (subLine == "") {
        subLine += textFragmant + " ";
        textUnbroken.erase(0, subpos + subLineDelimiter.length());
        if (subLine.length() > maxStringLength) {
          pt->AddText(Form("#color[%d]{#rightarrow %s:} #color[%d]{%s}", colorInfoType, infoType.data(), kGray + 2, subLine.data()));
          ((TText*)pt->GetListOfLines()->Last())->SetTextAlign(12);

          if (firstOccurance) {
            colorInfoType = kWhite;
            firstOccurance = false;
          }
          subLine = "";
        }
      } else {
        if (subLine.length() + textFragmant.length() + 1 <= maxStringLength) {
          subLine += textFragmant + " ";
          textUnbroken.erase(0, subpos + subLineDelimiter.length());
        } else {
          pt->AddText(Form("#color[%d]{#rightarrow %s:} #color[%d]{%s}", colorInfoType, infoType.data(), kGray + 2, subLine.data()));
          ((TText*)pt->GetListOfLines()->Last())->SetTextAlign(12);

          if (firstOccurance) {
            colorInfoType = kWhite;
            firstOccurance = false;
          }
          subLine = "";
        }
      }
    } // while ((subpos = textUnbroken.find(subLineDelimiter)) != std::string::npos)

    if (subLine.length() > 0) {
      pt->AddText(Form("#color[%d]{#rightarrow %s:} #color[%d]{%s}", colorInfoType, infoType.data(), kGray + 2, subLine.data()));
      ((TText*)pt->GetListOfLines()->Last())->SetTextAlign(12);
      if (firstOccurance) {
        colorInfoType = kWhite;
        firstOccurance = false;
      }
    }
    if (textUnbroken.length() > 0) {
      pt->AddText(Form("#color[%d]{#rightarrow %s:} #color[%d]{%s}", colorInfoType, infoType.data(), kGray + 2, textUnbroken.data()));
      ((TText*)pt->GetListOfLines()->Last())->SetTextAlign(12);
    }

  } else { // string longer than maxStringLength but does not contain subLineDelimiter
    ILOG(Warning, Support) << "   Here 4" << ENDM;
    pt->AddText(Form("#color[%d]{#rightarrow %s: %s}", kGray + 2, infoType.data(), textUnbroken.data()));
    ((TText*)pt->GetListOfLines()->Last())->SetTextAlign(12);
    ILOG(Warning, Support) << "   Leaving breakText" << ENDM;
    return;
  }
  ILOG(Warning, Support) << "   Leaving breakText" << ENDM;
}