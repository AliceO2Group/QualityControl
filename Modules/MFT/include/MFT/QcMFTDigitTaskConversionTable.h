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
/// \file   BasicDigitQcTaskConversionTable.h
/// \author Tomas Herman
/// \author Guillermo Contreras
/// \author Katarina Krizkova Gajdosova
/// \author Diana Maria Krupova
///

//  header file to be used by QcMFTDigitTask.cxx

int mLadder[936] = {
  0, 0, 10, 10, 11, 11, 12, 12, 13, 13,
  23, 23, 1, 1, 1, 2, 2, 2, 3, 3,
  3, 4, 4, 4, 5, 5, 5, 6, 6, 6,
  7, 7, 7, 8, 8, 8, 9, 9, 9, 14,
  14, 14, 15, 15, 15, 16, 16, 16, 17, 17,
  17, 18, 18, 18, 19, 19, 19, 20, 20, 20,
  21, 21, 21, 22, 22, 22, 0, 0, 10, 10,
  11, 11, 12, 12, 13, 13, 23, 23, 1, 1,
  1, 2, 2, 2, 3, 3, 3, 4, 4, 4,
  5, 5, 5, 6, 6, 6, 7, 7, 7, 8,
  8, 8, 9, 9, 9, 14, 14, 14, 15, 15,
  15, 16, 16, 16, 17, 17, 17, 18, 18, 18,
  19, 19, 19, 20, 20, 20, 21, 21, 21, 22,
  22, 22, 0, 0, 12, 12, 13, 13, 25, 25,
  1, 1, 1, 2, 2, 2, 5, 5, 5, 6,
  6, 6, 7, 7, 7, 10, 10, 10, 11, 11,
  11, 14, 14, 14, 15, 15, 15, 18, 18, 18,
  19, 19, 19, 20, 20, 20, 23, 23, 23, 24,
  24, 24, 3, 3, 3, 3, 4, 4, 4, 4,
  8, 8, 8, 8, 9, 9, 9, 9, 16, 16,
  16, 16, 17, 17, 17, 17, 21, 21, 21, 21,
  22, 22, 22, 22, 0, 0, 0, 1, 1, 1,
  13, 13, 13, 14, 14, 14, 15, 15, 15, 16,
  16, 16, 17, 17, 17, 18, 18, 18, 30, 30,
  30, 31, 31, 31, 2, 2, 2, 2, 3, 3,
  3, 3, 4, 4, 4, 4, 5, 5, 5, 5,
  6, 6, 6, 6, 7, 7, 7, 7, 8, 8,
  8, 8, 9, 9, 9, 9, 10, 10, 10, 10,
  11, 11, 11, 11, 12, 12, 12, 12, 19, 19,
  19, 19, 20, 20, 20, 20, 21, 21, 21, 21,
  22, 22, 22, 22, 23, 23, 23, 23, 24, 24,
  24, 24, 25, 25, 25, 25, 26, 26, 26, 26,
  27, 27, 27, 27, 28, 28, 28, 28, 29, 29,
  29, 29, 0, 0, 0, 1, 1, 1, 15, 15,
  15, 16, 16, 16, 17, 17, 17, 18, 18, 18,
  32, 32, 32, 33, 33, 33, 2, 2, 2, 2,
  3, 3, 3, 3, 6, 6, 6, 6, 7, 7,
  7, 7, 8, 8, 8, 8, 9, 9, 9, 9,
  10, 10, 10, 10, 13, 13, 13, 13, 14, 14,
  14, 14, 19, 19, 19, 19, 20, 20, 20, 20,
  23, 23, 23, 23, 24, 24, 24, 24, 25, 25,
  25, 25, 26, 26, 26, 26, 27, 27, 27, 27,
  30, 30, 30, 30, 31, 31, 31, 31, 4, 4,
  4, 4, 4, 5, 5, 5, 5, 5, 11, 11,
  11, 11, 11, 12, 12, 12, 12, 12, 21, 21,
  21, 21, 21, 22, 22, 22, 22, 22, 28, 28,
  28, 28, 28, 29, 29, 29, 29, 29, 0, 0,
  10, 10, 11, 11, 12, 12, 13, 13, 23, 23,
  1, 1, 1, 2, 2, 2, 3, 3, 3, 4,
  4, 4, 5, 5, 5, 6, 6, 6, 7, 7,
  7, 8, 8, 8, 9, 9, 9, 14, 14, 14,
  15, 15, 15, 16, 16, 16, 17, 17, 17, 18,
  18, 18, 19, 19, 19, 20, 20, 20, 21, 21,
  21, 22, 22, 22, 0, 0, 10, 10, 11, 11,
  12, 12, 13, 13, 23, 23, 1, 1, 1, 2,
  2, 2, 3, 3, 3, 4, 4, 4, 5, 5,
  5, 6, 6, 6, 7, 7, 7, 8, 8, 8,
  9, 9, 9, 14, 14, 14, 15, 15, 15, 16,
  16, 16, 17, 17, 17, 18, 18, 18, 19, 19,
  19, 20, 20, 20, 21, 21, 21, 22, 22, 22,
  0, 0, 12, 12, 13, 13, 25, 25, 1, 1,
  1, 2, 2, 2, 5, 5, 5, 6, 6, 6,
  7, 7, 7, 10, 10, 10, 11, 11, 11, 14,
  14, 14, 15, 15, 15, 18, 18, 18, 19, 19,
  19, 20, 20, 20, 23, 23, 23, 24, 24, 24,
  3, 3, 3, 3, 4, 4, 4, 4, 8, 8,
  8, 8, 9, 9, 9, 9, 16, 16, 16, 16,
  17, 17, 17, 17, 21, 21, 21, 21, 22, 22,
  22, 22, 0, 0, 0, 1, 1, 1, 13, 13,
  13, 14, 14, 14, 15, 15, 15, 16, 16, 16,
  17, 17, 17, 18, 18, 18, 30, 30, 30, 31,
  31, 31, 2, 2, 2, 2, 3, 3, 3, 3,
  4, 4, 4, 4, 5, 5, 5, 5, 6, 6,
  6, 6, 7, 7, 7, 7, 8, 8, 8, 8,
  9, 9, 9, 9, 10, 10, 10, 10, 11, 11,
  11, 11, 12, 12, 12, 12, 19, 19, 19, 19,
  20, 20, 20, 20, 21, 21, 21, 21, 22, 22,
  22, 22, 23, 23, 23, 23, 24, 24, 24, 24,
  25, 25, 25, 25, 26, 26, 26, 26, 27, 27,
  27, 27, 28, 28, 28, 28, 29, 29, 29, 29,
  0, 0, 0, 1, 1, 1, 15, 15, 15, 16,
  16, 16, 17, 17, 17, 18, 18, 18, 32, 32,
  32, 33, 33, 33, 2, 2, 2, 2, 3, 3,
  3, 3, 6, 6, 6, 6, 7, 7, 7, 7,
  8, 8, 8, 8, 9, 9, 9, 9, 10, 10,
  10, 10, 13, 13, 13, 13, 14, 14, 14, 14,
  19, 19, 19, 19, 20, 20, 20, 20, 23, 23,
  23, 23, 24, 24, 24, 24, 25, 25, 25, 25,
  26, 26, 26, 26, 27, 27, 27, 27, 30, 30,
  30, 30, 31, 31, 31, 31, 4, 4, 4, 4,
  4, 5, 5, 5, 5, 5, 11, 11, 11, 11,
  11, 12, 12, 12, 12, 12, 21, 21, 21, 21,
  21, 22, 22, 22, 22, 22, 28, 28, 28, 28,
  28, 29, 29, 29, 29, 29
};

