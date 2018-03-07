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

/// \brief Dump the publications received from the InformationService.
/// Useful for checking the InformationService.
class InformationServiceDump : public FairMQDevice
{
  public:
    InformationServiceDump();
    virtual ~InformationServiceDump();

  protected:
    bool HandleData(FairMQMessagePtr&, int);
};

#endif //PROJECT_InformationServiceDump_H
