<!--  \cond EXCLUDE_FOR_DOXYGEN -->
[![JIRA](https://img.shields.io/badge/JIRA-Report%20issue-blue.svg)](https://alice.its.cern.ch/jira/secure/CreateIssue.jspa?pid=11201&issuetype=1)
[![doxygen](https://img.shields.io/badge/doxygen-documentation-blue.svg)](https://aliceo2group.github.io/QualityControl/)
[![Discourse](https://img.shields.io/badge/discourse-Get%20help-blue.svg)](https://alice-talk.web.cern.ch/)
[![FLP doc](https://img.shields.io/badge/FLP-documentation-blue.svg)](https://alice-flp.docs.cern.ch/)

<!--  \endcond  --> 

This is the repository of the data quality control (QC) software for the ALICE O<sup>2</sup> system. 
 
For a general overview of our (O2) software, organization and processes, please see this [page](https://aliceo2group.github.io/).

* [QuickStart](doc/QuickStart.md)
    * [Read this first!](doc/QuickStart.md#read-this-first)
    * [Requirements](doc/QuickStart.md#requirements)
    * [Setup](doc/QuickStart.md#setup)
        * [Environment loading](doc/QuickStart.md#environment-loading)
    * [Execution](doc/QuickStart.md#execution)
        * [Basic workflow](doc/QuickStart.md#basic-workflow)
        * [Readout chain (optional)](doc/QuickStart.md#readout-chain-optional)
        * [Post-processing example](doc/QuickStart.md#post-processing-example)
* [Modules development](doc/ModulesDevelopment.md)
    * [Context](doc/ModulesDevelopment.md#context)
        * [QC architecture](doc/ModulesDevelopment.md#qc-architecture)
        * [DPL](doc/ModulesDevelopment.md#dpl)
        * [Data Sampling](doc/ModulesDevelopment.md#data-sampling)
            * [Custom Data Sampling Condition](doc/ModulesDevelopment.md#custom-data-sampling-condition)
            * [Bypassing the Data Sampling](doc/ModulesDevelopment.md#bypassing-the-data-sampling)
        * [Code Organization](doc/ModulesDevelopment.md#code-organization)
        * [Developing with aliBuild/alienv](doc/ModulesDevelopment.md#developing-with-alibuildalienv)
        * [User-defined modules](doc/ModulesDevelopment.md#user-defined-modules)
        * [Repository](doc/ModulesDevelopment.md#repository)
            * [Paths](doc/ModulesDevelopment.md#paths)
    * [Module creation](doc/ModulesDevelopment.md#module-creation)
    * [Test run](doc/ModulesDevelopment.md#test-run)
        * [Saving the QC objects in a local file](doc/ModulesDevelopment.md#saving-the-qc-objects-in-a-local-file)
    * [Modification of the Task](doc/ModulesDevelopment.md#modification-of-the-task)
    * [Check](doc/ModulesDevelopment.md#check)
        * [Configuration](doc/ModulesDevelopment.md#configuration)
        * [Implementation](doc/ModulesDevelopment.md#implementation)
        * [Results](doc/ModulesDevelopment.md#results)
    * [Quality Aggregation](doc/ModulesDevelopment.md#quality-aggregation)
        * [Quick try](doc/ModulesDevelopment.md#quick-try)
        * [Configuration](doc/ModulesDevelopment.md#configuration-1)
        * [Implementation](doc/ModulesDevelopment.md#implementation-1)
    * [Naming convention](doc/ModulesDevelopment.md#naming-convention)
    * [Committing code](doc/ModulesDevelopment.md#committing-code)
    * [Data sources](doc/ModulesDevelopment.md#data-sources)
        * [Readout](doc/ModulesDevelopment.md#readout)
        * [DPL workflow](doc/ModulesDevelopment.md#dpl-workflow)
    * [Run number and other run attributes (period, pass type, provenance)](doc/ModulesDevelopment.md#run-number-and-other-run-attributes-period-pass-type-provenance)
    * [A more advanced example](doc/ModulesDevelopment.md#a-more-advanced-example)
    * [Monitoring](doc/ModulesDevelopment.md#monitoring)
* [Post-processing](doc/PostProcessing.md)
    * [The post-processing framework](doc/PostProcessing.md#the-post-processing-framework)
        * [Post-processing interface](doc/PostProcessing.md#post-processing-interface)
        * [Configuration](doc/PostProcessing.md#configuration)
    * [Definition and access of user-specific configuration](doc/PostProcessing.md#definition-and-access-of-user-specific-configuration)
        * [Running it](doc/PostProcessing.md#running-it)
    * [Convenience classes](doc/PostProcessing.md#convenience-classes)
        * [The TrendingTask class](doc/PostProcessing.md#the-trendingtask-class)
        * [The SliceTrendingTask class](doc/PostProcessing.md#the-slicetrendingtask-class)
        * [The QualityTask class](doc/PostProcessing.md#the-qualitytask-class)
        * [The FlagCollectionTask class](doc/PostProcessing.md#the-flagcollectiontask-class)
        * [The BigScreen class](doc/PostProcessing.md#the-bigscreen-class)
    * [More examples](doc/PostProcessing.md#more-examples)
* [Advanced topics](doc/Advanced.md)
  * [Framework](doc/Advanced.md#framework)
      * [Plugging the QC to an existing DPL workflow](doc/Advanced.md#plugging-the-qc-to-an-existing-dpl-workflow)
      * [Production of QC objects outside this framework](doc/Advanced.md#production-of-qc-objects-outside-this-framework)
      * [Multi-node setups](doc/Advanced.md#multi-node-setups)
      * [Batch processing](doc/Advanced.md#batch-processing)
      * [Moving window](doc/Advanced.md#moving-window)
      * [Monitor cycles](doc/Advanced.md#monitor-cycles)
      * [Writing a DPL data producer](doc/Advanced.md#writing-a-dpl-data-producer)
      * [Custom merging](doc/Advanced.md#custom-merging)
      * [Critical, resilient and non-critical tasks](doc/Advanced.md#critical-resilient-and-non-critical-tasks)
      * [QC with DPL Analysis](doc/Advanced.md#qc-with-dpl-analysis)
          * [Uploading objects to QCDB](doc/Advanced.md#uploading-objects-to-qcdb)
      * [Propagating Check results to RCT in Bookkeeping](doc/Advanced.md#propagating-check-results-to-rct-in-bookkeeping)
          * [Conversion details](doc/Advanced.md#conversion-details)
  * [Solving performance issues](doc/Advanced.md#solving-performance-issues)
      * [Dispatcher](doc/Advanced.md#dispatcher)
      * [QC Tasks](doc/Advanced.md#qc-tasks-1)
      * [Mergers](doc/Advanced.md#mergers)
  * [Understanding and reducing memory footprint](doc/Advanced.md#understanding-and-reducing-memory-footprint)
      * [Analysing memory usage with valgrind](doc/Advanced.md#analysing-memory-usage-with-valgrind)
  * [CCDB / QCDB](doc/Advanced.md#ccdb--qcdb)
      * [Accessing objects in CCDB](doc/Advanced.md#accessing-objects-in-ccdb)
      * [Access GRP objects with GRP Geom Helper](doc/Advanced.md#access-grp-objects-with-grp-geom-helper)
      * [Global Tracking Data Request helper](doc/Advanced.md#global-tracking-data-request-helper)
      * [Custom metadata](doc/Advanced.md#custom-metadata)
      * [Details on the data storage format in the CCDB](doc/Advanced.md#details-on-the-data-storage-format-in-the-ccdb)
      * [Local CCDB setup](doc/Advanced.md#local-ccdb-setup)
      * [Instructions to move an object in the QCDB](doc/Advanced.md#instructions-to-move-an-object-in-the-qcdb)
  * [Asynchronous Data and Monte Carlo QC operations](doc/Advanced.md#asynchronous-data-and-monte-carlo-qc-operations)
  * [QCG](doc/Advanced.md#qcg)
      * [Display a non-standard ROOT object in QCG](doc/Advanced.md#display-a-non-standard-root-object-in-qcg)
      * [Canvas options](doc/Advanced.md#canvas-options)
      * [Local QCG (QC GUI) setup](doc/Advanced.md#local-qcg-qc-gui-setup)
  * [FLP Suite](doc/Advanced.md#flp-suite)
      * [Developing QC modules on a machine with FLP suite](doc/Advanced.md#developing-qc-modules-on-a-machine-with-flp-suite)
      * [Switch detector in the workflow <em>readout-dataflow</em>](doc/Advanced.md#switch-detector-in-the-workflow-readout-dataflow)
      * [Get all the task output to the infologger](doc/Advanced.md#get-all-the-task-output-to-the-infologger)
      * [Using a different config file with the general QC](doc/Advanced.md#using-a-different-config-file-with-the-general-qc)
      * [Enable the repo cleaner](doc/Advanced.md#enable-the-repo-cleaner)
  * [Configuration](doc/Advanced.md#configuration-1)
      * [Merging multiple configuration files into one](doc/Advanced.md#merging-multiple-configuration-files-into-one)
      * [Definition and access of simple user-defined task configuration ("taskParameters")](doc/Advanced.md#definition-and-access-of-simple-user-defined-task-configuration-taskparameters)
      * [Definition and access of user-defined configuration ("extendedTaskParameters")](doc/Advanced.md#definition-and-access-of-user-defined-configuration-extendedtaskparameters)
      * [Definition of new arguments](doc/Advanced.md#definition-of-new-arguments)
      * [Configuration files details](doc/Advanced.md#configuration-files-details)
          * [Global configuration structure](doc/Advanced.md#global-configuration-structure)
          * [Common configuration](doc/Advanced.md#common-configuration)
          * [QC Tasks configuration](doc/Advanced.md#qc-tasks-configuration)
          * [QC Checks configuration](doc/Advanced.md#qc-checks-configuration)
          * [QC Aggregators configuration](doc/Advanced.md#qc-aggregators-configuration)
          * [QC Post-processing configuration](doc/Advanced.md#qc-post-processing-configuration)
          * [External tasks configuration](doc/Advanced.md#external-tasks-configuration)
  * [Miscellaneous](doc/Advanced.md#miscellaneous)
      * [Data Sampling monitoring](doc/Advanced.md#data-sampling-monitoring)
      * [Monitoring metrics](doc/Advanced.md#monitoring-metrics)
      * [Common check IncreasingEntries](doc/Advanced.md#common-check-increasingentries)
      * [Update the shmem segment size of a detector](doc/Advanced.md#update-the-shmem-segment-size-of-a-detector)

### Where to get help

* By email at [alice-o2-qc-support@cern.ch](mailto:alice-o2-qc-support@cern.ch) 
* Discourse: https://alice-talk.web.cern.ch/
* JIRA: https://alice.its.cern.ch
* O2 development newcomers' guide: https://aliceo2group.github.io/quickstart
