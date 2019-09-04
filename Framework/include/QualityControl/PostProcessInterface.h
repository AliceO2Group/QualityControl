//
// Created by pkonopka on 19/08/2019.
//

#ifndef QUALITYCONTROL_POSTPROCESSINTERFACE_H
#define QUALITYCONTROL_POSTPROCESSINTERFACE_H


#include <CCDB/CcdbApi.h>


namespace o2::quality_control::postprocessing {

class PostProcessingInterface {

  // all three - only TObjects. how about any object with a dictionary?

  // - how to do correlation with that?
  process(TObject* obj);
  // - difficult as an API
  process(TObject* obj, ...);
  //
  process(const std::vector<TObject*> objs);
  requestedInputs();


  // user gets to know what triggered the init
  void init(const Trigger);
  // user gets to know what triggered the processing
  void postProcess(const Trigger);
  // user gets to know what triggered the end
  void finalize(const Trigger);
  // store your stuff
  void store();
  // reset your stuff. maybe a trigger needed?
  void reset();

  // todo: ccdb api which does not allow to delete?

  protected:


};

// namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_POSTPROCESSINTERFACE_H
