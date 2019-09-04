// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef QUALITYCONTROL_POSTPROCESSINGRUNNER_H
#define QUALITYCONTROL_POSTPROCESSINGRUNNER_H

// include from OCC
//class RuntimeControlledObject;

namespace o2::quality_control::postprocessing {

class PostProcessingRunner {
  public:
  PostProcessingRunner() = default;
  ~PostProcessingRunner() = default;

  void init();
  // one iteration over the event loop
  void run();
  // other state transitions
  void stop();
  void reset();
  private:

};

// namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_POSTPROCESSINGRUNNER_H
