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
/// \file   PostProcessingFactory.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_POSTPROCESSINGFACTORY_H
#define QUALITYCONTROL_POSTPROCESSINGFACTORY_H

namespace o2::quality_control::postprocessing
{

class PostProcessingInterface;
class PostProcessingConfig;

/// \brief Factory in charge of creating post-processing tasks
///
/// The factory needs a library name and a class name provided as an object of type PostProcessingConfig.
/// The class loaded in the library must inherit from PostProcesingInterface
class PostProcessingFactory
{
 public:
  PostProcessingFactory() = default;
  virtual ~PostProcessingFactory() = default;

  /// \brief Create a new instance of a PostProcessingInterface.
  /// The PostProcessingInterface actual class is decided based on the parameters passed.
  PostProcessingInterface* create(const PostProcessingConfig& config);
};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_POSTPROCESSINGFACTORY_H
