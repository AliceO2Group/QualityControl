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
#ifndef QC_MODULE_FDD_HELPER_H
#define QC_MODULE_FDD_HELPER_H
namespace o2::quality_control_modules::fdd
{
namespace helper
{ //helper for soft fixes in case of DataFormatsFDD's modifications
namespace digit
{
template <typename T>
auto getTriggerBits(const T& digit) -> std::enable_if_t<std::is_same<decltype(std::declval<T>().mTriggers.triggersignals), uint8_t>::value, const decltype(std::declval<T>().mTriggers.triggersignals)&>
{
  return digit.mTriggers.triggersignals;
}
template <typename T>
auto getTriggerBits(const T& digit) -> std::enable_if_t<!std::is_same<decltype(std::declval<T>().mTriggers.triggersignals), uint8_t>::value, const decltype(std::declval<T>().mTriggers.triggersignals)&>
{
  return digit.mTriggers.triggersignals;
}
template <typename T>
auto getTriggerBits(const T& digit) -> std::enable_if_t<std::is_same<decltype(std::declval<T>().mTriggers.triggerSignals), uint8_t>::value, const decltype(std::declval<T>().mTriggers.triggerSignals)&>
{
  return digit.mTriggers.triggerSignals;
}
template <typename T>
auto getTriggerBits(const T& digit) -> std::enable_if_t<!std::is_same<decltype(std::declval<T>().mTriggers.triggerSignals), uint8_t>::value, const decltype(std::declval<T>().mTriggers.triggerSignals)&>
{
  return digit.mTriggers.triggerSignals;
}
} //namespace digit
namespace channel_data
{ //ChannelData section
//ChannelID section

//Current field
template <typename T>
auto getChId(const T& digit) -> std::enable_if_t<std::is_same<decltype(std::declval<T>().mPMNumber), uint8_t>::value, const decltype(std::declval<T>().mPMNumber)&>
{
  return digit.mPMNumber;
}
//In case of changes in field type
template <typename T>
auto getChId(const T& digit) -> std::enable_if_t<!std::is_same<decltype(std::declval<T>().mPMNumber), uint8_t>::value, const decltype(std::declval<T>().mPMNumber)&>
{
  return digit.mPMNumber;
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
template <typename T>
auto getTime(const T& digit) -> std::enable_if_t<std::is_same<decltype(std::declval<T>().mTime), int16_t>::value, const decltype(std::declval<T>().mTime)&>
{
  return digit.mTime;
}
template <typename T>
auto getTime(const T& digit) -> std::enable_if_t<!std::is_same<decltype(std::declval<T>().mTime), int16_t>::value, const decltype(std::declval<T>().mTime)&>
{
  return digit.mTime;
}
template <typename T>
auto getTime(const T& digit) -> std::enable_if_t<std::is_same<decltype(std::declval<T>().time), int16_t>::value, const decltype(std::declval<T>().time)&>
{
  return digit.time;
}
template <typename T>
auto getTime(const T& digit) -> std::enable_if_t<!std::is_same<decltype(std::declval<T>().time), int16_t>::value, const decltype(std::declval<T>().time)&>
{
  return digit.time;
}
//Amplitude section
template <typename T>
auto getCharge(const T& digit) -> std::enable_if_t<std::is_same<decltype(std::declval<T>().mChargeADC), int16_t>::value, const decltype(std::declval<T>().mChargeADC)&>
{
  return digit.mChargeADC;
}
template <typename T>
auto getCharge(const T& digit) -> std::enable_if_t<!std::is_same<decltype(std::declval<T>().mChargeADC), int16_t>::value, const decltype(std::declval<T>().mChargeADC)&>
{
  return digit.mChargeADC;
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

//PM Bits section
template <typename T>
auto getPMbits(const T& digit) -> std::enable_if_t<std::is_same<decltype(std::declval<T>().mFEEBits), int16_t>::value, const decltype(std::declval<T>().mFEEBits)&>
{
  return digit.mFEEBits;
}
template <typename T>
auto getPMbits(const T& digit) -> std::enable_if_t<!std::is_same<decltype(std::declval<T>().mFEEBits), int16_t>::value, const decltype(std::declval<T>().mFEEBits)&>
{
  return digit.mFEEBits;
}
template <typename T>
auto getPMbits(const T& digit) -> std::enable_if_t<std::is_same<decltype(std::declval<T>().pmBits), int16_t>::value, const decltype(std::declval<T>().pmBits)&>
{
  return digit.pmBits;
}
template <typename T>
auto getPMbits(const T& digit) -> std::enable_if_t<!std::is_same<decltype(std::declval<T>().pmBits), int16_t>::value, const decltype(std::declval<T>().pmBits)&>
{
  return digit.pmBits;
}
} //namespace channel_data
} //namespace helper
} //namespace o2::quality_control_modules::fdd

#endif // QC_MODULE_FDD_HELPER_H