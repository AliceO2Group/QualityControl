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
/// \file   RootFileSink.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_ROOTFILESINK_H
#define QUALITYCONTROL_ROOTFILESINK_H

#include <Framework/Task.h>
#include <Framework/CompletionPolicy.h>
#include <Framework/DataProcessorLabel.h>

namespace o2::quality_control::core
{

/// \brief A Data Processor which stores MonitorObjectCollections in a specified file
class RootFileSink : public framework::Task
{
 public:
  explicit RootFileSink(std::string filePath);
  ~RootFileSink() override;

  void init(framework::InitContext& ictx) override;
  void run(framework::ProcessingContext& pctx) override;

  static framework::DataProcessorLabel getLabel()
  {
    return { "QC-ROOT-FILE-SINK" };
  }

  static void customizeInfrastructure(std::vector<framework::CompletionPolicy>& policies);

 private:
  void reset();

 private:
  std::string mFilePath;
};

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_ROOTFILESINK_H