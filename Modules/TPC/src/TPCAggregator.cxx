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
/// \file   TPCAggregator.cxx
/// \author Marcel Lesch
///

#include "TPC/TPCAggregator.h"
#include "QualityControl/QcInfoLogger.h"

using namespace o2::quality_control::core;
using namespace o2::quality_control;

namespace o2::quality_control_modules::tpc
{

void TPCAggregator::configure()
{
}

std::map<std::string, Quality> TPCAggregator::aggregate(QualityObjectsMapType& qoMap)
{
  if (qoMap.empty()) {
    Quality null = Quality::Null;
    std::string NullReason = "QO map given to the aggregator '" + mName + "' is empty.";
    null.addReason(FlagReasonFactory::UnknownQuality(), NullReason);
    null.addMetadata(Quality::Null.getName(), NullReason);
    return { { mName, null } };
  }

  std::unordered_map<std::string, std::string> AggregatorMetaData;
  AggregatorMetaData[Quality::Null.getName()] = "";
  AggregatorMetaData[Quality::Bad.getName()] = "";
  AggregatorMetaData[Quality::Medium.getName()] = "";
  AggregatorMetaData[Quality::Good.getName()] = "";
  AggregatorMetaData["Comment"] = "";

  // we return the worse quality of all the objects we receive, but we preserve all FlagReasons
  Quality current = Quality::Good;
  for (const auto& [qoName, qo] : qoMap) {
    (void)qoName;
    for (const auto& reason : qo->getReasons()) {
      current.addReason(reason.first, reason.second);
    }

    std::string qoTitle = qo->getName();
    std::string qoMetaData = qo->getQuality().getMetadata(qo->getQuality().getName(), "");
    std::string qoMetaDataComment = qo->getQuality().getMetadata("Comment", "");
    std::string insertTitle = "(" + qoTitle + ") \n";

   ILOG(Error, Support) << qoTitle << ENDM;
   ILOG(Error, Support) << qoMetaData << ENDM;
   ILOG(Error, Support) << qoMetaDataComment << ENDM;

    /* if(qoMetaData != ""){
      std::string delimiter = "\n";
      size_t pos = 0;
      while ((pos = qoMetaData.find("\n", pos)) != std::string::npos) {
        qoMetaData.replace(pos,delimiter.size(),insertTitle);
        pos += insertTitle.size();
      }
    }

    if(qoMetaDataComment != ""){
      std::string delimiter = "\n";
      size_t pos = 0;
      while ((pos = qoMetaDataComment.find("\n", pos)) != std::string::npos) {
        qoMetaDataComment.replace(pos,delimiter.size(),insertTitle);
        pos += insertTitle.size();
      }
    }*/ 

    AggregatorMetaData[qo->getQuality().getName()] += qoMetaData;
    AggregatorMetaData["Comment"] += qoMetaDataComment;

    if (qo->getQuality().isWorseThan(current)) {
      current.set(qo->getQuality());
    }
  }
  ILOG(Info, Devel) << "Aggregated Quality: " << current << ENDM;

  //current.addMetadata(Quality::Bad.getName(), AggregatorMetaData[Quality::Bad.getName()]);
  //current.addMetadata(Quality::Medium.getName(), AggregatorMetaData[Quality::Medium.getName()]);
  //current.addMetadata(Quality::Good.getName(), AggregatorMetaData[Quality::Good.getName()]);
  //current.addMetadata(Quality::Null.getName(), AggregatorMetaData[Quality::Null.getName()]);
  //current.addMetadata("Comment", AggregatorMetaData["Comment"]);

  current.addMetadata(Quality::Bad.getName(), "TestBAD");
  current.addMetadata(Quality::Medium.getName(), "TestMEDIUM");
  current.addMetadata(Quality::Good.getName(), "TestGOOD");
  current.addMetadata(Quality::Null.getName(), "TestNULL");
  current.addMetadata("Comment","TestCOMMENT");

  return { { mName, current } };
}

} // namespace o2::quality_control_modules::tpc
