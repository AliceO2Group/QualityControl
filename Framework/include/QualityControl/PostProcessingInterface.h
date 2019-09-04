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
/// \file   PostProcessingInterface.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_POSTPROCESSINTERFACE_H
#define QUALITYCONTROL_POSTPROCESSINTERFACE_H


#include <CCDB/CcdbApi.h>
#include "QualityControl/Triggers.h"

namespace o2::quality_control::postprocessing {

class PostProcessingInterface
{
  public:
  PostProcessingInterface() = default;
  virtual ~PostProcessingInterface() = default;

  // user gets to know what triggered the init
  virtual void init(Trigger) = 0;
  // user gets to know what triggered the processing
  virtual void postProcess(Trigger) = 0;
  // user gets to know what triggered the end
  virtual void finalize(Trigger) = 0;
  // store your stuff
  virtual void store() = 0;
  // reset your stuff. maybe a trigger needed?
  virtual void reset() = 0;

  // todo: ccdb api which does not allow to delete?

  protected:

  private:

};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_POSTPROCESSINTERFACE_H
