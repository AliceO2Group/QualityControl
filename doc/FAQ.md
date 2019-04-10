# Frequently asked questions

<!--TOC generated with https://github.com/ekalinin/github-markdown-toc-->
<!--./gh-md-toc --insert /path/to/README.md-->
<!--ts-->
   * [Frequently asked questions](#frequently-asked-questions)
      * [CCDB repository](#ccdb-repository)
         * [How to see which objects are stored in the CCDB ?](#how-to-see-which-objects-are-stored-in-the-ccdb-)
         * [How to delete objects from the CCDB ?](#how-to-delete-objects-from-the-ccdb-)

<!-- Added by: bvonhall, at: 2019-04-10T12:19+0200 -->

<!--te-->

[← Go back to Advanced Topics](Advanced.md) | [↑ Go to the Table of Content ↑](../README.md) 

## CCDB repository

### How to see which objects are stored in the CCDB ?

The easiest is to use the QCG (QC GUI). If you use the central test CCDB, you can use the central test QCG. Simply direct your browser to [https://qcg-test.cern.ch](https://qcg-test.cern.ch).

If for some reason you don't want or can't use the QCG, the CCDB provides a web interface accessible at [http://ccdb-test.cern.ch:8080/browse/](http://ccdb-test.cern.ch:8080/browse/).

### How to delete objects from the CCDB ?

By accessing http://ccdb-test.cern.ch:8080/truncate/path/to/folder you will delete all the objects at the given path. Careful with that please ! Don't delete data of others.<br/>In production it will of course not be possible to do so. 

[← Go back to Advanced Topics](Advanced.md) | [↑ Go to the Table of Content ↑](../README.md) 
