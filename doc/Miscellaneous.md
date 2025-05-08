
Miscellaneous
---

<!--TOC generated with https://github.com/ekalinin/github-markdown-toc-->
<!--./gh-md-toc --insert --no-backup --hide-footer --indent 3 QualityControl/doc/Advanced.md -->
<!--ts-->
* [Asynchronous Data and Monte Carlo QC operations](#asynchronous-data-and-monte-carlo-qc-operations)
* [QCG](#qcg)
   * [Display a non-standard ROOT object in QCG](#display-a-non-standard-root-object-in-qcg)
   * [Canvas options](#canvas-options)
   * [Local QCG (QC GUI) setup](#local-qcg-qc-gui-setup)
   * [Data Sampling monitoring](#data-sampling-monitoring)
   * [Monitoring metrics](#monitoring-metrics)
   * [Common check IncreasingEntries](#common-check-increasingentries)
   * [Common check TrendCheck](#common-check-trendcheck)
         * [Full configuration example](#full-configuration-example)
   * [Update the shmem segment size of a detector](#update-the-shmem-segment-size-of-a-detector)
      * [Readout chain (optional)](#readout-chain-optional)
         * [Getting real data from readout](#getting-real-data-from-readout)
         * [Readout data format as received by the Task](#readout-data-format-as-received-by-the-task)
<!--te-->

[← Go back to Post-processing](PostProcessing.md) | [↑ Go to the Table of Content ↑](../README.md) | [Continue to Frequently Asked Questions →](FAQ.md)



# Asynchronous Data and Monte Carlo QC operations

QC can accompany workflows reconstructing real and simulated data asynchronously.
Usually these are distributed among thousands of nodes which might not have access to each other, thus partial results are stored and merged in form of files with mechanism explained in [Batch processing](#batch-processing).

QC workflows for asynchronous data reconstructions are listed in [O2DPG/Data/production/qc-workflow.sh](https://github.com/AliceO2Group/O2DPG/blob/master/DATA/production/qc-workflow.sh).
The script includes paths to corresponding QC configuration files for subsystems which take part in the reconstruction.
All the enabled files are merged into a combined QC workflow.
Thus, it is crucial that unique keys are used in `tasks`, `checks` and `aggregators` structures, as explained in [Merging multiple configuration files into one](#merging-multiple-configuration-files-into-one).
Post-processing tasks can be added in the script [O2DPG/DATA/production/o2dpg_qc_postproc_workflow.py](https://github.com/AliceO2Group/O2DPG/blob/master/DATA/production/o2dpg_qc_postproc_workflow.py).
Please see the included example and the in-code documentation for further guidelines in this matter.

Generating and reconstructing simulated data is ran by a framework which organizes specific workflows in a directed acyclic graph and executes them in an order which satisfies all the dependencies and allocated computing resources.
In contrast to data reconstruction, here, QC workflows are executed separately and pick up corresponding input files.
For further details, please refer to [Adding QC Tasks to the simulation script](https://github.com/AliceO2Group/O2DPG/tree/master/MC#adding-qc-tasks-to-the-simulation-script).

Data and simulation productions are typically executed on Grid and EPNs, and the outcomes can be inspected in [MonALISA](http://alimonitor.cern.ch/).
In both cases, QC runs alongside of each subjob and incomplete QC results are stored in files.
For asynchronous data reconstruction, one file `QC.root` is created.
Simulation subjobs contain a `QC` directory with separate files for each QC workflow.
Relevant logs can be found in files like `stdout`, `stderr` as well as archives `debug_log_archive.zip` and `log_archive.zip`.

Once an expected percentage of subjobs completes, several QC merging stages are executed, each producing a merged file for certain range of subjobs.
The last stage produces the complete file for given masterjob.
This file is read by the `o2-qc --remote-batch` executable to run Checks on the complete objects and all the results to the QCDB.
Post-Processing Tasks and associated Checks are executed right after.

Some runs contain too much data to be processed with one masterjob.
In such case, several masterjobs are run in parallel.
Each produces a `QC.root` file which contains all the statistics for a masterjob.
The last masterjob to complete recognizes this fact and merges all `QC.root` into `QC_fullrun.root` and only then uploads the results to QCDB.
To find it, one can use `alien_find`:

```
> alien_find /alice/data/2022/LHC22m/523897/apass1_epn QC_fullrun.root
/alice/data/2022/LHC22m/523897/apass1_epn/0750/QC/001/QC_fullrun.root
```

TODO explain how a connection to QCDB is made from Grid sites.

# QCG

## Display a non-standard ROOT object in QCG

Users can publish objects inheriting from a custom class, e.g. not a TH2F but a MyCustomClass, as long as a dictionary is available. By default, JSROOT and in turn the QCG won't be able to display such objects.

The solution depends on the strategy to adopt to display the object.

1. The custom class has multiple inheritance and one of them is a standard ROOT object which the QCG can display (e.g. a histogram). In such case, add a member `mTreatMeAs` to your custom class and set it to the name of the class that should be used to interpret and display the data. There is an example in the Example module :

```c++
  std::string mTreatMeAs = "TH2F"; // the name of the class this object should be considered as when drawing in QCG.
```

2. [Not ready yet] The class encapsulates the object that should actually be drawn. Contact us if you need this feature, we can easily add it.
3. [Not ready yet] The class cannot be drawn in the ways outlined above and need a custom piece of JS to actually do it. Contact us if you need this feature, it is not a trivial thing to do.

## Canvas options

The developer of a Task might perfectly know how to display a plot or a graph but cannot set these options if they belong to the Canvas. This is typically the case of `drawOptions` such as `colz` or `alp`. It is also the case for canvases' properties such as logarithmic scale and grid. These options can be set by the end user in the QCG but it is likely that the developer wants to give pertinent default options.

To do so, one can use one of the two following methods.

* `getObjectsManager()->setDefaultDrawOptions(<objectname or pointer>, string& drawOptions)` where
  `drawOptions` is a space-separated list of drawing options. E.g. "colz" or "alp lego1".
* `getObjectsManager()->setDisplayHint(<objectname or pointer>, string& hints)` where
  `hints` is a space-separated list of hints on how to draw the object. E.g. "logz" or "gridy logy".
  Currently supported by QCG: logx, logy, logz, gridx, gridy, gridz.

These methods must be called after the objects has been published, i.e. after the call to `getObjectsManager()->startPublishing(<pointer to object>)

## Local QCG (QC GUI) setup

To install and run the QCG locally please follow these instructions : <https://github.com/AliceO2Group/WebUi/tree/dev/QualityControl#installation>



# Data Sampling monitoring

To have the monitoring metrics for the Data Sampling (the Dispatcher) sent to a specific sink (like influxdb), add the option `--monitoring-backend` when launching the DPL workflow. For example:

```shell
--monitoring-backend 'influxdb-udp://influxdb-server.cern.ch:8086'
```

This will actually send the monitoring data of _all_ DPL devices to this database.

**Note for mac users**: if you get a crash and the message "std::exception::what: send_to: Message too long", it means that you have to adapt a `udp` parameter. You can check the datagram size via `sudo sysctl net.inet.udp.maxdgram`. If it says something less than 64 kB, then increase size: `sudo sysctl -w net.inet.udp.maxdgram=65535`

# Monitoring metrics

The QC framework publishes monitoring metrics concerning data/object rates, which are published to the monitoring backend
specified in the `"monitoring.url"` parameter in config files. If QC is run in standalone mode (no AliECS) and with
`"infologger:///debug?qc"` as the monitoring backend, the metrics will appear in logs in buffered chunks. To force
printing them as soon as they are reported, please also add `--monitoring-backend infologger://` as the argument.

One can also enable publishing metrics related to CPU/memory usage. To do so, use `--resources-monitoring <interval_sec>`.

# Common check `IncreasingEntries`

This check make sures that the number of entries has increased in the past cycle(s). If not, it will display a pavetext
on the plot and set the quality to bad.

If you use `SetBinContent` the number of entries does not increase creating a false positive. Please call `ResetStats()`
after using `SetBinContent`.

The behaviour of the check can be inverted by setting the customparameter "mustIncrease" to "false" :

```
        "checkParameters": {
          "mustIncrease": "false"
        }
```

The number of cycles during which we tolerate increasing (or not respectively) the number of entries can be set with the custom parameter `nBadCyclesLimit`:

```
        "extendedCheckParameters": {
          "default": {
            "default": {
              "nBadCyclesLimit": "3",
            }
          }
        }
```

In the example above, the quality goes to bad when there are 3 cycles in a row with no increase in the number of entries.

# Common check `TrendCheck`

This check compares the last point of a trending plot with some minimum and maximum thresholds.

The thresholds can be defined in different ways, controlled by the `trendCheckMode` parameter:

* `"trendCheckMode": "ExpectedRange"` ==> fixed threshold values: the thresholds represent the minimum and maximum allowed values for the last point
* `"trendCheckMode": "DeviationFromMean"` ==> the thresholds represent the relative variation with respect to the mean of the N points preceding the last one (which is checked)
For example:

```
    "thresholdsBad": "-0.1,0.2",
```

means that the last point should not be lower than `(mean - 0.1 * |mean|)` and not higher than `(mean + 0.2 * |mean|)`.

* `"trendCheckMode": "StdDeviation"` ==> the thresholds represent the relative variation with respect to the total error of the N points preceding the last one (which is checked)
For example:

```
    "thresholdsBad": "-1,2",
```

means that the last point should not be lower than `(mean - 1 * TotError)` and not higher than `(mean + 2 * TotError)`.
The total error takes into account the standard deviation of the N points before the current one, as well as the error associated to the current point.

In general, the threshold values are configured separately for the Bad and Medium qualities, like this:

```
    "thresholdsBad": "min,max",
    "thresholdsMedium": "min,max",
```

It is also possible to customize the threshold values for specific plots:

```
    "thresholdsBad:PlotName": "min,max",
    "thresholdsMedium:PlotName": "min,max",
```

Here `PlotName` represents the name of the plot, stripped from all the QCDB path.

The position and size of the text label that shows the check result can also be customized in the configuration:

```
    "qualityLabelPosition": "0.5,0.8",
    "qualityLabelSize": "0.5,0.1"
```

The values are relative to the canvas size, so in the example above the label width is 50% of the canvas width and the label height is 10% of the canvas height.

### Full configuration example

```json
      "MyTrendingCheckFixed": {
        "active": "true",
        "className": "o2::quality_control_modules::common::TrendCheck",
        "moduleName": "QualityControl",
        "detectorName": "TST",
        "policy": "OnAll",
        "extendedCheckParameters": {
          "default": {
            "default": {
                "trendCheckMode": "ExpectedRange",
                "nPointsForAverage": "3",
                "": "default threshold values not specifying the plot name",
                "thresholdsMedium": "3000,7000",
                "thresholdsBad": "2000,8000"
                "": "thresholds  specific to one plot",
                "thresholdsBad:mean_of_histogram_2": "1000,9000",
                "": "customize the position and size of the text label showing the quality"
                "qualityLabelPosition": "0.5,0.8",
                "qualityLabelSize": "0.5,0.1"
            }
          }
        },
        "dataSource": [
          {
            "type": "PostProcessing",
            "name": "MyTrendingTask",
            "MOs" : [
              "mean_of_histogram_1", "mean_of_histogram_2"
            ]
          }
        ]
      },
      "MyTrendingCheckMean": {
        "active": "true",
        "className": "o2::quality_control_modules::common::TrendCheck",
        "moduleName": "QualityControl",
        "detectorName": "TST",
        "policy": "OnAll",
        "extendedCheckParameters": {
          "default": {
            "default": {
                "trendCheckMode": "DeviationFromMean",
                "nPointsForAverage": "3",  
                "thresholdsBad": "-0.2,0.5",      "": "from -20% to +50%",
                "thresholdsMedium": "-0.1,0.25"
            }
          }
        },
        "dataSource": [
          {
            "type": "PostProcessing",
            "name": "MyTrendingTask",
            "MOs" : [
              "mean_of_histogram_3"
            ]
          }
        ]
      },
      "MyTrendingCheckStdDev": {
        "active": "true",
        "className": "o2::quality_control_modules::common::TrendCheck",
        "moduleName": "QualityControl",
        "detectorName": "TST",
        "policy": "OnAll",
        "extendedCheckParameters": {
          "default": {
            "default": {
                "trendCheckMode": "StdDeviation",
                "nPointsForAverage": "5",  
                "thresholdsBad:mean_of_histogram_3": "-2,5",      "": "from -2sigma to +5sigma",
                "thresholdsMedium:mean_of_histogram_3": "-1,2.5"
            }
          }
        },
        "dataSource": [
          {
            "type": "PostProcessing",
            "name": "MyTrendingTask",
            "MOs" : [
              "mean_of_histogram_4"
            ]
          }
        ]
      }
```

# Update the shmem segment size of a detector

In consul go to `o2/runtime/aliecs/defaults` and modify the file corresponding to the detector: [det]_qc_shm_segment_size

## Readout chain (optional)

In this second example, we are going to use the Readout as our data source.
This example assumes that Readout has been compiled beforehand (`aliBuild build Readout --defaults o2`).

![alt text](images/readout-schema.png)

This workflow is a bit different from the basic one. The _Readout_ is not a DPL, nor a FairMQ, device and thus we have to have a _proxy_ to get data from it. This is the extra box going to the _Data Sampling_, which then injects data to the task. This is handled in the _Readout_ as long as you enable the corresponding configuration flag.

The first thing is to load the environment for the readout in a new terminal: `alienv enter Readout/latest`.

Then enable the data sampling channel in readout by opening the readout config file located at `$READOUT_ROOT/etc/readout-qc.cfg` and make sure that the following properties are correct:

```
# Enable the data sampling
[consumer-fmq-qc]
consumerType=FairMQChannel
enableRawFormat=1
fmq-name=readout-qc
fmq-address=ipc:///tmp/readout-pipe-1
fmq-type=pub
fmq-transport=zeromq
unmanagedMemorySize=2G
memoryPoolNumberOfPages=500
memoryPoolPageSize=1M
enabled=1
(...)
```

Start Readout in a terminal:
```
o2-readout-exe file://$READOUT_ROOT/etc/readout-qc.cfg
```

Start in another terminal the proxy, DataSampling and QC workflows:
```
o2-qc-run-readout | o2-qc --config json://${QUALITYCONTROL_ROOT}/etc/readout.json
```

The data sampling is configured to sample 1% of the data as the readout should run by default at full speed.

### Getting real data from readout

See [these instructions for readout](ModulesDevelopment.md#readout) and [these for O2 utilities](ModulesDevelopment.md#dpl-workflow).

### Readout data format as received by the Task

The header is an O2 header populated with data from the header built by the Readout.
The payload received is a 2MB (configurable) data page made of CRU pages (8kB).

__Configuration file__

The configuration file is installed in `$QUALITYCONTROL_ROOT/etc`. Each time you rebuild the code, `$QUALITYCONTROL_ROOT/etc/readout.json` is overwritten by the file in the source directory (`~/alice/QualityControl/Framework/readout.json`).
To avoid this behaviour and preserve the changes you do to the configuration, you can copy the file and specify the path to it with the parameter `--config` when launch `o2-qc`.

To change the fraction of the data being monitored, change the option `fraction`.

```
"fraction": "0.01",
```

---

[← Go back to Post-processing](PostProcessing.md) | [↑ Go to the Table of Content ↑](../README.md) | [Continue to Configuration →](Configuration.md)
