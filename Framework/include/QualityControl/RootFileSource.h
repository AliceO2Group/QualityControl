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
/// \file   RootFileSource.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_ROOTFILESOURCE_H
#define QUALITYCONTROL_ROOTFILESOURCE_H

#include <Framework/Task.h>
#include <string>
#include <vector>
#include <memory>

namespace o2::quality_control::core
{

class RootFileStorage;
class IntegralMocWalker;
class MovingWindowMocWalker;

/// \brief A Data Processor which reads MonitorObjectCollections from a specified file
class RootFileSource : public framework::Task
{
 public:
  RootFileSource(std::string filePath);
  ~RootFileSource() override = default;

  void init(framework::InitContext& ictx) override;
  void run(framework::ProcessingContext& pctx) override;

  static framework::OutputLabel outputBinding(const std::string& detectorCode, const std::string& taskName, bool movingWindow = false);

 private:
  std::string mFilePath;
  std::vector<framework::OutputLabel> mAllowedOutputs;

  std::shared_ptr<RootFileStorage> mRootFileManager = nullptr;
  std::shared_ptr<IntegralMocWalker> mIntegralMocWalker = nullptr;
  std::shared_ptr<MovingWindowMocWalker> mMovingWindowMocWalker = nullptr;
};

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_ROOTFILESOURCE_H
