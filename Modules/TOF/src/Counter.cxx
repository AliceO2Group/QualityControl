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
/// \file   Counter.cxx
/// \author Nicolo' Jacazio
///

#include "Base/Counter.h"

namespace o2::quality_control_modules::tof
{

template <typename Tc, Tc dim>
void Counter<Tc, dim>::Count(Tc v)
{
  assert(v > 0);
  ILOG(Info) << "Incrementing " << v << " to " << counter[v];
  counter[v]++;
}

template <typename Tc, Tc dim>
void Counter<Tc, dim>::Reset()
{
  ILOG(Info) << "Resetting Counter";
  for (UInt_t i = 0; i < size; i++) {
    counter[i] = 0;
  }
}

template <UInt_t dim, typename Tc, Tc cdim>
void CounterList<dim, Tc, cdim>::Count(UInt_t c, Tc v)
{
  assert(c < size);
  counter[c].Count(v);
}

template <UInt_t dim, typename Tc, Tc cdim>
void CounterList<dim, Tc, cdim>::Reset()
{
  for (Int_t i = 0; i < size; i++) {
    counter[i].Reset();
  }
}

template <UInt_t dimX, UInt_t dimY, typename Tc, Tc cdim>
void CounterMatrix<dimX, dimY, Tc, cdim>::Count(UInt_t c, UInt_t cc, Tc v)
{
  assert(c < size);
  counter[c].Count(cc, v);
}

template <UInt_t dimX, UInt_t dimY, typename Tc, Tc cdim>
void CounterMatrix<dimX, dimY, Tc, cdim>::Reset()
{
  for (Int_t i = 0; i < size; i++) {
    counter[i].Reset();
  }
}

} // namespace o2::quality_control_modules::tof