float x[936] = {
  -8.8, -8.8, 8.2, 8.2, 9.9, 9.9, -9.9, -9.9, -8.2, -8.2,
  8.8, 8.8, -7.1, -7.1, -7.1, -5.4, -5.4, -5.4, -3.7, -3.7,
  -3.7, -2, -2, -2, -0.3, -0.3, -0.3, 1.4, 1.4, 1.4,
  3.1, 3.1, 3.1, 4.8, 4.8, 4.8, 6.5, 6.5, 6.5, -6.5,
  -6.5, -6.5, -4.8, -4.8, -4.8, -3.1, -3.1, -3.1, -1.4, -1.4,
  -1.4, 0.3, 0.3, 0.3, 2, 2, 2, 3.7, 3.7, 3.7,
  5.4, 5.4, 5.4, 7.1, 7.1, 7.1, -8.8, -8.8, 8.2, 8.2,
  9.9, 9.9, -9.9, -9.9, -8.2, -8.2, 8.8, 8.8, -7.1, -7.1,
  -7.1, -5.4, -5.4, -5.4, -3.7, -3.7, -3.7, -2, -2, -2,
  -0.3, -0.3, -0.3, 1.4, 1.4, 1.4, 3.1, 3.1, 3.1, 4.8,
  4.8, 4.8, 6.5, 6.5, 6.5, -6.5, -6.5, -6.5, -4.8, -4.8,
  -4.8, -3.1, -3.1, -3.1, -1.4, -1.4, -1.4, 0.3, 0.3, 0.3,
  2, 2, 2, 3.7, 3.7, 3.7, 5.4, 5.4, 5.4, 7.1,
  7.1, 7.1, -10.5, -10.5, 9.9, 9.9, -9.9, -9.9, 10.5, 10.5,
  -8.8, -8.8, -8.8, -7.1, -7.1, -7.1, -2, -2, -2, -0.3,
  -0.3, -0.3, 1.4, 1.4, 1.4, 6.5, 6.5, 6.5, 8.2, 8.2,
  8.2, -8.2, -8.2, -8.2, -6.5, -6.5, -6.5, -1.4, -1.4, -1.4,
  0.3, 0.3, 0.3, 2, 2, 2, 7.1, 7.1, 7.1, 8.8,
  8.8, 8.8, -5.4, -5.4, -5.4, -5.4, -3.7, -3.7, -3.7, -3.7,
  3.1, 3.1, 3.1, 3.1, 4.8, 4.8, 4.8, 4.8, -4.8, -4.8,
  -4.8, -4.8, -3.1, -3.1, -3.1, -3.1, 3.7, 3.7, 3.7, 3.7,
  5.4, 5.4, 5.4, 5.4, -12.2, -12.2, -12.2, -10.5, -10.5, -10.5,
  9.9, 9.9, 9.9, 11.6, 11.6, 11.6, 13.3, 13.3, 13.3, -13.3,
  -13.3, -13.3, -11.6, -11.6, -11.6, -9.9, -9.9, -9.9, 10.5, 10.5,
  10.5, 12.2, 12.2, 12.2, -8.8, -8.8, -8.8, -8.8, -7.1, -7.1,
  -7.1, -7.1, -5.4, -5.4, -5.4, -5.4, -3.7, -3.7, -3.7, -3.7,
  -2, -2, -2, -2, -0.3, -0.3, -0.3, -0.3, 1.4, 1.4,
  1.4, 1.4, 3.1, 3.1, 3.1, 3.1, 4.8, 4.8, 4.8, 4.8,
  6.5, 6.5, 6.5, 6.5, 8.2, 8.2, 8.2, 8.2, -8.2, -8.2,
  -8.2, -8.2, -6.5, -6.5, -6.5, -6.5, -4.8, -4.8, -4.8, -4.8,
  -3.1, -3.1, -3.1, -3.1, -1.4, -1.4, -1.4, -1.4, 0.3, 0.3,
  0.3, 0.3, 2, 2, 2, 2, 3.7, 3.7, 3.7, 3.7,
  5.4, 5.4, 5.4, 5.4, 7.1, 7.1, 7.1, 7.1, 8.8, 8.8,
  8.8, 8.8, -13.9, -13.9, -13.9, -12.2, -12.2, -12.2, 11.6, 11.6,
  11.6, 13.3, 13.3, 13.3, -13.3, -13.3, -13.3, -11.6, -11.6, -11.6,
  12.2, 12.2, 12.2, 13.9, 13.9, 13.9, -10.5, -10.5, -10.5, -10.5,
  -8.8, -8.8, -8.8, -8.8, -3.7, -3.7, -3.7, -3.7, -2, -2,
  -2, -2, -0.3, -0.3, -0.3, -0.3, 1.4, 1.4, 1.4, 1.4,
  3.1, 3.1, 3.1, 3.1, 8.2, 8.2, 8.2, 8.2, 9.9, 9.9,
  9.9, 9.9, -9.9, -9.9, -9.9, -9.9, -8.2, -8.2, -8.2, -8.2,
  -3.1, -3.1, -3.1, -3.1, -1.4, -1.4, -1.4, -1.4, 0.3, 0.3,
  0.3, 0.3, 2, 2, 2, 2, 3.7, 3.7, 3.7, 3.7,
  8.8, 8.8, 8.8, 8.8, 10.5, 10.5, 10.5, 10.5, -7.1, -7.1,
  -7.1, -7.1, -7.1, -5.4, -5.4, -5.4, -5.4, -5.4, 4.8, 4.8,
  4.8, 4.8, 4.8, 6.5, 6.5, 6.5, 6.5, 6.5, -6.5, -6.5,
  -6.5, -6.5, -6.5, -4.8, -4.8, -4.8, -4.8, -4.8, 5.4, 5.4,
  5.4, 5.4, 5.4, 7.1, 7.1, 7.1, 7.1, 7.1, 8.8, 8.8,
  -8.2, -8.2, -9.9, -9.9, 9.9, 9.9, 8.2, 8.2, -8.8, -8.8,
  7.1, 7.1, 7.1, 5.4, 5.4, 5.4, 3.7, 3.7, 3.7, 2,
  2, 2, 0.3, 0.3, 0.3, -1.4, -1.4, -1.4, -3.1, -3.1,
  -3.1, -4.8, -4.8, -4.8, -6.5, -6.5, -6.5, 6.5, 6.5, 6.5,
  4.8, 4.8, 4.8, 3.1, 3.1, 3.1, 1.4, 1.4, 1.4, -0.3,
  -0.3, -0.3, -2, -2, -2, -3.7, -3.7, -3.7, -5.4, -5.4,
  -5.4, -7.1, -7.1, -7.1, 8.8, 8.8, -8.2, -8.2, -9.9, -9.9,
  9.9, 9.9, 8.2, 8.2, -8.8, -8.8, 7.1, 7.1, 7.1, 5.4,
  5.4, 5.4, 3.7, 3.7, 3.7, 2, 2, 2, 0.3, 0.3,
  0.3, -1.4, -1.4, -1.4, -3.1, -3.1, -3.1, -4.8, -4.8, -4.8,
  -6.5, -6.5, -6.5, 6.5, 6.5, 6.5, 4.8, 4.8, 4.8, 3.1,
  3.1, 3.1, 1.4, 1.4, 1.4, -0.3, -0.3, -0.3, -2, -2,
  -2, -3.7, -3.7, -3.7, -5.4, -5.4, -5.4, -7.1, -7.1, -7.1,
  10.5, 10.5, -9.9, -9.9, 9.9, 9.9, -10.5, -10.5, 8.8, 8.8,
  8.8, 7.1, 7.1, 7.1, 2, 2, 2, 0.3, 0.3, 0.3,
  -1.4, -1.4, -1.4, -6.5, -6.5, -6.5, -8.2, -8.2, -8.2, 8.2,
  8.2, 8.2, 6.5, 6.5, 6.5, 1.4, 1.4, 1.4, -0.3, -0.3,
  -0.3, -2, -2, -2, -7.1, -7.1, -7.1, -8.8, -8.8, -8.8,
  5.4, 5.4, 5.4, 5.4, 3.7, 3.7, 3.7, 3.7, -3.1, -3.1,
  -3.1, -3.1, -4.8, -4.8, -4.8, -4.8, 4.8, 4.8, 4.8, 4.8,
  3.1, 3.1, 3.1, 3.1, -3.7, -3.7, -3.7, -3.7, -5.4, -5.4,
  -5.4, -5.4, 12.2, 12.2, 12.2, 10.5, 10.5, 10.5, -9.9, -9.9,
  -9.9, -11.6, -11.6, -11.6, -13.3, -13.3, -13.3, 13.3, 13.3, 13.3,
  11.6, 11.6, 11.6, 9.9, 9.9, 9.9, -10.5, -10.5, -10.5, -12.2,
  -12.2, -12.2, 8.8, 8.8, 8.8, 8.8, 7.1, 7.1, 7.1, 7.1,
  5.4, 5.4, 5.4, 5.4, 3.7, 3.7, 3.7, 3.7, 2, 2,
  2, 2, 0.3, 0.3, 0.3, 0.3, -1.4, -1.4, -1.4, -1.4,
  -3.1, -3.1, -3.1, -3.1, -4.8, -4.8, -4.8, -4.8, -6.5, -6.5,
  -6.5, -6.5, -8.2, -8.2, -8.2, -8.2, 8.2, 8.2, 8.2, 8.2,
  6.5, 6.5, 6.5, 6.5, 4.8, 4.8, 4.8, 4.8, 3.1, 3.1,
  3.1, 3.1, 1.4, 1.4, 1.4, 1.4, -0.3, -0.3, -0.3, -0.3,
  -2, -2, -2, -2, -3.7, -3.7, -3.7, -3.7, -5.4, -5.4,
  -5.4, -5.4, -7.1, -7.1, -7.1, -7.1, -8.8, -8.8, -8.8, -8.8,
  13.9, 13.9, 13.9, 12.2, 12.2, 12.2, -11.6, -11.6, -11.6, -13.3,
  -13.3, -13.3, 13.3, 13.3, 13.3, 11.6, 11.6, 11.6, -12.2, -12.2,
  -12.2, -13.9, -13.9, -13.9, 10.5, 10.5, 10.5, 10.5, 8.8, 8.8,
  8.8, 8.8, 3.7, 3.7, 3.7, 3.7, 2, 2, 2, 2,
  0.3, 0.3, 0.3, 0.3, -1.4, -1.4, -1.4, -1.4, -3.1, -3.1,
  -3.1, -3.1, -8.2, -8.2, -8.2, -8.2, -9.9, -9.9, -9.9, -9.9,
  9.9, 9.9, 9.9, 9.9, 8.2, 8.2, 8.2, 8.2, 3.1, 3.1,
  3.1, 3.1, 1.4, 1.4, 1.4, 1.4, -0.3, -0.3, -0.3, -0.3,
  -2, -2, -2, -2, -3.7, -3.7, -3.7, -3.7, -8.8, -8.8,
  -8.8, -8.8, -10.5, -10.5, -10.5, -10.5, 7.1, 7.1, 7.1, 7.1,
  7.1, 5.4, 5.4, 5.4, 5.4, 5.4, -4.8, -4.8, -4.8, -4.8,
  -4.8, -6.5, -6.5, -6.5, -6.5, -6.5, 6.5, 6.5, 6.5, 6.5,
  6.5, 4.8, 4.8, 4.8, 4.8, 4.8, -5.4, -5.4, -5.4, -5.4,
  -5.4, -7.1, -7.1, -7.1, -7.1, -7.1
};

