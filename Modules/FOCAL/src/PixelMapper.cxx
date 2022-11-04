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

#include <iostream>
#include <FOCAL/PixelMapper.h>

using namespace o2::quality_control_modules::focal;

PixelMapping::PixelMapping(unsigned int version) : mVersion(version) {}

PixelMapping::ChipPosition PixelMapping::getPosition(unsigned int laneID, unsigned int chipID) const
{
  ChipIdentifier identifier{ mUseLanes ? laneID : 0, chipID };
  auto found = mMapping.find(identifier);
  if (found == mMapping.end()) {
    throw InvalidChipException(mVersion, identifier);
  }
  return found->second;
}

void PixelMapping::InvalidChipException::print(std::ostream& stream) const
{
  stream << mMessage;
}

void PixelMapping::VersionException::print(std::ostream& stream) const
{
  stream << mMessage;
}

PixelMappingOB::PixelMappingOB(unsigned int version) : PixelMapping(version) { init(version); }

void PixelMappingOB::init(unsigned int version)
{
  if (version >= 2) {
    throw VersionException(version);
  }
  std::cout << "Initializing OB mapping (" << version << ")" << std::endl;
  switch (version) {
    case 0:
      buildVersion0();
      break;

    case 1:
      buildVersion1();
      break;

    default:
      break;
  }
  mUseLanes = true;
}

void PixelMappingOB::buildVersion0()
{
  mMapping.clear();
  mMapping.insert({ { 7, 6 }, { 0, 5 } });
  mMapping.insert({ { 7, 5 }, { 1, 5 } });
  mMapping.insert({ { 7, 4 }, { 2, 5 } });
  mMapping.insert({ { 7, 3 }, { 3, 5 } });
  mMapping.insert({ { 7, 2 }, { 4, 5 } });
  mMapping.insert({ { 7, 1 }, { 5, 5 } });
  mMapping.insert({ { 7, 0 }, { 6, 5 } });
  mMapping.insert({ { 6, 8 }, { 0, 4 } });
  mMapping.insert({ { 6, 9 }, { 1, 4 } });
  mMapping.insert({ { 6, 10 }, { 2, 4 } });
  mMapping.insert({ { 6, 11 }, { 3, 4 } });
  mMapping.insert({ { 6, 12 }, { 4, 4 } });
  mMapping.insert({ { 6, 13 }, { 5, 4 } });
  mMapping.insert({ { 7, 14 }, { 6, 4 } });
  mMapping.insert({ { 21, 6 }, { 0, 1 } });
  mMapping.insert({ { 21, 5 }, { 1, 1 } });
  mMapping.insert({ { 21, 4 }, { 2, 1 } });
  mMapping.insert({ { 21, 3 }, { 3, 1 } });
  mMapping.insert({ { 21, 2 }, { 4, 1 } });
  mMapping.insert({ { 21, 1 }, { 5, 1 } });
  mMapping.insert({ { 21, 0 }, { 6, 1 } });
  mMapping.insert({ { 20, 8 }, { 0, 0 } });
  mMapping.insert({ { 20, 9 }, { 1, 0 } });
  mMapping.insert({ { 20, 10 }, { 2, 0 } });
  mMapping.insert({ { 20, 11 }, { 3, 0 } });
  mMapping.insert({ { 20, 12 }, { 4, 0 } });
  mMapping.insert({ { 20, 13 }, { 5, 0 } });
  mMapping.insert({ { 20, 14 }, { 6, 0 } });
}

void PixelMappingOB::buildVersion1()
{
  mMapping.clear();
  mMapping.insert({ { 6, 8 }, { 0, 3 } });
  mMapping.insert({ { 6, 9 }, { 1, 3 } });
  mMapping.insert({ { 6, 10 }, { 2, 3 } });
  mMapping.insert({ { 6, 11 }, { 3, 3 } });
  mMapping.insert({ { 6, 12 }, { 4, 3 } });
  mMapping.insert({ { 6, 13 }, { 5, 3 } });
  mMapping.insert({ { 6, 14 }, { 6, 3 } });
  mMapping.insert({ { 7, 6 }, { 0, 2 } });
  mMapping.insert({ { 7, 5 }, { 1, 2 } });
  mMapping.insert({ { 7, 4 }, { 2, 2 } });
  mMapping.insert({ { 7, 3 }, { 3, 2 } });
  mMapping.insert({ { 7, 2 }, { 4, 2 } });
  mMapping.insert({ { 7, 1 }, { 5, 2 } });
  mMapping.insert({ { 7, 0 }, { 6, 2 } });
}

PixelMappingIB::PixelMappingIB(unsigned int version) : PixelMapping(version) { init(version); }

void PixelMappingIB::init(unsigned int version)
{
  if (version >= 2) {
    throw VersionException(version);
  }
  std::cout << "Initializing IB mapping (" << version << ")" << std::endl;
  switch (version) {
    case 0:
      buildVersion0();
      break;

    case 1:
      buildVersion1();
      break;

    default:
      break;
  }
  mUseLanes = false;
}

void PixelMappingIB::buildVersion0()
{
  mMapping.clear();
  mMapping.insert({ { 0, 0 }, { 0, 4 } });
  mMapping.insert({ { 0, 1 }, { 1, 4 } });
  mMapping.insert({ { 0, 2 }, { 2, 4 } });
  mMapping.insert({ { 0, 3 }, { 0, 2 } });
  mMapping.insert({ { 0, 4 }, { 1, 2 } });
  mMapping.insert({ { 0, 5 }, { 2, 2 } });
  mMapping.insert({ { 0, 6 }, { 0, 0 } });
  mMapping.insert({ { 0, 7 }, { 1, 0 } });
  mMapping.insert({ { 0, 8 }, { 2, 0 } });
}

void PixelMappingIB::buildVersion1()
{
  mMapping.clear();
  mMapping.insert({ { 0, 0 }, { 0, 5 } });
  mMapping.insert({ { 0, 1 }, { 1, 5 } });
  mMapping.insert({ { 0, 2 }, { 2, 5 } });
  mMapping.insert({ { 0, 3 }, { 0, 3 } });
  mMapping.insert({ { 0, 4 }, { 1, 3 } });
  mMapping.insert({ { 0, 5 }, { 2, 3 } });
  mMapping.insert({ { 0, 6 }, { 0, 1 } });
  mMapping.insert({ { 0, 7 }, { 1, 1 } });
  mMapping.insert({ { 0, 8 }, { 2, 1 } });
}

PixelMapper::PixelMapper(PixelMapper::MappingType_t mappingtype)
{
  switch (mappingtype) {
    case MappingType_t::MAPPING_IB:
      for (int iversion = 0; iversion < 2; iversion++) {
        mMappings[iversion] = std::make_shared<PixelMappingIB>(iversion);
      }
      break;
    case MappingType_t::MAPPING_OB:
      for (int iversion = 0; iversion < 2; iversion++) {
        mMappings[iversion] = std::make_shared<PixelMappingOB>(iversion);
      }
  };
}

const PixelMapping& PixelMapper::getMapping(unsigned int feeID) const
{
  return *(mMappings[feeID % 2]);
}

std::ostream& o2::quality_control_modules::focal::operator<<(std::ostream& stream, const PixelMapping::InvalidChipException& error)
{
  error.print(stream);
  return stream;
}

std::ostream& o2::quality_control_modules::focal::operator<<(std::ostream& stream, const PixelMapping::VersionException& error)
{
  error.print(stream);
  return stream;
}
