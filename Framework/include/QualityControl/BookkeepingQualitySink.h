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
/// \file   BookkeepingQualitySink.h
/// \author Michal Tichak
///

#ifndef QUALITYCONTROL_BOOKKEEPINGQUALITYSINK_H
#define QUALITYCONTROL_BOOKKEEPINGQUALITYSINK_H

#include <Framework/CompletionPolicy.h>
#include <Framework/DataProcessorSpec.h>
#include <Framework/Task.h>
#include "QualityControl/QualitiesToFlagCollectionConverter.h"
#include "QualityControl/Provenance.h"

namespace o2::quality_control::core
{

// This class gathers all QualityObjects from it's inputs, converting them to flags + sending them to grpc RCT/BKP when stop() is invoked
class BookkeepingQualitySink : public framework::Task
{
 public:
  // we are using map here instead of the set, because items in the map are changeable, however items of the set are not.
  using FlagsMap = std::unordered_map<std::string /*detector*/,
                                      std::unordered_map<std::string /* QO name */,
                                                         std::unique_ptr<QualitiesToFlagCollectionConverter>>>;
  using SendCallback = std::function<void(const std::string& grpcUri, const FlagsMap&, Provenance)>;

  // sendCallback is mainly used for testing without the necessity to do grpc calls
  BookkeepingQualitySink(const std::string& grpcUri, Provenance, SendCallback sendCallback = send);

  void init(framework::InitContext&) override;
  void run(framework::ProcessingContext&) override;

  void endOfStream(framework::EndOfStreamContext& context) override;
  void stop() override;

  static void customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies);
  static framework::DataProcessorLabel getLabel() { return { "BookkeepingQualitySink" }; }
  static void send(const std::string& grpcUri, const FlagsMap&, Provenance);

 private:
  std::string mGrpcUri;
  Provenance mProvenance;
  SendCallback mSendCallback;
  FlagsMap mFlagsMap;

  void sendAndClear();
};

} // namespace o2::quality_control::core

#endif