float y[936] = {
  -1.7, -4.715, -1.7, -4.715, -1.7, -4.715, -1.7, -4.715, -1.7, -4.715,
  -1.7, -4.715, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -1.7, -4.715,
  -7.73, -3.546, -6.561, -9.576, -3.8, -6.815, -9.83, -3.706, -6.721, -9.736,
  -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -1.7,
  -4.715, -7.73, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -3.706, -6.721,
  -9.736, -3.8, -6.815, -9.83, -3.546, -6.561, -9.576, -1.7, -4.715, -7.73,
  -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -1.7, -4.715, -1.7, -4.715,
  -1.7, -4.715, -1.7, -4.715, -1.7, -4.715, -1.7, -4.715, -1.7, -4.715,
  -7.73, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -3.546, -6.561, -9.576,
  -3.8, -6.815, -9.83, -3.706, -6.721, -9.736, -1.7, -4.715, -7.73, -1.7,
  -4.715, -7.73, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -1.7, -4.715,
  -7.73, -1.7, -4.715, -7.73, -3.706, -6.721, -9.736, -3.8, -6.815, -9.83,
  -3.546, -6.561, -9.576, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -1.7,
  -4.715, -7.73, -1.7, -4.715, -1.7, -4.715, -1.7, -4.715, -1.7, -4.715,
  -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -3.55, -6.565, -9.58, -3.8,
  -6.815, -9.83, -3.71, -6.725, -9.74, -1.7, -4.715, -7.73, -1.7, -4.715,
  -7.73, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -3.71, -6.725, -9.74,
  -3.8, -6.815, -9.83, -3.55, -6.565, -9.58, -1.7, -4.715, -7.73, -1.7,
  -4.715, -7.73, -1.7, -4.715, -7.73, -10.745, -1.7, -4.715, -7.73, -10.745,
  -1.7, -4.715, -7.73, -10.745, -1.7, -4.715, -7.73, -10.745, -1.7, -4.715,
  -7.73, -10.745, -1.7, -4.715, -7.73, -10.745, -1.7, -4.715, -7.73, -10.745,
  -1.7, -4.715, -7.73, -10.745, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73,
  -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -1.7,
  -4.715, -7.73, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -1.7, -4.715,
  -7.73, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -10.745, -1.7, -4.715,
  -7.73, -10.745, -1.7, -4.715, -7.73, -10.745, -3.5, -6.515, -9.53, -12.545,
  -4.735, -7.75, -10.765, -13.78, -4.9, -7.915, -10.93, -13.945, -4.84, -7.855,
  -10.87, -13.885, -3.97, -6.985, -10, -13.015, -1.7, -4.715, -7.73, -10.745,
  -1.7, -4.715, -7.73, -10.745, -1.7, -4.715, -7.73, -10.745, -1.7, -4.715,
  -7.73, -10.745, -1.7, -4.715, -7.73, -10.745, -1.7, -4.715, -7.73, -10.745,
  -3.97, -6.985, -10, -13.015, -4.84, -7.855, -10.87, -13.885, -4.9, -7.915,
  -10.93, -13.945, -4.735, -7.75, -10.765, -13.78, -3.5, -6.515, -9.53, -12.545,
  -1.7, -4.715, -7.73, -10.745, -1.7, -4.715, -7.73, -10.745, -1.7, -4.715,
  -7.73, -10.745, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -1.7, -4.715,
  -7.73, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73,
  -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -1.7, -4.715, -7.73, -10.745,
  -1.7, -4.715, -7.73, -10.745, -4.125, -7.14, -10.155, -13.17, -5.155, -8.17,
  -11.185, -14.2, -5.3, -8.315, -11.33, -14.345, -5.245, -8.26, -11.275, -14.29,
  -4.5, -7.515, -10.53, -13.545, -1.7, -4.715, -7.73, -10.745, -1.7, -4.715,
  -7.73, -10.745, -1.7, -4.715, -7.73, -10.745, -1.7, -4.715, -7.73, -10.745,
  -4.5, -7.515, -10.53, -13.545, -5.245, -8.26, -11.275, -14.29, -5.3, -8.315,
  -11.33, -14.345, -5.155, -8.17, -11.185, -14.2, -4.125, -7.14, -10.155, -13.17,
  -1.7, -4.715, -7.73, -10.745, -1.7, -4.715, -7.73, -10.745, -1.7, -4.715,
  -7.73, -10.745, -13.76, -1.7, -4.715, -7.73, -10.745, -13.76, -1.7, -4.715,
  -7.73, -10.745, -13.76, -1.7, -4.715, -7.73, -10.745, -13.76, -1.7, -4.715,
  -7.73, -10.745, -13.76, -1.7, -4.715, -7.73, -10.745, -13.76, -1.7, -4.715,
  -7.73, -10.745, -13.76, -1.7, -4.715, -7.73, -10.745, -13.76, 1.7, 4.715,
  1.7, 4.715, 1.7, 4.715, 1.7, 4.715, 1.7, 4.715, 1.7, 4.715,
  1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 3.546,
  6.561, 9.576, 3.8, 6.815, 9.83, 3.706, 6.721, 9.736, 1.7, 4.715,
  7.73, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73,
  1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 3.706, 6.721, 9.736, 3.8,
  6.815, 9.83, 3.546, 6.561, 9.576, 1.7, 4.715, 7.73, 1.7, 4.715,
  7.73, 1.7, 4.715, 7.73, 1.7, 4.715, 1.7, 4.715, 1.7, 4.715,
  1.7, 4.715, 1.7, 4.715, 1.7, 4.715, 1.7, 4.715, 7.73, 1.7,
  4.715, 7.73, 1.7, 4.715, 7.73, 3.546, 6.561, 9.576, 3.8, 6.815,
  9.83, 3.706, 6.721, 9.736, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73,
  1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 1.7,
  4.715, 7.73, 3.706, 6.721, 9.736, 3.8, 6.815, 9.83, 3.546, 6.561,
  9.576, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73,
  1.7, 4.715, 1.7, 4.715, 1.7, 4.715, 1.7, 4.715, 1.7, 4.715,
  7.73, 1.7, 4.715, 7.73, 3.55, 6.565, 9.58, 3.8, 6.815, 9.83,
  3.71, 6.725, 9.74, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 1.7,
  4.715, 7.73, 1.7, 4.715, 7.73, 3.71, 6.725, 9.74, 3.8, 6.815,
  9.83, 3.55, 6.565, 9.58, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73,
  1.7, 4.715, 7.73, 10.745, 1.7, 4.715, 7.73, 10.745, 1.7, 4.715,
  7.73, 10.745, 1.7, 4.715, 7.73, 10.745, 1.7, 4.715, 7.73, 10.745,
  1.7, 4.715, 7.73, 10.745, 1.7, 4.715, 7.73, 10.745, 1.7, 4.715,
  7.73, 10.745, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 1.7, 4.715,
  7.73, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73,
  1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 1.7,
  4.715, 7.73, 1.7, 4.715, 7.73, 10.745, 1.7, 4.715, 7.73, 10.745,
  1.7, 4.715, 7.73, 10.745, 3.5, 6.515, 9.53, 12.545, 4.735, 7.75,
  10.765, 13.78, 4.9, 7.915, 10.93, 13.945, 4.84, 7.855, 10.87, 13.885,
  3.97, 6.985, 10, 13.015, 1.7, 4.715, 7.73, 10.745, 1.7, 4.715,
  7.73, 10.745, 1.7, 4.715, 7.73, 10.745, 1.7, 4.715, 7.73, 10.745,
  1.7, 4.715, 7.73, 10.745, 1.7, 4.715, 7.73, 10.745, 3.97, 6.985,
  10, 13.015, 4.84, 7.855, 10.87, 13.885, 4.9, 7.915, 10.93, 13.945,
  4.735, 7.75, 10.765, 13.78, 3.5, 6.515, 9.53, 12.545, 1.7, 4.715,
  7.73, 10.745, 1.7, 4.715, 7.73, 10.745, 1.7, 4.715, 7.73, 10.745,
  1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 1.7,
  4.715, 7.73, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 1.7, 4.715,
  7.73, 1.7, 4.715, 7.73, 1.7, 4.715, 7.73, 10.745, 1.7, 4.715,
  7.73, 10.745, 4.125, 7.14, 10.155, 13.17, 5.155, 8.17, 11.185, 14.2,
  5.3, 8.315, 11.33, 14.345, 5.245, 8.26, 11.275, 14.29, 4.5, 7.515,
  10.53, 13.545, 1.7, 4.715, 7.73, 10.745, 1.7, 4.715, 7.73, 10.745,
  1.7, 4.715, 7.73, 10.745, 1.7, 4.715, 7.73, 10.745, 4.5, 7.515,
  10.53, 13.545, 5.245, 8.26, 11.275, 14.29, 5.3, 8.315, 11.33, 14.345,
  5.155, 8.17, 11.185, 14.2, 4.125, 7.14, 10.155, 13.17, 1.7, 4.715,
  7.73, 10.745, 1.7, 4.715, 7.73, 10.745, 1.7, 4.715, 7.73, 10.745,
  13.76, 1.7, 4.715, 7.73, 10.745, 13.76, 1.7, 4.715, 7.73, 10.745,
  13.76, 1.7, 4.715, 7.73, 10.745, 13.76, 1.7, 4.715, 7.73, 10.745,
  13.76, 1.7, 4.715, 7.73, 10.745, 13.76, 1.7, 4.715, 7.73, 10.745,
  13.76, 1.7, 4.715, 7.73, 10.745, 13.76
};

