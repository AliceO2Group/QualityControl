
Configuration reference
---

<!--TOC generated with https://github.com/ekalinin/github-markdown-toc-->
<!--./gh-md-toc --insert --no-backup --hide-footer QualityControl/doc/Configuration.md -->
<!--ts-->
   * [Global configuration structure](./QualityControl/doc/Configuration.md#global-configuration-structure)
   * [Common configuration](./QualityControl/doc/Configuration.md#common-configuration)
   * [QC Tasks configuration](./QualityControl/doc/Configuration.md#qc-tasks-configuration)
   * [QC Checks configuration](./QualityControl/doc/Configuration.md#qc-checks-configuration)
   * [QC Aggregators configuration](./QualityControl/doc/Configuration.md#qc-aggregators-configuration)
   * [QC Post-processing configuration](./QualityControl/doc/Configuration.md#qc-post-processing-configuration)
   * [External tasks configuration](./QualityControl/doc/Configuration.md#external-tasks-configuration)
<!--te-->


The QC requires a number of configuration items. An example config file is
provided in the repo under the name _example-default.json_. This is a quick reference for all the parameters.

## Global configuration structure

This is the global structure of the configuration in QC.

```json
{
  "qc": {
    "config": {

    },
    "tasks": {
      
    },
    "externalTasks": {
    
    },
    "checks": {
      
    },
    "aggregators": {

    },
    "postprocessing": {
      
    }
  },
  "dataSamplingPolicies": [

  ]
}
```

There are six QC-related components:
* "config" - contains global configuration of QC which apply to any component. It is required in any configuration
  file.
* "tasks" - contains declarations of QC Tasks. It is mandatory for running topologies with Tasks and
  Checks.
* "externalTasks" - contains declarations of external devices which sends objects to the QC to be checked and stored.
* "checks" - contains declarations of QC Checks. It is mandatory for running topologies with
  Tasks and Checks.
* "aggregators" - contains declarations of QC Aggregators. It is not mandatory.
* "postprocessing" - contains declarations of PostProcessing Tasks. It is only needed only when Post-Processing is
  run.

The configuration file can also include a list of Data Sampling Policies.
Please refer to the [Data Sampling documentation](https://github.com/AliceO2Group/AliceO2/tree/dev/Utilities/DataSampling) to find more information.

## Common configuration

This is how a typical "config" structure looks like. Each configuration element is described with a relevant comment
afterwards. The `"": "<comment>",` formatting is to keep the JSON structure valid. Please note that these comments
should not be present in real configuration files.

```json
{
  "qc": {
    "config": {
      "database": {                       "": "Configuration of a QC database (the place where QC results are stored).",
        "username": "qc_user",            "": "Username to log into a DB. Relevant only to the MySQL implementation.",
        "password": "qc_user",            "": "Password to log into a DB. Relevant only to the MySQL implementation.",
        "name": "quality_control",        "": "Name of a DB. Relevant only to the MySQL implementation.",
        "implementation": "CCDB",         "": "Implementation of a DB. It can be CCDB, or MySQL (deprecated).",
        "host": "ccdb-test.cern.ch:8080", "": "URL of a DB.",
        "maxObjectSize": "2097152",       "": "[Bytes, default=2MB] Maximum size allowed, larger objects are rejected."
      },
      "Activity": {                       "": ["Configuration of a QC Activity (Run). This structure is subject to",
                                               "change or the values might come from other source (e.g. ECS+Bookkeeping)." ],
        "number": "42",                   "": "Activity number.",
        "type": "PHYSICS",                "": "Activity type.",
        "periodName": "",                 "": "Period name - e.g. LHC22c, LHC22c1b_test",
        "passName": "",                   "": "Pass type - e.g. spass, cpass1",
        "provenance": "qc",               "": "Provenance - qc or qc_mc depending whether it is normal data or monte carlo data",
        "start" : "0",                    "": "Activity start time in ms since epoch. One can use it as a filter in post-processing",
        "end" : "1234",                   "": "Activity end time in ms since epoch. One can use it as a filter in post-processing", 
        "beamType" : "pp",                "": "Beam type: `pp`, `PbPb`, `pPb` ", 
        "partitionName" : "",             "": "Partition name", 
        "fillNumber" : "123",             "": "Fill Number"
      },
      "monitoring": {                     "": "Configuration of the Monitoring library.",
        "url": "infologger:///debug?qc",  "": ["URI to the Monitoring backend. Refer to the link below for more info:",
                                               "https://github.com/AliceO2Group/Monitoring#monitoring-instance"]
      },
      "consul": {                         "": "Configuration of the Consul library (used for Service Discovery).",
        "url": "",                        "": "URL of the Consul backend"
      },
      "conditionDB": {                    "": ["Configuration of the Conditions and Calibration DataBase (CCDB).",
                                               "Do not mistake with the CCDB which is used as QC repository."],
        "url": "ccdb-test.cern.ch:8080",  "": "URL of a CCDB"
      },
      "infologger": {                     "": "Configuration of the Infologger (optional).",
        "filterDiscardDebug": "false",    "": "Set to 1 to discard debug and trace messages (default: false)",
        "filterDiscardLevel": "2",        "": "Message at this level or above are discarded (default: 21 - Trace)",
        "filterDiscardFile": "",          "": ["If set, the discarded messages will go to this file (default: <none>)",
                                              "The keyword _ID_, if used, is replaced by the device ID."],
        "filterRotateMaxBytes": "",       "": "Maximum size of the discard file.", 
        "filterRotateMaxFiles": "",       "": "Maximum number of discard files.",
        "debugInDiscardFile": "false",    "": "If true, the debug discarded messages go to the file (default: false)."
      },
      "bookkeeping": {                    "": "Configuration of the bookkeeping (optional)",
        "url": "localhost:4001",          "": "Url of the bookkeeping API (port is usually different from web interface)"
      },
      "kafka": {
        "url": "kafka-broker:123",        "": "url of the kafka broker",
        "topicAliecsRun":"aliecs.run",    "": "the topic where AliECS publishes Run Events, 'aliecs.run' by default"
      },
      "postprocessing": {                 "": "Configuration parameters for post-processing",
        "periodSeconds": 10.0,            "": "Sets the interval of checking all the triggers. One can put a very small value",
                                          "": "for async processing, but use 10 or more seconds for synchronous operations",
        "matchAnyRunNumber": "false",     "": "Forces post-processing triggers to match any run, useful when running with AliECS"
      }
    }
  }
}
```

### Common configuration in production

In production at P2 and in staging, some common items are defined globally in the file `QC/general-config-params`:

* QCDB
* monitoring
* consul
* conditionDB
* bookkeeping
*

It is mandatory to use them by including the file:

```
"config": {
            {% include "QC/general-config-params" %}
      },
```

Other configuration items can still be added in your files as such (note the comma after the inclusion) :

```
      "config": {
            {% include "QC/general-config-params" %},
            "infologger": {
                "filterDiscardDebug": "false",
                "filterDiscardLevel": "22"
            },
            "postprocessing": {
                "matchAnyRunNumber": "true"
            }
      },
```

Please, do not use `Activity` in production !

## QC Tasks configuration

Below the full QC Task configuration structure is described. Note that more than one task might be declared inside in
the "tasks" path.

 ```json
{
  "qc": {
    "tasks": {
      "QcTaskID": {                         "": ["ID of the QC Task. Less than 14 character names are preferred.",
                                                 "If \"taskName\" is empty or missing, the ID is used"],
        "active": "true",                   "": "Activation flag. If not \"true\", the Task will not be created.",
        "taskName": "MyTaskName",           "": ["Name of the task, used e.g. in the QCDB. If empty, the ID is used.",
                                                 "Less than 14 character names are preferred."],
        "className": "namespace::of::Task", "": "Class name of the QC Task with full namespace.",
        "moduleName": "QcSkeleton",         "": "Library name. It can be found in CMakeLists of the detector module.",
        "detectorName": "TST",              "": "3-letter code of the detector.",
        "critical": "true",                "": "if false the task is allowed to die without stopping the workflow, default: true",
        "cycleDurationSeconds": "10",       "": "Cycle duration (how often objects are published), 10 seconds minimum.",
                                            "": "The first cycle will be randomly shorter. ",
        "": "Alternatively, one can specify different cycle durations for different periods. The last item in cycleDurations will be used for the rest of the duration whatever the period. The first cycle will be randomly shorter.",
        "cycleDurations": [
          {"cycleDurationSeconds": 10, "validitySeconds": 35},
          {"cycleDurationSeconds": 12, "validitySeconds": 1}
        ],
        "maxNumberCycles": "-1",            "": "Number of cycles to perform. Use -1 for infinite.",
        "disableLastCycle": "true",         "": "Last cycle, upon EndOfStream, is not published. (default: false)",
        "dataSources": [{                   "": "Data sources of the QC Task. The following are supported",
          "type": "dataSamplingPolicy",     "": "Type of the data source",
          "name": "tst-raw",                "": "Name of Data Sampling Policy"
        }, {
          "type": "direct",                 "": "connects directly to another output",
          "query": "raw:TST/RAWDATA/0",     "": "input spec query, as expected by DataDescriptorQueryBuilder"
        }],
        "taskParameters": {                 "": "User Task parameters which are then accessible as a key-value map.",
          "myOwnKey": "myOwnValue",         "": "An example of a key and a value. Nested structures are not supported"
        },
        "resetAfterCycles" : "0",           "": "Makes the Task or Merger reset MOs each n cycles.",
                                            "": "0 (default) means that MOs should cover the full run.",
        "location": "local",                "": ["Location of the QC Task, it can be local or remote. Needed only for",
                                                 "multi-node setups, not respected in standalone development setups."],
        "localMachines": [                  "", "List of local machines where the QC task should run. Required only",
                                            "", "for multi-node setups. An alias can be used if merging deltas.",
          "o2flp1",                         "", "Hostname of a local machine.",
          "o2flp2",                         "", "Hostname of a local machine."
        ],
        "remoteMachine": "o2qc1",           "": "Remote QC machine hostname. Required only for multi-node setups with EPNs.",
        "remotePort": "30432",              "": "Remote QC machine TCP port. Required only for multi-node setups with EPNs.",
        "localControl": "aliecs",           "": ["Control software specification, \"aliecs\" (default) or \"odc\").",
                                                 "Needed only for multi-node setups."],
        "mergingMode": "delta",             "": "Merging mode, \"delta\" (default) or \"entire\" objects are expected",
        "mergerCycleMultiplier": "1",       "": "Multiplies the Merger cycle duration with respect to the QC Task cycle"
        "mergersPerLayer": [ "3", "1" ],    "": "Defines the number of Mergers per layer, the default is [\"1\"]",
        "grpGeomRequest" : {                "": "Requests to retrieve GRP objects, then available in GRPGeomHelper::instance()",
          "geomRequest": "None",            "": "Available options are \"None\", \"Aligned\", \"Ideal\", \"Alignements\"",
          "askGRPECS": "false",
          "askGRPLHCIF": "false",
          "askGRPMagField": "false",
          "askMatLUT": "false",
          "askTime": "false",
          "askOnceAllButField": "false",
          "needPropagatorD":  "false"
        },
        "globalTrackingDataRequest": {         "": "A helper to add tracks or clusters to inputs of the task",
          "canProcessTracks" : "ITS,ITS-TPC",  "": "tracks that the QC task can process, usually should not change",
          "requestTracks" : "ITS,TPC-TRD",     "": ["tracks that the QC task should process, TPC-TRD will not be",
                                                    "requested, as it is not listed in the previous parameter"],
          "canProcessClusters" : "TPC",        "": "clusters that the QC task can process",
          "requestClusters" : "TPC",           "": "clusters that the QC task should process",
          "mc" : "false",                      "": "mc boolean flag for the data request"
        }
      }
    }
  }
}
```

## QC Checks configuration

Below the full QC Checks configuration structure is described. Note that more than one check might be declared inside in
the "checks" path. Please also refer to [the Checks documentation](ModulesDevelopment.md#configuration) for more details.

 ```json
{
  "qc": {
    "checks": {
      "MeanIsAbove": {                "": ["ID of the Check. Less than 12 character names are preferred.",
                                           "If \"checkName\" is empty or missing, the ID is used"],
        "active": "true",             "": "Activation flag. If not \"true\", the Check will not be run.",
        "checkName": "MeanIsAbove",   "": ["Name of the check, used e.g. in the QCDB. If empty, the ID is used.",
                                           "Less than 12 character names are preferred."],
        "className": "ns::of::Check", "": "Class name of the QC Check with full namespace.",
        "moduleName": "QcCommon",     "": "Library name. It can be found in CMakeLists of the detector module.",
        "detectorName": "TST",        "": "3-letter code of the detector.",
        "policy": "OnAny",            "": ["Policy which determines when MOs should be checked. See the documentation",
                                           "of Checks for the list of available policies and their behaviour."],
        "exportToBookkeeping": "true","": "Flag that toggles reporting of QcFlags created by this check into BKP via gRPC.",
                                           "When not presented it equals to "false""
        "dataSource": [{              "": "List of data source of the Check.",
          "type": "Task",             "": "Type of the data source, \"Task\", \"ExternalTask\" or \"PostProcessing\"", 
          "name": "myTask_1",         "": "Name of the Task",
          "MOs": [ "example" ],       "": ["List of MOs to be checked. "
                                            "Can be omitted to mean \"all\"."]
        }],
        "checkParameters": {          "": "User Check parameters which are then accessible as a key-value map.",
          "myOwnKey": "myOwnValue",   "": "An example of a key and a value. Nested structures are not supported"
        }
      }
    }
  }
}
```

## QC Aggregators configuration

Below the full QC Aggregators configuration structure is described. Note that more than one aggregator might be declared inside in
the "aggregators" path. Please also refer to [the Aggregators documentation](ModulesDevelopment.md#quality-aggregation) for more details.

```json
{
  "qc": {
    "aggregators": {
      "MyAggregator1": {                    "": "ID of the Aggregator. Less than 12 character names are preferred.",
        "active": "true",                   "": "Activation flag. If not \"true\", the Aggregator will not be run.",
        "aggregatorName" : "MyAggregator1", "": ["Name of the Aggregator, used e.g. in the QCDB. If empty, the ID is used.",
                                                 "Less than 12 character names are preferred."],
        "className": "ns::of::Aggregator",  "": "Class name of the QC Aggregator with full namespace.",
        "moduleName": "QcCommon",           "": "Library name. It can be found in CMakeLists of the detector module.",
        "policy": "OnAny",                  "": ["Policy which determines when QOs should be aggregated. See the documentation",
                                                "of Aggregators for the list of available policies and their behaviour."],
        "exportToBookkeeping": "true",      "": "Flag that toggles reporting of QcFlags created by all of the sources into 
                                                "the BKP via gRPC. "When not presented it equals to "false""
        "detectorName": "TST",              "": "3-letter code of the detector.",
        "dataSource": [{                    "": "List of data source of the Aggregator.",
          "type": "Check",,                 "": "Type of the data source: \"Check\" or \"Aggregator\"", 
          "name": "dataSizeCheck",          "": "Name of the Check or Aggregator",
          "QOs": ["newQuality", "another"], "": ["List of QOs to be checked.",
                                          "Can be omitted for Checks", 
                                          "that publish a single Quality or to mean \"all\"."]
        }]
      }
    }
  }
}
```

## QC Post-processing configuration

Below the full QC Post-processing (PP) configuration structure is described. Note that more than one PP Task might be
declared inside in the "postprocessing" path. Please also refer to [the Post-processing documentation](PostProcessing.md) for more details.

```json
{
  "qc": {
    "postprocessing": {
      "ExamplePostprocessingID": {            "": "ID of the PP Task.",
        "active": "true",                     "": "Activation flag. If not \"true\", the PP Task will not be run.",
        "taskName": "MyPPTaskName",           "": ["Name of the task, used e.g. in the QCDB. If empty, the ID is used.",
                                                 "Less than 14 character names are preferred."],
        "className": "namespace::of::PPTask", "": "Class name of the PP Task with full namespace.",
        "moduleName": "QcSkeleton",           "": "Library name. It can be found in CMakeLists of the detector module.",
        "detectorName": "TST",                "": "3-letter code of the detector.",
        "initTrigger": [                      "", "List of initialization triggers",
          "userorcontrol",                    "", "An example of an init trigger"
        ],
        "updateTrigger": [                    "", "List of update triggers",
          "10min",                            "", "An example of an update trigger"
        ],
        "stopTrigger": [                      "", "List of stop triggers",
          "userorcontrol",                    "", "An example of a stop trigger"
        ],
        "validityFromLastTriggerOnly": "false",  "": "If true, the output objects will use validity of the last trigger,",
                                                 "": "otherwise a union of all triggers' validity is used by default.",
        "sourceRepo": {                          "": "It allows to specify a different repository for the input objects.",
          "implementation": "CCDB",
          "host": "another-test.cern.ch:8080"
        }
      }
    }
  }
}
```

## External tasks configuration

Below the external task configuration structure is described. Note that more than one external task might be declared inside in the "externalTasks" path.

```json
{
  "qc": {
    "externalTasks": {
      "External-1": {                       "": "ID of the task",
        "active": "true",                   "": "Activation flag. If not \"true\", the Task will not be created.",
        "taskName": "External-1",           "": "Name of the task, used e.g. in the QCDB. If empty, the ID is used.",
        "query": "External-1:TST/HISTO/0",  "": "Query specifying where the objects to be checked and stored are coming from. Use the task name as binding."
      }
    }
  }
}
```
---

[← Go back to Advanced](Advanced.md) | [↑ Go to the Table of Content ↑](../README.md) | [Continue to Frequently Asked Questions →](FAQ.md)
