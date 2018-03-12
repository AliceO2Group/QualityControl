// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
//

///
/// \author bvonhall
/// \file InformationServiceDump.h
///


#ifndef PROJECT_INFORMATIONSERVICEDUMP_H
#define PROJECT_INFORMATIONSERVICEDUMP_H

#include "FairMQDevice.h"
#include <boost/asio.hpp>

/// \brief Dump the publications received from the InformationService.
///
/// Useful for checking the InformationService.
/// It will receive the updates from the tasks. Upon reception , it dumps it and send a request for all or a single
/// task data and displays the reply.
/// To decide which task the request should target, use parameter "request-task". By default it asks for all.
///
/// See runInformationServiceDump.cxx for the steering code.
///
///
/// Example usage :
///       qcInfoServiceDump -c /absolute/path/to/InformationService.json -n information_service_dump
///                         --id information_service_dump --mq-config /absolute/path/to/InformationService.json
///                         --request-task myTask1
class InformationServiceDump : public FairMQDevice
{
  public:
    InformationServiceDump(boost::asio::io_service& io);
    virtual ~InformationServiceDump();

  protected:
    bool HandleData(FairMQMessagePtr&, int);

  private:
    boost::asio::deadline_timer mTimer; /// the asynchronous timer to call print at regular intervals

};

#endif //PROJECT_InformationServiceDump_H