float z[936] = {
  -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -46.7119, -46.7119, -46.7119, -46.7119,
  -46.7119, -46.7119, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881,
  -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881,
  -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -46.7119,
  -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119,
  -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119,
  -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -48.5881, -48.5881, -48.5881, -48.5881,
  -48.5881, -48.5881, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -48.5881, -48.5881,
  -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881,
  -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881,
  -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119,
  -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119,
  -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119,
  -50.0119, -50.0119, -52.3881, -52.3881, -52.3881, -52.3881, -53.8119, -53.8119, -53.8119, -53.8119,
  -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881,
  -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881,
  -52.3881, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119,
  -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119,
  -53.8119, -53.8119, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881,
  -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -53.8119, -53.8119,
  -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119,
  -53.8119, -53.8119, -53.8119, -53.8119, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881,
  -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -69.1119,
  -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119,
  -69.1119, -69.1119, -69.1119, -69.1119, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881,
  -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881,
  -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881,
  -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881,
  -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -69.1119, -69.1119,
  -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119,
  -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119,
  -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119,
  -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119,
  -69.1119, -69.1119, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881,
  -76.0881, -76.0881, -76.0881, -76.0881, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119,
  -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -76.0881, -76.0881, -76.0881, -76.0881,
  -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881,
  -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881,
  -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881,
  -76.0881, -76.0881, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119,
  -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119,
  -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119,
  -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -76.0881, -76.0881,
  -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881,
  -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -77.5119, -77.5119,
  -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119,
  -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -45.2881, -45.2881,
  -45.2881, -45.2881, -45.2881, -45.2881, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119,
  -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881,
  -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881,
  -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -45.2881, -46.7119, -46.7119, -46.7119,
  -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119,
  -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119, -46.7119,
  -46.7119, -46.7119, -46.7119, -46.7119, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881,
  -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -48.5881, -48.5881, -48.5881, -48.5881,
  -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881,
  -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881, -48.5881,
  -48.5881, -48.5881, -48.5881, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119,
  -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119,
  -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119, -50.0119,
  -52.3881, -52.3881, -52.3881, -52.3881, -53.8119, -53.8119, -53.8119, -53.8119, -52.3881, -52.3881,
  -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881,
  -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -53.8119,
  -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119,
  -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119,
  -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881,
  -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -52.3881, -53.8119, -53.8119, -53.8119, -53.8119,
  -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119, -53.8119,
  -53.8119, -53.8119, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881,
  -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -69.1119, -69.1119, -69.1119,
  -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119,
  -69.1119, -69.1119, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881,
  -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881,
  -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881,
  -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881,
  -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -67.6881, -69.1119, -69.1119, -69.1119, -69.1119,
  -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119,
  -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119,
  -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119,
  -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119, -69.1119,
  -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881,
  -76.0881, -76.0881, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119,
  -77.5119, -77.5119, -77.5119, -77.5119, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881,
  -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881,
  -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881,
  -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881,
  -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119,
  -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119,
  -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119,
  -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -76.0881, -76.0881, -76.0881, -76.0881,
  -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881,
  -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -76.0881, -77.5119, -77.5119, -77.5119, -77.5119,
  -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119,
  -77.5119, -77.5119, -77.5119, -77.5119, -77.5119, -77.5119
};

