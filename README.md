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
        * [Readout chain](doc/QuickStart.md#readout-chain)
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
        * [Running it](doc/PostProcessing.md#running-it)
    * [Convenience classes](doc/PostProcessing.md#convenience-classes)
        * [The TrendingTask class](doc/PostProcessing.md#the-trendingtask-class)
        * [The SliceTrendingTask class](doc/PostProcessing.md#the-slicetrendingtask-class)
        * [The TRFCollectionTask class](doc/PostProcessing.md#the-trfcollectiontask-class)
    * [More examples](doc/PostProcessing.md#more-examples)
* [Advanced topics](doc/Advanced.md)
    * [Framework](doc/Advanced.md#framework)
        * [Plugging the QC to an existing DPL workflow](doc/Advanced.md#plugging-the-qc-to-an-existing-dpl-workflow)
        * [Production of QC objects outside this framework](doc/Advanced.md#production-of-qc-objects-outside-this-framework)
        * [Multi-node setups](doc/Advanced.md#multi-node-setups)
        * [Batch processing](doc/Advanced.md#batch-processing)
        * [Moving window](doc/Advanced.md#moving-window)
        * [Writing a DPL data producer](doc/Advanced.md#writing-a-dpl-data-producer)
        * [Custom merging](doc/Advanced.md#custom-merging)
        * [QC with DPL Analysis](doc/Advanced.md#qc-with-dpl-analysis)
    * [Solving performance issues](doc/Advanced.md#solving-performance-issues)
    * [CCDB / QCDB](doc/Advanced.md#ccdb--qcdb)
        * [Accessing objects in CCDB](doc/Advanced.md#accessing-objects-in-ccdb)
        * [Access GRP objects with GRP Geom Helper](doc/Advanced.md#access-grp-objects-with-grp-geom-helper)
        * [Custom metadata](doc/Advanced.md#custom-metadata)
        * [Details on the data storage format in the CCDB](doc/Advanced.md#details-on-the-data-storage-format-in-the-ccdb)
        * [Local CCDB setup](doc/Advanced.md#local-ccdb-setup)
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
        * [Definition and access of task-specific configuration](doc/Advanced.md#definition-and-access-of-task-specific-configuration)
        * [Configuration files details](doc/Advanced.md#configuration-files-details)
    * [Miscellaneous](doc/Advanced.md#miscellaneous)
        * [Data Sampling monitoring](doc/Advanced.md#data-sampling-monitoring)
        * [Monitoring metrics](doc/Advanced.md#monitoring-metrics)
        * [Common check IncreasingEntries](doc/Advanced.md#common-check-increasingentries)
* [Frequently asked questions](doc/FAQ.md)
    * [Git](doc/FAQ.md#git)
        * [Are there instructions how to use Git ?](doc/FAQ.md#are-there-instructions-how-to-use-git-)
    * [Coding Guidelines](doc/FAQ.md#coding-guidelines)
        * [Where are the Coding Guidelines ?](doc/FAQ.md#where-are-the-coding-guidelines-)
        * [How do I check my code or apply the guidelines ?](doc/FAQ.md#how-do-i-check-my-code-or-apply-the-guidelines-)
    * [Build](doc/FAQ.md#build)
        * [How do I add a dependency to my module ?](doc/FAQ.md#how-do-i-add-a-dependency-to-my-module-)
        * [How do I make my module depend on library XXX from AliceO2 ?](doc/FAQ.md#how-do-i-make-my-module-depend-on-library-xxx-from-aliceo2-)
    * [Run](doc/FAQ.md#run)
        * [Why are my QC processes using 100% CPU ?](doc/FAQ.md#why-are-my-qc-processes-using-100-cpu-)
    * [QCDB](doc/FAQ.md#qcdb)
        * [How to see which objects are stored in the CCDB ?](doc/FAQ.md#how-to-see-which-objects-are-stored-in-the-ccdb-)
        * [How to delete objects from the CCDB ?](doc/FAQ.md#how-to-delete-objects-from-the-ccdb-)
        * [My objects are not stored due to their size. What can I do ?](doc/FAQ.md#my-objects-are-not-stored-due-to-their-size-what-can-i-do-)

### Where to get help

* Discourse: https://alice-talk.web.cern.ch/
* JIRA: https://alice.its.cern.ch
* O2 development newcomers' guide: https://aliceo2group.github.io/quickstart
* Doxygen: https://aliceo2group.github.io/QualityControl/
* CERN Mailing lists: alice-o2-wp7 and alice-o2-qc-contact
