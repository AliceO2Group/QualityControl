// Copyright 2024 CERN and copyright holders of ALICE O2.
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
/// \file    testBookkeepingQualitySink.cxx
/// \author  Michal Tichak
///

#include <DataFormatsQualityControl/QualityControlFlag.h>
#include <DataSampling/DataSampling.h>
#include "QualityControl/BookkeepingQualitySink.h"
#include "QualityControl/InfrastructureGenerator.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::utilities;

void customize(std::vector<CompletionPolicy>& policies)
{
  quality_control::customizeInfrastructure(policies);
}

#include <Framework/runDataProcessing.h>
#include <Framework/ControlService.h>
#include <Configuration/ConfigurationFactory.h>
#include <Configuration/ConfigurationInterface.h>
#include <QualityControl/BookkeepingQualitySink.h>
#include <QualityControl/QualityObject.h>

using namespace o2::configuration;

void compareFatal(const quality_control::QualityControlFlag& got, const quality_control::QualityControlFlag& expected)
{
  if (got != expected) {
    LOG(fatal) << "flags in test do not match. expected:\n"
               << expected << "\nreceived\n"
               << got;
  }
}

WorkflowSpec defineDataProcessing(ConfigContext const&)
{
  using namespace quality_control;

  WorkflowSpec specs;

  DataProcessorSpec writer{
    "writer",
    Inputs{},
    Outputs{ { { "tst-qo" }, "TST", "DATA" } },
    AlgorithmSpec{ [](ProcessingContext& ctx) {
      auto obj = std::make_unique<core::QualityObject>(0, "testCheckNull", "TST");
      obj->getActivity().mValidity = core::ValidityInterval{ 10, 500 };
      obj->addFlag(FlagTypeFactory::Good(), "I am comment");
      ctx.outputs().snapshot(Output{ "TST", "DATA", 0 }, *obj);
      ctx.outputs().make<int>(Output{ "TST", "DATA", 0 }, 1);
      ctx.services().get<ControlService>().endOfStream();
    } }

  };

  specs.push_back(writer);

  DataProcessorSpec reader{
    "bookkeepingSink",
    Inputs{ { { "tst-qo" }, "TST", "DATA" } },
    Outputs{},
    adaptFromTask<quality_control::core::BookkeepingQualitySink>(
      "grpcUri", core::Provenance::SyncQC,
      [](const std::string&, const core::BookkeepingQualitySink::FlagsMap& flagsMap, core::Provenance) {
        if (!flagsMap.contains("TST")) {
          LOG(fatal) << "no flag collections for detector TST";
          return;
        }
        const auto& flagsCollectionsTST = flagsMap.at("TST");
        if (!flagsCollectionsTST.contains("testCheckNull")) {
          LOG(fatal) << "no flag collections for QO testCheckNull";
          return;
        }
        const auto& flagConverter = flagsCollectionsTST.at("testCheckNull");
        if (flagConverter == nullptr) {
          LOG(fatal) << "nullptr flag collection for QO testCheckNull";
          return;
        }
        const auto flagsCollection = flagConverter->getResult();
        if (flagsCollection == nullptr) {
          LOG(fatal) << "nullptr flag collection for QO testCheckNull";
          return;
        }

        for (size_t i = 0; const auto& flag : *flagsCollection) {
          switch (i) {
            case 0:
              compareFatal(flag, QualityControlFlag{ core::gFullValidityInterval.getMin(), 10, FlagTypeFactory::UnknownQuality(), "Did not receive a Quality Object which covers this period", "qc/TST/QO/testCheckNull" });
              break;
            case 1:
              compareFatal(flag, QualityControlFlag{ 10, 500, FlagTypeFactory::Good(), "I am comment", "qc/TST/QO/testCheckNull" });
              break;
            case 2:
              compareFatal(flag, QualityControlFlag{ 500, core::gFullValidityInterval.getMax(), FlagTypeFactory::UnknownQuality(), "Did not receive a Quality Object which covers this period", "qc/TST/QO/testCheckNull" });
              break;
            default:
              LOG(fatal) << "More Flags received than expected";
          }
          ++i;
        }
      })
  };

  specs.push_back(reader);
  return specs;
}
