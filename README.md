<!--  \cond EXCLUDE_FOR_DOXYGEN -->
[![aliBuild](https://img.shields.io/badge/aliBuild-dashboard-lightgrey.svg)](https://alisw.cern.ch/cockpit-legacy/d/000000001/main-dashboard?orgId=1&var-storagename=All&var-reponame=All&var-checkname=build%2FQualityControl%2Fo2-dataflow%2F0&var-upthreshold=30m&var-minuptime=30)
[![JIRA](https://img.shields.io/badge/JIRA-Report%20issue-blue.svg)](https://alice.its.cern.ch/jira/secure/CreateIssue.jspa?pid=11201&issuetype=1)
[![doxygen](https://img.shields.io/badge/doxygen-documentation-blue.svg)](https://aliceo2group.github.io/QualityControl/)
[![Discourse](https://img.shields.io/badge/discourse-Get%20help-blue.svg)](https://alice-talk.web.cern.ch/)

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
        * [Code Organization](doc/ModulesDevelopment.md#code-organization)
        * [Developing with aliBuild/alienv](doc/ModulesDevelopment.md#developing-with-alibuildalienv)
        * [User-defined modules](doc/ModulesDevelopment.md#user-defined-modules)
        * [Repository](doc/ModulesDevelopment.md#repository)
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
    * [Committing code](doc/ModulesDevelopment.md#committing-code)
    * [Data sources](doc/ModulesDevelopment.md#data-sources)
        * [Readout](doc/ModulesDevelopment.md#readout)
        * [DPL workflow](doc/ModulesDevelopment.md#dpl-workflow)
    * [Run number](doc/ModulesDevelopment.md#run-number)
    * [A more advanced example](doc/ModulesDevelopment.md#a-more-advanced-example)
    * [Monitoring](doc/ModulesDevelopment.md#monitoring)
    
* [The post-processing framework](doc/PostProcessing.md)
    * [Post-processing interface](doc/PostProcessing.md#post-processing-interface)
    * [Post-processing Task configuration](doc/PostProcessing.md#configuration)
        * [Triggers configuration](doc/PostProcessing.md#triggers-configuration)
    * [Running it](doc/PostProcessing.md#running-it)
    * [Convenience classes](doc/PostProcessing.md#convenience-classes)
        * [The TrendingTask class](doc/PostProcessing.md#the-trendingtask-class)
        * [The TRFCollectionTask class](doc/PostProcessing.md#the-trfcollectiontask-class)

* [Advanced topics](doc/Advanced.md)
    * [Plugging the QC to an existing DPL workflow](doc/Advanced.md#plugging-the-qc-to-an-existing-dpl-workflow)
    * [Production of QC objects outside this framework](doc/Advanced.md#production-of-qc-objects-outside-this-framework)
    * [Multi-node setups](doc/Advanced.md#multi-node-setups)
    * [Writing a DPL data producer](doc/Advanced.md#writing-a-dpl-data-producer)
    * [Access run conditions and calibrations from the CCDB](doc/Advanced.md#access-run-conditions-and-calibrations-from-the-ccdb)
    * [Definition and access of task-specific configuration](doc/Advanced.md#definition-and-access-of-task-specific-configuration)
    * [Custom QC object metadata](doc/Advanced.md#custom-qc-object-metadata)
    * [Canvas options](doc/Advanced.md#canvas-options)
    * [QC with DPL Analysis](doc/Advanced.md#qc-with-dpl-analysis)
    * [Data Inspector](doc/Advanced.md#data-inspector)
    * [Details on the data storage format in the CCDB](doc/Advanced.md#details-on-the-data-storage-format-in-the-ccdb)
    * [Local CCDB setup](doc/Advanced.md#local-ccdb-setup)
    * [Local QCG (QC GUI) setup](doc/Advanced.md#local-qcg-qc-gui-setup)
    * [FLP Suite](doc/Advanced.md#flp-suite)
    * [Configuration files details](doc/Advanced.md#configuration-files-details)

### Where to get help

* CERN Mailing lists: alice-o2-wp7 and alice-o2-qc-contact
* Discourse: https://alice-talk.web.cern.ch/
* JIRA: https://alice.its.cern.ch
* O2 development newcomers' guide: https://aliceo2group.github.io/quickstart
* Doxygen: https://aliceo2group.github.io/QualityControl/
