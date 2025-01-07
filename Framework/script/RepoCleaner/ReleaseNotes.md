# Release notes

New
- 

1.10
- Revive and clean up repocleaner tests
- Add option to ignore the last execution of the repocleaner
- repocleaner: none_kept: use creation time

1.9
- [QC-1229] - Repocleaner policy for the moving windows

1.8
- 3 small fixes (https://github.com/AliceO2Group/QualityControl/pull/2308)
- [QC-1097] - use metadata to filter what to delete with o2-qc-repo-delete-time-interval
- [QC-1226] - Make config file name in consul configurable

1.7
- [QC-1144] 1_per_run : Consider RunNumber=0 as no run number (#2238)
- [QC-1142 ]fix object preservation for 1_per_run policy (#2228)
- [QC-996] policy multiple_per_run can delete first and last (#1921)

1.6 
- add option to set adjustableEOV when updating validity
- [QC-986] Do not touch the validity any more in rules 1_per_hour and production
- Add the possibility to Limit the script o2-qc-repo-delete-not-in-runs to a time period

1.5
- Add the binary `o2-qc-repo-delete-not-in-runs` to the install list. 
- Remove excessive logging in `Ccdb.py`

1.4
- Add new tool to delete objects not belonging to a list of runs.

1.3
- Add new tool `o2-qc-repo-find-objects-not-updated` to find all the objects under a path that did not get a new
  version in the past X days. 

1.2

- Add option `--only-path-no-subdir` to `o2-qc-repo-cleaner` to allow setting `--only-path` to an object rather than a
  folder, or to ignore subfolders. 
- Add option `--only-path-no-subdir` to `o2-qc-repo-delete-interval` to allow processing an object rather than a folder or 
  to ignore subfolders. 

1.1

- Add a new policy `multiple_per_run` that is simpler than `production`. 
- Several rules can now be applied to the same path if the flag `continue_with_next_rule` is set to true on the first one(s). 
  It means that any rule that has `continue_with_next_rule` set to true will be applied as well as the next matching rule
  and so on and so forth.

1.0

- First version released with pip.