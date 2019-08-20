//
// Created by pkonopka on 19/08/2019.
//

#ifndef QUALITYCONTROL_TRENDINGINTERFACE_H
#define QUALITYCONTROL_TRENDINGINTERFACE_H

#include <functional>
#include <pair>
#include <TObject.h>

using ReduceFcn = std::function<std::pair<double, double>(TObject*)>;


class TrendInterface {

  // maybe use merger to do the job?
  trend(TObject* newEntry);


  TObject mTrend;
  ReduceFcn mReduce;

};

#endif //QUALITYCONTROL_TRENDINGINTERFACE_H
;