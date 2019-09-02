<!--  \cond EXCLUDE_FOR_DOXYGEN -->
[![aliBuild](https://img.shields.io/badge/aliBuild-dashboard-lightgrey.svg)](https://alisw.cern.ch/cockpit-legacy/d/000000001/main-dashboard?orgId=1&var-storagename=All&var-reponame=All&var-checkname=build%2FQualityControl%2Fo2-dataflow%2F0&var-upthreshold=30m&var-minuptime=30)
[![JIRA](https://img.shields.io/badge/JIRA-Report%20issue-blue.svg)](https://alice.its.cern.ch/jira/secure/CreateIssue.jspa?pid=11201&issuetype=1)
[![doxygen](https://img.shields.io/badge/doxygen-documentation-blue.svg)](https://aliceo2group.github.io/QualityControl/)
[![Discourse](https://img.shields.io/badge/discourse-Get%20help-blue.svg)](https://alice-talk.web.cern.ch/)

<!--  \endcond  -->

This is the repository of the data quality control (QC) software for the ALICE O<sup>2</sup> system. 

For a general overview of our (O2) software, organization and processes, please see this [page](https://aliceo2group.github.io/).

<!--TOC generated with https://github.com/ekalinin/github-markdown-toc-->
<!--./gh-md-toc --insert /path/to/README.md-->
<!--ts-->
   * [QuickStart](doc/QuickStart.md)
      * [Requirements](doc/QuickStart.md#requirements)
      * [Setup](doc/QuickStart.md#setup)
      * [Execution](doc/QuickStart.md#execution)
         * [Basic workflow](doc/QuickStart.md#basic-workflow)
         * [Readout chain](doc/QuickStart.md#readout-chain)
   * [Modules development](doc/ModulesDevelopment.md)
      * [Context](doc/ModulesDevelopment.md#context)
         * [QC architecture](doc/ModulesDevelopment.md#qc-architecture)
         * [DPL](doc/ModulesDevelopment.md#dpl)
         * [Data Sampling](doc/ModulesDevelopment.md#data-sampling)
         * [Code Organization](doc/ModulesDevelopment.md#code-organization)
         * [User-defined modules](doc/ModulesDevelopment.md#user-defined-modules)
      * [Module creation](doc/ModulesDevelopment.md#module-creation)
      * [Test run](doc/ModulesDevelopment.md#test-run)
      * [Modification of a Task](doc/ModulesDevelopment.md#modification-of-a-task)
      * [Addition of a Check](doc/ModulesDevelopment.md#addition-of-a-check)
      * [Commit Code](doc/ModulesDevelopment.md#commit-code)
      * [Details on data storage](doc/ModulesDevelopment.md#details-on-data-storage)
   * [Advanced topics](doc/Advanced.md)
      * [Access conditions from the CCDB](doc/Advanced.md#access-conditions-from-the-ccdb)
      * [Definition and access of task-specific configuration](doc/Advanced.md#definition-and-access-of-task-specific-configuration)
      * [Custom QC object metadata](doc/Advanced.md#custom-qc-object-metadata)
      * [Data Inspector](doc/Advanced.md#data-inspector)
      * [Use MySQL as QC backend](doc/Advanced.md#use-mysql-as-qc-backend)
      * [Local CCDB setup](doc/Advanced.md#local-ccdb-setup)
      * [Local QCG (QC GUI) setup](doc/Advanced.md#local-qcg-qc-gui-setup)
      * [Information Service](doc/Advanced.md#information-service)
      * [Configuration files details](doc/Advanced.md#configuration-files-details)
   * [Frequently Asked Questions](doc/FAQ.md)
<!-- Added by: bvonhall, at:  -->

<!--te-->

### Where to get help

* CERN Mailing lists: alice-o2-wp7 and alice-o2-qc-contact
* Discourse: https://alice-talk.web.cern.ch/
* JIRA: https://alice.its.cern.ch
* O2 development newcomers' guide: https://aliceo2group.github.io/quickstart
* Doxygen: https://aliceo2group.github.io/QualityControl/