float binx[936] = {
  1, 1, 11, 11, 12, 12, 1, 1, 2, 2,
  12, 12, 2, 2, 2, 3, 3, 3, 4, 4,
  4, 5, 5, 5, 6, 6, 6, 7, 7, 7,
  8, 8, 8, 9, 9, 9, 10, 10, 10, 3,
  3, 3, 4, 4, 4, 5, 5, 5, 6, 6,
  6, 7, 7, 7, 8, 8, 8, 9, 9, 9,
  10, 10, 10, 11, 11, 11, 1, 1, 11, 11,
  12, 12, 1, 1, 2, 2, 12, 12, 2, 2,
  2, 3, 3, 3, 4, 4, 4, 5, 5, 5,
  6, 6, 6, 7, 7, 7, 8, 8, 8, 9,
  9, 9, 10, 10, 10, 3, 3, 3, 4, 4,
  4, 5, 5, 5, 6, 6, 6, 7, 7, 7,
  8, 8, 8, 9, 9, 9, 10, 10, 10, 11,
  11, 11, 1, 1, 13, 13, 1, 1, 13, 13,
  2, 2, 2, 3, 3, 3, 6, 6, 6, 7,
  7, 7, 8, 8, 8, 11, 11, 11, 12, 12,
  12, 2, 2, 2, 3, 3, 3, 6, 6, 6,
  7, 7, 7, 8, 8, 8, 11, 11, 11, 12,
  12, 12, 4, 4, 4, 4, 5, 5, 5, 5,
  9, 9, 9, 9, 10, 10, 10, 10, 4, 4,
  4, 4, 5, 5, 5, 5, 9, 9, 9, 9,
  10, 10, 10, 10, 1, 1, 1, 2, 2, 2,
  14, 14, 14, 15, 15, 15, 16, 16, 16, 1,
  1, 1, 2, 2, 2, 3, 3, 3, 15, 15,
  15, 16, 16, 16, 3, 3, 3, 3, 4, 4,
  4, 4, 5, 5, 5, 5, 6, 6, 6, 6,
  7, 7, 7, 7, 8, 8, 8, 8, 9, 9,
  9, 9, 10, 10, 10, 10, 11, 11, 11, 11,
  12, 12, 12, 12, 13, 13, 13, 13, 4, 4,
  4, 4, 5, 5, 5, 5, 6, 6, 6, 6,
  7, 7, 7, 7, 8, 8, 8, 8, 9, 9,
  9, 9, 10, 10, 10, 10, 11, 11, 11, 11,
  12, 12, 12, 12, 13, 13, 13, 13, 14, 14,
  14, 14, 1, 1, 1, 2, 2, 2, 16, 16,
  16, 17, 17, 17, 1, 1, 1, 2, 2, 2,
  16, 16, 16, 17, 17, 17, 3, 3, 3, 3,
  4, 4, 4, 4, 7, 7, 7, 7, 8, 8,
  8, 8, 9, 9, 9, 9, 10, 10, 10, 10,
  11, 11, 11, 11, 14, 14, 14, 14, 15, 15,
  15, 15, 3, 3, 3, 3, 4, 4, 4, 4,
  7, 7, 7, 7, 8, 8, 8, 8, 9, 9,
  9, 9, 10, 10, 10, 10, 11, 11, 11, 11,
  14, 14, 14, 14, 15, 15, 15, 15, 5, 5,
  5, 5, 5, 6, 6, 6, 6, 6, 12, 12,
  12, 12, 12, 13, 13, 13, 13, 13, 5, 5,
  5, 5, 5, 6, 6, 6, 6, 6, 12, 12,
  12, 12, 12, 13, 13, 13, 13, 13, 12, 12,
  2, 2, 1, 1, 12, 12, 11, 11, 1, 1,
  11, 11, 11, 10, 10, 10, 9, 9, 9, 8,
  8, 8, 7, 7, 7, 6, 6, 6, 5, 5,
  5, 4, 4, 4, 3, 3, 3, 10, 10, 10,
  9, 9, 9, 8, 8, 8, 7, 7, 7, 6,
  6, 6, 5, 5, 5, 4, 4, 4, 3, 3,
  3, 2, 2, 2, 12, 12, 2, 2, 1, 1,
  12, 12, 11, 11, 1, 1, 11, 11, 11, 10,
  10, 10, 9, 9, 9, 8, 8, 8, 7, 7,
  7, 6, 6, 6, 5, 5, 5, 4, 4, 4,
  3, 3, 3, 10, 10, 10, 9, 9, 9, 8,
  8, 8, 7, 7, 7, 6, 6, 6, 5, 5,
  5, 4, 4, 4, 3, 3, 3, 2, 2, 2,
  13, 13, 1, 1, 13, 13, 1, 1, 12, 12,
  12, 11, 11, 11, 8, 8, 8, 7, 7, 7,
  6, 6, 6, 3, 3, 3, 2, 2, 2, 12,
  12, 12, 11, 11, 11, 8, 8, 8, 7, 7,
  7, 6, 6, 6, 3, 3, 3, 2, 2, 2,
  10, 10, 10, 10, 9, 9, 9, 9, 5, 5,
  5, 5, 4, 4, 4, 4, 10, 10, 10, 10,
  9, 9, 9, 9, 5, 5, 5, 5, 4, 4,
  4, 4, 16, 16, 16, 15, 15, 15, 3, 3,
  3, 2, 2, 2, 1, 1, 1, 16, 16, 16,
  15, 15, 15, 14, 14, 14, 2, 2, 2, 1,
  1, 1, 14, 14, 14, 14, 13, 13, 13, 13,
  12, 12, 12, 12, 11, 11, 11, 11, 10, 10,
  10, 10, 9, 9, 9, 9, 8, 8, 8, 8,
  7, 7, 7, 7, 6, 6, 6, 6, 5, 5,
  5, 5, 4, 4, 4, 4, 13, 13, 13, 13,
  12, 12, 12, 12, 11, 11, 11, 11, 10, 10,
  10, 10, 9, 9, 9, 9, 8, 8, 8, 8,
  7, 7, 7, 7, 6, 6, 6, 6, 5, 5,
  5, 5, 4, 4, 4, 4, 3, 3, 3, 3,
  17, 17, 17, 16, 16, 16, 2, 2, 2, 1,
  1, 1, 17, 17, 17, 16, 16, 16, 2, 2,
  2, 1, 1, 1, 15, 15, 15, 15, 14, 14,
  14, 14, 11, 11, 11, 11, 10, 10, 10, 10,
  9, 9, 9, 9, 8, 8, 8, 8, 7, 7,
  7, 7, 4, 4, 4, 4, 3, 3, 3, 3,
  15, 15, 15, 15, 14, 14, 14, 14, 11, 11,
  11, 11, 10, 10, 10, 10, 9, 9, 9, 9,
  8, 8, 8, 8, 7, 7, 7, 7, 4, 4,
  4, 4, 3, 3, 3, 3, 13, 13, 13, 13,
  13, 12, 12, 12, 12, 12, 6, 6, 6, 6,
  6, 5, 5, 5, 5, 5, 13, 13, 13, 13,
  13, 12, 12, 12, 12, 12, 6, 6, 6, 6,
  6, 5, 5, 5, 5, 5
};

