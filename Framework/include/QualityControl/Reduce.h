//
// Created by pkonopka on 29/08/2019.
//

#ifndef QUALITYCONTROL_REDUCE_H
#define QUALITYCONTROL_REDUCE_H


namespace o2::quality_control::postprocessing
{

using ReduceFcn = std::function<double(TObject*)>;

// todo: root function reflection?
namespace reductors
{

ReduceFcn mean() {
  return [](const TObject* ) {

  }
}

ReduceFcn reductorFactory(std::string reductor) {
  if (reductor == "Mean") {
    return mean();
  } else {
    throw std::runtime_error("unknown reductor: " + reductor);
  }
}

}

} // namespace o2::quality_control::postprocessing


#endif //QUALITYCONTROL_REDUCE_H
