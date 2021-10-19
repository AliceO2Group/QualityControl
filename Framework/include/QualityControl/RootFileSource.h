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

namespace o2::quality_control::core
{

/// \brief A Data Processor which reads MonitorObjectCollections from a specified file
class RootFileSource : public framework::Task
{
 public:
  RootFileSource(std::string filePath);
  ~RootFileSource() override = default;

  void init(framework::InitContext& ictx) override;
  void run(framework::ProcessingContext& pctx) override;

 private:
  std::string mFilePath;
};

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_ROOTFILESOURCE_H