float biny[936] = {
  4, 3, 4, 3, 4, 3, 4, 3, 4, 3,
  4, 3, 4, 3, 2, 4, 3, 2, 4, 3,
  2, 3, 2, 1, 3, 2, 1, 3, 2, 1,
  4, 3, 2, 4, 3, 2, 4, 3, 2, 4,
  3, 2, 4, 3, 2, 4, 3, 2, 3, 2,
  1, 3, 2, 1, 3, 2, 1, 4, 3, 2,
  4, 3, 2, 4, 3, 2, 4, 3, 4, 3,
  4, 3, 4, 3, 4, 3, 4, 3, 4, 3,
  2, 4, 3, 2, 4, 3, 2, 3, 2, 1,
  3, 2, 1, 3, 2, 1, 4, 3, 2, 4,
  3, 2, 4, 3, 2, 4, 3, 2, 4, 3,
  2, 4, 3, 2, 3, 2, 1, 3, 2, 1,
  3, 2, 1, 4, 3, 2, 4, 3, 2, 4,
  3, 2, 4, 3, 4, 3, 4, 3, 4, 3,
  4, 3, 2, 4, 3, 2, 3, 2, 1, 3,
  2, 1, 3, 2, 1, 4, 3, 2, 4, 3,
  2, 4, 3, 2, 4, 3, 2, 3, 2, 1,
  3, 2, 1, 3, 2, 1, 4, 3, 2, 4,
  3, 2, 4, 3, 2, 1, 4, 3, 2, 1,
  4, 3, 2, 1, 4, 3, 2, 1, 4, 3,
  2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
  4, 3, 2, 1, 5, 4, 3, 5, 4, 3,
  5, 4, 3, 5, 4, 3, 5, 4, 3, 5,
  4, 3, 5, 4, 3, 5, 4, 3, 5, 4,
  3, 5, 4, 3, 5, 4, 3, 2, 5, 4,
  3, 2, 5, 4, 3, 2, 4, 3, 2, 1,
  4, 3, 2, 1, 4, 3, 2, 1, 4, 3,
  2, 1, 4, 3, 2, 1, 5, 4, 3, 2,
  5, 4, 3, 2, 5, 4, 3, 2, 5, 4,
  3, 2, 5, 4, 3, 2, 5, 4, 3, 2,
  4, 3, 2, 1, 4, 3, 2, 1, 4, 3,
  2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
  5, 4, 3, 2, 5, 4, 3, 2, 5, 4,
  3, 2, 5, 4, 3, 5, 4, 3, 5, 4,
  3, 5, 4, 3, 5, 4, 3, 5, 4, 3,
  5, 4, 3, 5, 4, 3, 5, 4, 3, 2,
  5, 4, 3, 2, 4, 3, 2, 1, 4, 3,
  2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
  4, 3, 2, 1, 5, 4, 3, 2, 5, 4,
  3, 2, 5, 4, 3, 2, 5, 4, 3, 2,
  4, 3, 2, 1, 4, 3, 2, 1, 4, 3,
  2, 1, 4, 3, 2, 1, 4, 3, 2, 1,
  5, 4, 3, 2, 5, 4, 3, 2, 5, 4,
  3, 2, 1, 5, 4, 3, 2, 1, 5, 4,
  3, 2, 1, 5, 4, 3, 2, 1, 5, 4,
  3, 2, 1, 5, 4, 3, 2, 1, 5, 4,
  3, 2, 1, 5, 4, 3, 2, 1, 1, 2,
  1, 2, 1, 2, 1, 2, 1, 2, 1, 2,
  1, 2, 3, 1, 2, 3, 1, 2, 3, 2,
  3, 4, 2, 3, 4, 2, 3, 4, 1, 2,
  3, 1, 2, 3, 1, 2, 3, 1, 2, 3,
  1, 2, 3, 1, 2, 3, 2, 3, 4, 2,
  3, 4, 2, 3, 4, 1, 2, 3, 1, 2,
  3, 1, 2, 3, 1, 2, 1, 2, 1, 2,
  1, 2, 1, 2, 1, 2, 1, 2, 3, 1,
  2, 3, 1, 2, 3, 2, 3, 4, 2, 3,
  4, 2, 3, 4, 1, 2, 3, 1, 2, 3,
  1, 2, 3, 1, 2, 3, 1, 2, 3, 1,
  2, 3, 2, 3, 4, 2, 3, 4, 2, 3,
  4, 1, 2, 3, 1, 2, 3, 1, 2, 3,
  1, 2, 1, 2, 1, 2, 1, 2, 1, 2,
  3, 1, 2, 3, 2, 3, 4, 2, 3, 4,
  2, 3, 4, 1, 2, 3, 1, 2, 3, 1,
  2, 3, 1, 2, 3, 2, 3, 4, 2, 3,
  4, 2, 3, 4, 1, 2, 3, 1, 2, 3,
  1, 2, 3, 4, 1, 2, 3, 4, 1, 2,
  3, 4, 1, 2, 3, 4, 1, 2, 3, 4,
  1, 2, 3, 4, 1, 2, 3, 4, 1, 2,
  3, 4, 1, 2, 3, 1, 2, 3, 1, 2,
  3, 1, 2, 3, 1, 2, 3, 1, 2, 3,
  1, 2, 3, 1, 2, 3, 1, 2, 3, 1,
  2, 3, 1, 2, 3, 4, 1, 2, 3, 4,
  1, 2, 3, 4, 2, 3, 4, 5, 2, 3,
  4, 5, 2, 3, 4, 5, 2, 3, 4, 5,
  2, 3, 4, 5, 1, 2, 3, 4, 1, 2,
  3, 4, 1, 2, 3, 4, 1, 2, 3, 4,
  1, 2, 3, 4, 1, 2, 3, 4, 2, 3,
  4, 5, 2, 3, 4, 5, 2, 3, 4, 5,
  2, 3, 4, 5, 2, 3, 4, 5, 1, 2,
  3, 4, 1, 2, 3, 4, 1, 2, 3, 4,
  1, 2, 3, 1, 2, 3, 1, 2, 3, 1,
  2, 3, 1, 2, 3, 1, 2, 3, 1, 2,
  3, 1, 2, 3, 1, 2, 3, 4, 1, 2,
  3, 4, 2, 3, 4, 5, 2, 3, 4, 5,
  2, 3, 4, 5, 2, 3, 4, 5, 2, 3,
  4, 5, 1, 2, 3, 4, 1, 2, 3, 4,
  1, 2, 3, 4, 1, 2, 3, 4, 2, 3,
  4, 5, 2, 3, 4, 5, 2, 3, 4, 5,
  2, 3, 4, 5, 2, 3, 4, 5, 1, 2,
  3, 4, 1, 2, 3, 4, 1, 2, 3, 4,
  5, 1, 2, 3, 4, 5, 1, 2, 3, 4,
  5, 1, 2, 3, 4, 5, 1, 2, 3, 4,
  5, 1, 2, 3, 4, 5, 1, 2, 3, 4,
  5, 1, 2, 3, 4, 5
};
