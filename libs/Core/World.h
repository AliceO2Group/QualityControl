///
/// \file    World.h
/// \author  Barthelemy von Haller
///

#ifndef ALICEO2_QC_CORE_WORLD_H
#define ALICEO2_QC_CORE_WORLD_H

/// \brief    Here you put a short description of the namespace
/// Extended documentation for this namespace
/// \author  	Barthelemy von Haller
namespace AliceO2 {
namespace QualityControl {
namespace Core {

/// \brief   Here you put a short description of the class
/// Extended documentation for this class.
/// \author 	Barthelemy von Haller
class World {
public:

  /// \brief   Greets the caller
  /// \author 	Barthelemy von Haller
  /// \brief	Simple hello world
  void greet();

  /// \brief   Returns the value passed to it
  /// Longer description that is useless here.
  /// \author 	Barthelemy von Haller
  /// \param n (In) input number.
  /// \return Returns the input number given.
  int returnsN(int n);
};

} // namespace Core
} // namespace QC
} // namespace AliceO2

#endif // ALICEO2_DATASAMPLING_WORLD_H
