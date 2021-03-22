// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   SkeletonCheck.cxx
/// \author My Name
///

#include "Skeleton/SkeletonAggregator.h"
#include "QualityControl/QcInfoLogger.h"

#include <iostream>

using namespace std;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::skeleton
{

void SkeletonAggregator::configure(std::string) {}

std::map<std::string, Quality> SkeletonAggregator::aggregate(QualityObjectsMapType& qoMap)
{
  std::map<std::string, Quality> result;

  std::cout << "asdf" << std::endl;
  ILOG(Info, Support) << "Info" << ENDM;    // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Debug, Support) << "Debug" << ENDM;  // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Error, Support) << "Error" << ENDM;  // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Info, Ops) << "Ops" << ENDM;         // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Info, Support) << "Support" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Info, Devel) << "Devel" << ENDM;     // QcInfoLogger is used. FairMQ logs will go to there as well.
  ILOG(Info, Trace) << "Trace" << ENDM;     // QcInfoLogger is used. FairMQ logs will go to there as well.


  ILOG(Info, Devel) << "Entered SkeletonAggregator::aggregate" << ENDM;
  ILOG(Info, Devel) << "   received a list of size : " << qoMap.size() << ENDM;
  for (const auto& item : qoMap) {
    ILOG(Info, Devel) << "Object: " << (*item.second) << ENDM;
  }

  // we return the worse quality of all the objects we receive
  Quality current = Quality::Good;
  for (auto qo : qoMap) {
    if (qo.second->getQuality().isWorseThan(current)) {
      current = qo.second->getQuality();
    }
  }

  ILOG(Info, Devel) << "   result: " << current << ENDM;
  result["newQuality"] = current;

  // add one more
  Quality plus = Quality::Medium;
  result["another"] = plus;

  return result;
}
} // namespace o2::quality_control_modules::skeleton
