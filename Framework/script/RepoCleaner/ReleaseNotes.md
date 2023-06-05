# Release notes

Current 
- 

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