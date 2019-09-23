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
/// \file   PostProcessingFactory.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_POSTPROCESSINGFACTORY_H
#define QUALITYCONTROL_POSTPROCESSINGFACTORY_H

// O2
#include <Common/Exceptions.h>

#include "QualityControl/PostProcessingConfig.h"

namespace o2::quality_control::postprocessing
{

class PostProcessingInterface;

/// \brief Factory in charge of creating post-processing tasks
///
/// The factory needs a library name and a class name provided as an object of type PostProcessingConfig.
/// The class loaded in the library must inherit from PostProcesingInterface
class PostProcessingFactory
{
  public:
  PostProcessingFactory() = default;
  virtual ~PostProcessingFactory() = default;

  using FatalException = AliceO2::Common::FatalException;
  using errinfo_details = AliceO2::Common::errinfo_details;

  /// \brief Create a new instance of a PostProcessingInterface.
  /// The PostProcessingInterface actual class is decided based on the parameters passed.
  PostProcessingInterface* create(const PostProcessingConfig& config);
};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_POSTPROCESSINGFACTORY_H
