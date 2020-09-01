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
/// \file    runPostProcessingOCC.cxx
/// \author  Piotr Konopka
///
/// \brief This is an OCC executable to run postprocessing

#include "QualityControl/PostProcessingRunner.h"
#include "QualityControl/QcInfoLogger.h"

#include <OccInstance.h>
#include <RuntimeControlledObject.h>
#include <boost/program_options.hpp>
#include <Common/Timer.h>
#include <boost/property_tree/ptree.hpp>

using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace AliceO2::Common;
namespace bpo = boost::program_options;

const std::string qcConfigurationKey = "qcConfiguration";

class PostProcessingOCCStateMachine : public RuntimeControlledObject
{
 public:
  PostProcessingOCCStateMachine(std::string name, double period = 1.0)
    : RuntimeControlledObject("Post-processing task runner"), mName(name), mPeriod(period)
  {
    mRunner = std::make_unique<PostProcessingRunner>(name);
  }

  // In all of the methods below we handle exceptions, so we can go into error state - OCC won't do that for us.
  int executeConfigure(const boost::property_tree::ptree& properties) final
  {
    if (mRunner == nullptr) {
      return -1;
    }

    bool success = true;
    try {
      auto config = properties.count(qcConfigurationKey) > 0 ? properties.get_child(qcConfigurationKey) : properties;
      mRunner->init(config);
    } catch (const std::exception& ex) {
      ILOG << LogErrorOps << "Exception caught: " << ex.what() << ENDM;
      success = false;
    } catch (...) {
      ILOG << LogErrorOps << "Unknown exception";
      success = false;
    }
    return !success;
  }

  int executeReset() final
  {
    if (mRunner == nullptr) {
      return -1;
    }

    bool success = true;
    try {
      mRunner->reset();
    } catch (const std::exception& ex) {
      ILOG << LogErrorOps << "Exception caught: " << ex.what() << ENDM;
      success = false;
    } catch (...) {
      ILOG << LogErrorOps << "Unknown exception";
      success = false;
    }
    return !success;
  }

  int executeRecover() final
  {
    mRunner = std::make_unique<PostProcessingRunner>(mName);
    return mRunner == nullptr;
  }

  int executeStart() final
  {
    bool success = true;
    try {
      mRunner->start();
    } catch (const std::exception& ex) {
      ILOG << LogErrorSupport << "Exception caught: " << ex.what() << ENDM;
      success = false;
    } catch (...) {
      ILOG << LogErrorSupport << "Unknown exception";
      success = false;
    }

    mRateLimiter.reset(static_cast<int>(mPeriod * 1000000));
    return !success;
  }

  int executeStop() final
  {
    if (mRunner == nullptr) {
      return -1;
    }

    bool success = true;
    try {
      mRunner->stop();
    } catch (const std::exception& ex) {
      ILOG << LogErrorSupport << "Exception caught: " << ex.what() << ENDM;
      success = false;
    } catch (...) {
      ILOG << LogErrorSupport << "Unknown exception";
      success = false;
    }
    return !success;
  }

  int executePause() final
  {
    return mRunner == nullptr;
  }

  int executeResume() final
  {
    mRateLimiter.reset(static_cast<int>(mPeriod * 1000000));
    return mRunner == nullptr;
  }

  int executeExit() final
  {
    ILOG << LogInfoSupport << "executeExit" << ENDM;
    mRunner = nullptr;
    return 0;
  }

  int iterateRunning() final
  {
    if (mRunner == nullptr) {
      return -1;
    }

    bool success = true;
    bool continueRunning = false;
    try {
      continueRunning = mRunner->run();
    } catch (const std::exception& ex) {
      ILOG << LogErrorSupport << "Exception caught: " << ex.what() << ENDM;
      success = false;
    } catch (...) {
      ILOG << LogErrorSupport << "Unknown exception";
      success = false;
    }

    while (mRateLimiter.getRemainingTime() < 0) {
      mRateLimiter.increment();
    }
    if (double sleepForUs = 1000000.0 * mRateLimiter.getRemainingTime(); sleepForUs > 0) {
      usleep(sleepForUs);
    }

    return success ? !continueRunning : -1;
  }

  int iterateCheck() final
  {
    if (mRunner == nullptr) {
      return 0;
    }
    return 0;
  }

 private:
  std::unique_ptr<PostProcessingRunner> mRunner = nullptr;
  std::string mName = "";
  double mPeriod = 1.0;
  Timer mRateLimiter;
};

int main(int argc, const char* argv[])
{
  try {
    bpo::options_description desc{ "Options" };
    desc.add_options()                                                                                     //
      ("help,h", "Help screen")                                                                            //
      ("name", bpo::value<std::string>(), "Name of a post processing task to run")                         //
      ("period", bpo::value<double>()->default_value(1.0), "Cycle period of checking triggers in seconds") //
      ("control-port", bpo::value<int>()->default_value(0), "Control port");

    bpo::variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    if (vm.count("help")) {
      ILOG << LogInfoSupport << desc << ENDM;
      return 0;
    } else if (vm.count("name") == 0) {
      ILOG << LogErrorSupport << "No 'name' parameter provided" << ENDM;
      return 1;
    }

    PostProcessingOCCStateMachine stateMachine(vm["name"].as<std::string>(), vm["period"].as<double>());
    OccInstance occ(&stateMachine, vm["control-port"].as<int>());
    occ.wait();
    return 0;

  } catch (const bpo::error& ex) {
    ILOG << LogErrorSupport << ex.what() << ENDM;
    return 1;
  } catch (const boost::exception& ex) {
    ILOG << LogErrorSupport << "Exception caught: " << boost::current_exception_diagnostic_information(true) << ENDM;
    return 1;
  }

  return 0;
}