# Release notes

1.1

- Add a new policy `multiple_per_run` that is simpler than `production`. 
- Several rules can now be applied to the same path if the flag `continue_with_next_rule` is set to true on the first one(s). 
  It means that any rule that has `continue_with_next_rule` set to true will be applied as well as the next matching rule
  and so on and so forth.

1.0

- First version released with pip.