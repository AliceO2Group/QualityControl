[![aliBuild](https://img.shields.io/badge/aliBuild-dashboard-lightgrey.svg)](https://alisw.cern.ch/dashboard/d/000000001/main-dashboard?orgId=1&var-storagename=All&var-reponame=All&var-checkname=build%2FQualityControl%2Fo2-dataflow%2F0&var-upthreshold=30m&var-minuptime=30)
[![JIRA](https://img.shields.io/badge/JIRA-Report%20issue-blue.svg)](https://alice.its.cern.ch/jira/secure/CreateIssue.jspa?pid=11201&issuetype=1)

This is the repository of the data quality control (QC) software for the ALICE O<sup>2</sup> system. 

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
            * [Bypassing the Data Sampling](doc/ModulesDevelopment.md#bypassing-the-data-sampling)
         * [Code Organization](doc/ModulesDevelopment.md#code-organization)
         * [User-defined modules](doc/ModulesDevelopment.md#user-defined-modules)
      * [Module creation](doc/ModulesDevelopment.md#module-creation)
      * [Test run](doc/ModulesDevelopment.md#test-run)
      * [Modification of a Task](doc/ModulesDevelopment.md#modification-of-a-task)
      * [Addition of a Check](doc/ModulesDevelopment.md#addition-of-a-check)
      * [Commit Code](doc/ModulesDevelopment.md#commit-code)
      * [DPL workflow customization](doc/ModulesDevelopment.md#dpl-workflow-customization)
      * [Plugging Data Sampling and QC into an existing DPL workflow](doc/ModulesDevelopment.md#plugging-data-sampling-and-qc-into-an-existing-dpl-workflow)
   * [Advanced topics](doc/Advanced.md)
      * [Data Inspector](doc/Advanced.md#data-inspector)
      * [Use MySQL as QC backend](doc/Advanced.md#use-mysql-as-qc-backend)
      * [Local CCDB setup](doc/Advanced.md#local-ccdb-setup)
      * [Local QCG (QC GUI) setup](doc/Advanced.md#local-qcg-qc-gui-setup)
      * [Information Service](doc/Advanced.md#information-service)
      * [Configuration files details](doc/Advanced.md#configuration-files-details)
   * [Frequently Asked Questions](doc/FAQ.md)
<!-- Added by: bvonhall, at:  -->

<!--te-->
