## ALICE data Quality Control framework and Modules

This repository contains the ALICE O<sup>2</sup> data Quality Control (QC) framework and modules.
It is divided into `Framework`, maintained by the framework developers and `Modules`, which are mostly developed by detector experts.

### Copilot agent instructions

- All the code should apply the [ALICE O2 Coding Guidelines](https://github.com/AliceO2Group/CodingGuidelines).
See [Formatting tool](https://github.com/AliceO2Group/CodingGuidelines?tab=readme-ov-file#formatting-tool) for details how to set up code formatter.
Do not attempt to install `clang` with `aliBuild`, use the one you have available on your system.
- Unless you are running on a developer's machine and you were provided specific instructions, do not attempt to build the code or run tests. Quality Control uses a large number of external dependencies, which are not available in the default Copilot environment on GitHub.
- When working on the C++ code-base, use the C++20 standard.
- When modifying class definitions for which a ROOT dictionary is generated, remember to update their version in macros such as `ClassDefOverride` (e.g. `ClassDefOverride(MonitorObject, 15)` -> `ClassDefOverride(MonitorObject, 16)`).
- Avoid changes which are not relevant to the current task.
- When adding a new feature, write a unit test if feasible. Unit tests should use catch2.
- When adding a new feature, extend the documentation accordingly and make sure that Tables of Contents are updated.
- When providing a fix, explain what was causing the issue and how it was fixed.
- When adding new code, make sure that necessary headers are included. Likewise, when removing code, remove the corresponding headers if they are not needed anymore.
- When dealing with the code in `Modules`, prefer minimal changes which do not break the existing functionality.