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
/// \file   Helper.h
/// \author Artur Furs afurs@cern.ch
///

// \brief Temporary helper
/// \author Artur Furs afurs@cern.ch

#include <utility>
#include <type_traits>
#ifndef QC_MODULE_FV0_HELPER_H
#define QC_MODULE_FV0_HELPER_H
namespace o2::quality_control_modules::fv0
{
namespace helper
{ //helper for soft fixes in case of DataFormatsFV0's modifications
namespace channel_data
{ //ChannelData section
//ChannelID section

//Current field
template <typename T>
auto getChId(const T& digit) -> std::enable_if_t<std::is_same<decltype(std::declval<T>().pmtNumber), Short_t>::value, const decltype(std::declval<T>().pmtNumber)&>
{
  return digit.pmtNumber;
}
//In case of changes in field type
template <typename T>
auto getChId(const T& digit) -> std::enable_if_t<!std::is_same<decltype(std::declval<T>().pmtNumber), Short_t>::value, const decltype(std::declval<T>().pmtNumber)&>
{
  return digit.pmtNumber;
}
//Target changes
template <typename T>
auto getChId(const T& digit) -> std::enable_if_t<std::is_same<decltype(std::declval<T>().chId), uint8_t>::value, const decltype(std::declval<T>().chId)&>
{
  return digit.chId;
}
//Target changes in case of different target field type
template <typename T>
auto getChId(const T& digit) -> std::enable_if_t<!std::is_same<decltype(std::declval<T>().chId), uint8_t>::value, const decltype(std::declval<T>().chId)&>
{
  return digit.chId;
}
//Time section
//Skipping, it is ok for FV0

//Amplitude section
template <typename T>
auto getCharge(const T& digit) -> std::enable_if_t<std::is_same<decltype(std::declval<T>().chargeAdc), Short_t>::value, const decltype(std::declval<T>().chargeAdc)&>
{
  return digit.chargeAdc;
}
template <typename T>
auto getCharge(const T& digit) -> std::enable_if_t<!std::is_same<decltype(std::declval<T>().chargeAdc), Short_t>::value, const decltype(std::declval<T>().chargeAdc)&>
{
  return digit.chargeAdc;
}
template <typename T>
auto getCharge(const T& digit) -> std::enable_if_t<std::is_same<decltype(std::declval<T>().charge), uint16_t>::value, const decltype(std::declval<T>().charge)&>
{
  return digit.charge;
}
template <typename T>
auto getCharge(const T& digit) -> std::enable_if_t<!std::is_same<decltype(std::declval<T>().charge), uint16_t>::value, const decltype(std::declval<T>().charge)&>
{
  return digit.charge;
}

//Bit section - nothing
} //namespace channel_data
} //namespace helper
} //namespace o2::quality_control_modules::fv0

#endif // QC_MODULE_FV0_HELPER_H