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
/// \file   testRepoPathUtils.cxx
/// \author Barthelemy von Haller
///

#include "QualityControl/RepoPathUtils.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QualityObject.h"
#include <Common/Exceptions.h>
#include <TH1F.h>
#include <catch_amalgamated.hpp>

using namespace std;
using namespace o2::quality_control::core;

TEST_CASE("RepoPathUtils_qopath")
{
  // no policy
  QualityObject qo(Quality::Null, "xyzCheck", "DET");
  string path = RepoPathUtils::getQoPath(&qo);
  CHECK(path == "qc/DET/QO/xyzCheck");
  path = RepoPathUtils::getQoPath("DET", "xyzCheck");
  CHECK(path == "qc/DET/QO/xyzCheck");

  // provenance
  qo.getActivity().mProvenance = "qc_mc";
  path = RepoPathUtils::getQoPath(&qo);
  CHECK(path == "qc_mc/DET/QO/xyzCheck");

  // a policy which is not OnEachSeparately
  QualityObject qo2(Quality::Null, "xyzCheck", "DET", "OnAnyNonZero");
  string path2 = RepoPathUtils::getQoPath(&qo2);
  CHECK(path2 == "qc/DET/QO/xyzCheck");
  path2 = RepoPathUtils::getQoPath("DET", "xyzCheck", "OnAnyNonZero");
  CHECK(path2 == "qc/DET/QO/xyzCheck");

  // policy is OnEachSeparately
  QualityObject qo3(Quality::Null, "xyzCheck", "DET", "OnEachSeparately", {}, { "objectABC" });
  string path3 = RepoPathUtils::getQoPath(&qo3);
  CHECK(path3 == "qc/DET/QO/xyzCheck/objectABC");
  path2 = RepoPathUtils::getQoPath("DET", "xyzCheck", "OnEachSeparately", { "objectABC" });
  CHECK(path3 == "qc/DET/QO/xyzCheck/objectABC");

  // policy is OnEachSeparately and the vector is empty
  QualityObject qo4(Quality::Null, "xyzCheck", "DET", "OnEachSeparately", {}, {});
  CHECK_THROWS_AS(RepoPathUtils::getQoPath(&qo4), AliceO2::Common::FatalException);
  CHECK_THROWS_AS(RepoPathUtils::getQoPath("DET", "xyzCheck", "OnEachSeparately", {}), AliceO2::Common::FatalException);
}

TEST_CASE("RepoPathUtils_mopath")
{
  string objectName = "asdf";
  TH1F h(objectName.data(), objectName.data(), 100, 0, 99);
  o2::quality_control::core::MonitorObject obj(&h, "task", "class", "DET");
  obj.setIsOwner(false);
  string path = RepoPathUtils::getMoPath(&obj);
  CHECK(path == "qc/DET/MO/task/asdf");
  path = RepoPathUtils::getMoPath("DET", "task", "asdf");
  CHECK(path == "qc/DET/MO/task/asdf");

  // provenance
  obj.getActivity().mProvenance = "qc_mc";
  path = RepoPathUtils::getMoPath(&obj);
  CHECK(path == "qc_mc/DET/MO/task/asdf");
}
