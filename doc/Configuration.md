
Configuration reference
---

<!--TOC generated with https://github.com/ekalinin/github-markdown-toc-->
<!--./gh-md-toc --insert --no-backup --hide-footer QualityControl/doc/Configuration.md -->
<!--ts-->
   * [Global configuration structure](#global-configuration-structure)
   * [Common configuration](#common-configuration)
   * [QC Tasks configuration](#qc-tasks-configuration)
   * [QC Checks configuration](#qc-checks-configuration)
   * [QC Aggregators configuration](#qc-aggregators-configuration)
   * [QC Post-processing configuration](#qc-post-processing-configuration)
   * [External tasks configuration](#external-tasks-configuration)
   * [Merging multiple configuration files into one](#merging-multiple-configuration-files-into-one)
   * [Templating config files](#templating-config-files)
   * [Definition and access of simple user-defined task configuration ("taskParameters")](#definition-and-access-of-simple-user-defined-task-configuration-taskparameters)
   * [Definition and access of user-defined configuration ("extendedTaskParameters")](#definition-and-access-of-user-defined-configuration-extendedtaskparameters)
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
      "Activity": {                       "": ["Configuration of a QC Activity (Run). DO NOT USE IN PRODUCTION! " ],
        "number": "42",                   "": "Activity number. ",
        "type": "PHYSICS",                "": "Activity type.",
        "periodName": "",                 "": "Period name - e.g. LHC22c, LHC22c1b_test",
        "passName": "",                   "": "Pass type - e.g. spass, cpass1",
        "provenance": "qc",               "": "Provenance - qc or qc_mc depending whether it is normal data or monte carlo data",
        "start" : "0",                    "": "Activity start time in ms since epoch. One can use it as a filter in post-processing",
        "end" : "1234",                   "": "Activity end time in ms since epoch. One can use it as a filter in post-processing", 
        "beamType" : "pp",                "": "Beam type: `pp`, `PbPb`, `pPb` ", 
        "partitionName" : "",             "": "Partition name", 
        "fillNumber" : "123",             "": "Fill Number",
        "originalRunNumber" : "100",      "": "When it is a REPLAY run, this contains the original run number"
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
        "cycleDurationSeconds": "60",       "": "Cycle duration (how often objects are published), 10 seconds minimum.",
                                            "": "The first cycle will be randomly shorter. ",
        "": "Alternatively, one can specify different cycle durations for different periods. The last item in cycleDurations will be used for the rest of the duration whatever the period. The first cycle will be randomly shorter.",
        "cycleDurations": [
          {"cycleDurationSeconds": 60, "validitySeconds": 35},
          {"cycleDurationSeconds": 120, "validitySeconds": 1}
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

## Merging multiple configuration files into one

To merge multiple QC configuration files into one, one can use `jq` in the following way:

```
jq -n 'reduce inputs as $s (input; .qc.tasks += ($s.qc.tasks) | .qc.checks += ($s.qc.checks)  | .qc.externalTasks += ($s.qc.externalTasks) | .qc.postprocessing += ($s.qc.postprocessing)| .dataSamplingPolicies += ($s.dataSamplingPolicies))' $QC_JSON_GLOBAL $JSON_FILES > $MERGED_JSON_FILENAME
```

However, one should pay attention to avoid duplicate task definition keys (e.g. having RawTask twice, each for a different detector), otherwise only one of them would find its way to a merged file.
In such case, one can add the `taskName` parameter in the body of a task configuration structure to use the preferred name and change the root key to a unique id, which shall be used only for the purpose of navigating a configuration file.
If `taskName` does not exist, it is taken from the root key value.
Please remember to update also the references to the task in other actors which refer it (e.g. in Check's data source).

These two tasks will **not** be merged correctly:

```json
      "RawTask": {
        "className": "o2::quality_control_modules::abc::RawTask",
        "moduleName": "QcA",
        "detectorName": "A",
        "dataSource": {
          "type": "dataSamplingPolicy",
          "name": "raw-a"
        }
      }
```

```json
      "RawTask": {
        "className": "o2::quality_control_modules::xyz::RawTask",
        "moduleName": "QcB",
        "detectorName": "B",
        "dataSource": {
          "type": "dataSamplingPolicy",
          "name": "raw-b"
        }
      }
```

The following tasks will be merged correctly:

```json
      "RawTaskA": {
        "taskName": "RawTask",
        "className": "o2::quality_control_modules::abc::RawTask",
        "moduleName": "QcA",
        "detectorName": "A",
        "dataSource": {
          "type": "dataSamplingPolicy",
          "name": "raw-a"
        }
      }
```

```json
      "RawTaskB": {
        "taskName": "RawTask"
        "className": "o2::quality_control_modules::xyz::RawTask",
        "moduleName": "QcB",
        "detectorName": "B",
        "dataSource": {
          "type": "dataSamplingPolicy",
          "name": "raw-b"
        }
      }
```

The same approach can be applied to other actors in the QC framework, like Checks (`checkName`), Aggregators(`aggregatorName`), External Tasks (`taskName`) and Postprocessing Tasks (`taskName`).

## Templating config files

> [!WARNING]  
> Templating only works when using aliECS, i.e. in production and staging.

The templating is provided by a template engine called `jinja`. You can use any of its feature. A couple are described below and should satisfy the vast majority of the needs.

### Preparation

> [!IMPORTANT]
> Workflows have already been migrated to apricot. This should not be needed anymore.

To template a config file, modify the corresponding workflow in `ControlWorkflows`. This is needed because we won't use directly `Consul`  but instead go through `apricot` to template it.

1. Replace `consul-json` by `apricot`
2. Replace `consul_endpoint` by `apricot_endpoint`
3. Make sure to have single quotes around the URI

Example:

```
o2-qc --config consul-json://{{ consul_endpoint }}/o2/components/qc/ANY/any/mch-qcmn-epn-full-track-matching --remote -b
```

becomes

```
o2-qc --config 'apricot://{{ apricot_endpoint }}/o2/components/qc/ANY/any/mch-qcmn-epn-full-track-matching' --remote -b
```

Make sure that you are able to run with the new workflow before actually templating.

### Include a config file

To include a config file (e.g. named `mch_digits`) add this line :

```
{% include "MCH/mch_digits" %}
```

The content of the file `mch_digits` is then copied into the config file. Thus make sure that you include all the commas and stuff.

#### Configuration files organisation

Once you start including files, you must put the included files inside the corresponding detector subfolder (that have already been created for you).

Common config files includes are provided in the `COMMON` subfolder.

### Conditionals

The `if` looks like

```
{% if [condition] %} …  {% endif %}
```

The condition probably requires some external info, such as the run type or a detectors list. Thus you must pass the info in the ControlWorkflows.

It could look like this

```
o2-qc --config 'apricot://{{ apricot_endpoint }}/o2/components/qc/ANY/any/tpc-pulser-calib-qcmn?run_type={{ run_type }}' ...
```

or

```
o2-qc --config 'apricot://{{ apricot_endpoint }}/o2/components/qc/ANY/any/mch-qcmn-epn-full-track-matching?detectors={{ detectors }}' ...
```

Then use it like this:

```
{% if run_type == "PHYSICS" %}
...
{% endif %}
```

or like this respectively:

```
{% if "mch" in detectors|lower %}
...
{% endif %}
```

### Test and debug

To see how a config file will look like once templated, simply open a browser at this address: `{{apricot_endpoint}}/components/qc/ANY/any/tpc-pulser-calib-qcmn?process=true`
Replace `{{apricot_endpoint}}` by the value you can find in Consul under `o2/runtime/aliecs/vars/apricot_endpoint` (it is different on staging and prod).
_Note that there is no `o2` in the path!!!_

### Example

We are going to create in staging a small example to demonstrate the above.
First create 2 files if they don't exist yet:

**o2/components/qc/ANY/any/templating_demo**

```
{
  "qc": {
    "config": {% include "TST/templating_included" %}
    {% if run_type == "PHYSICS" %} ,"aggregators": "included"  {% endif %}
  }
}
```

Here we simply include 1 file from a subfolder and add a piece if a certain condition is successful.

**o2/components/qc/ANY/any/TST/templating_included**

```
{
  bookkeeping": {
    "url": "alio2-cr1-hv-web01.cern.ch:4001"
  }
}
```

And now you can try it out:

```
http://alio2-cr1-hv-mvs00.cern.ch:32188/components/qc/ANY/any/templating_demo?process=true
```

--> the file is included inside the other.

```
[http://alio2-cr1-hv-mvs00.cern.ch:32188/components/qc/ANY/any/templating_demo?process=true](http://alio2-cr1-hv-mvs00.cern.ch:32188/components/qc/ANY/any/templating_demo?process=true&run_type=PHYSICS)
```

--> the file is included and the condition is true thus we have an extra line.

## Definition and access of user-defined task configuration

###  Simple and limited approach ("taskParameters")

The new, extended, way of defining such parameters, not only in Tasks but also in Checks, Aggregators and PP tasks,
is described in the next section.

A task can access custom parameters declared in the configuration file at `qc.tasks.<task_id>.taskParameters`. They are stored inside an object of type `CustomParameters` named `mCustomParameters`, which is a protected member of `TaskInterface`.

The syntax is

```json
    "tasks": {
      "QcTask": {
        "taskParameters": {
          "myOwnKey1": "myOwnValue1"
        },
```

It is accessed with : `mCustomParameters["myOwnKey"]`.

### Advanced approach with beam and run type parametrization ("extendedTaskParameters")

User code, whether it is a Task, a Check, an Aggregator or a PostProcessing task, can access custom parameters declared in the configuration file.
They are stored inside an object of type `CustomParameters` named `mCustomParameters`, which is a protected member of `TaskInterface`.

The following table gives the path in the config file and the name of the configuration parameter for the various types of user code:

| User code      | Config File item                                       |
|----------------|--------------------------------------------------------|
| Task           | `qc.tasks.<task_id>.extendedTaskParameters`            |
| Check          | `qc.checks.<check_id>.extendedCheckParameters`         |
| Aggregator     | `qc.aggregators.<agg_id>.extendedAggregatorParameters` |
| PostProcessing | `qc.postprocessing.<pp_id>.extendedTaskParameters`     |

The new syntax is

```json
    "tasks": {
      "QcTask": {
        "extendedTaskParameters": {
          "default": {
            "default": {
              "myOwnKey1": "myOwnValue1",
              "myOwnKey2": "myOwnValue2",
              "myOwnKey3": "myOwnValue3"
            }
          },
          "PHYSICS": {
            "default": {
              "myOwnKey1": "myOwnValue1b",
              "myOwnKey2": "myOwnValue2b"
            },
            "pp": {
              "myOwnKey1": "myOwnValue1c"
            },
            "PbPb": {
              "myOwnKey1": "myOwnValue1d"
            }
          },
          "COSMICS": {
            "myOwnKey1": "myOwnValue1e",
            "myOwnKey2": "myOwnValue2e"
          }
        },
```

It allows to have variations of the parameters depending on the run and beam types. The proper run types can be found here: [ECSDataAdapters.h](https://github.com/AliceO2Group/AliceO2/blob/dev/DataFormats/Parameters/include/DataFormatsParameters/ECSDataAdapters.h#L54). The `default` can be used
to ignore the run or the beam type.
The beam type comes from the parameter `pdp_beam_type` set by ECS and can be one of the following: `pp`, `PbPb`, `pPb`, `pO`, `OO`, `NeNe`, `cosmic`, `technical`.
See `[readout-dataflow](https://github.com/AliceO2Group/ControlWorkflows/blob/master/workflows/readout-dataflow.yaml)` to verify the possible values.

The values can be accessed in various ways described in the following sub-sections.

### Access optional values with or without activity

The value for the key, runType and beamType is returned if found, or an empty value otherwise.
However, before returning an empty value we try to substitute the runType and the beamType with "default".

```c++
// returns an Optional<string> if it finds the key `myOwnKey` for the runType and beamType of the provided activity, 
// or if it can find the key with the runType or beamType substituted with "default". 
auto param = mCustomParameters.atOptional("myOwnKey1", activity); // activity is "PHYSICS", "PbPb" , returns "myOwnValue1d"
// same but passing directly the run and beam types
auto param = mCustomParameters.atOptional("myOwnKey1", "PHYSICS", "PbPb"); // returns "myOwnValue1d"
// or with only the run type
auto param = mCustomParameters.atOptional("myOwnKey1", "PHYSICS"); // returns "myOwnValue1b"
```

### Access values directly specifying the run and beam type or an activity

The value for the key, runType and beamType is returned if found, or an exception is thrown otherwise..
However, before throwing we try to substitute the runType and the beamType with "default".

```c++
mCustomParameters["myOwnKey"]; // considering that run and beam type are `default` --> returns `myOwnValue`
mCustomParameters.at("myOwnKey"); // returns `myOwnValue`
mCustomParameters.at("myOwnKey", "default"); // returns `myOwnValue`
mCustomParameters.at("myOwnKey", "default", "default"); // returns `myOwnValue`

mCustomParameters.at("myOwnKey1", "PHYSICS", "pp"); // returns `myOwnValue1c`
mCustomParameters.at("myOwnKey1", "PHYSICS", "PbPb"); // returns `myOwnValue1d`
mCustomParameters.at("myOwnKey2", "COSMICS"); // returns `myOwnValue2e`

mCustomParameters.at("myOwnKey1", activity); // result will depend on activity
```

### Access values and return default if not found

The correct way of accessing a parameter and to default to a value if it is not there, is the following:

```c++
  std::string param = mCustomParameters.atOrDefaultValue("myOwnKey1", "1" /*default value*/, "physics", "pp");
  int casted = std::stoi(param);

  // alternatively
  std::string param = mCustomParameters.atOrDefaultValue("myOwnKey1", "1" /*default value*/, activity); // see below how to get the activity
```

### Find a value

Finally the way to search for a value and only act if it is there is the following:

```c++
  if (auto param2 = mCustomParameters.find("myOwnKey1", "physics", "pp"); param2 != cp.end()) {
    int casted = std::stoi(param);
  }
```

### Retrieve the activity in the modules

In a task, the `activity` is provided in `startOfActivity`.

In a Check, it is returned by `getActivity()`.

In an Aggregator, it is returned by `getActivity()`.

In a postprocessing task, it is available in the objects manager: `getObjectsManager()->getActivity()`

TODO we miss the definition of the datasampling policies


---

[← Go back to Post-Processing](PostProcessing.md) | [↑ Go to the Table of Content ↑](../README.md) | [Continue to QCDB →](QCDB.md)
