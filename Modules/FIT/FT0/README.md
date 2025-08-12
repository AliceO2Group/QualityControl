# FT0 quality control

## Aging monitoring

The aging monitoring of FT0 is performed by 1 minute long laser runs that should be launched after each beam dump. A dedicated QC task is analyzing the laser data: `o2::quality_control_modules::ft0::AgingLaserTask`.

At the moment the QC task is adapted to the FT0 laser calibration system (LCS) and the monitoring of the FT0 aging. If needed, the task can be generalized to work with other FIT detectors.

### Monitoring principles

The schematics of the LCS is shown below. Per laser pulse, there will be two signals in each reference channel and one signal in each detector channel. The signals are separated in time by well defined delays, so one can identify them by BC ID.

<img src="images/lcs.png" width="500px">

More information about the LCS and the hardware side of the aging monitoring can be found [here](https://indico.cern.ch/event/1229241/contributions/5172798/attachments/2561719/4420583/Ageing-related%20tasks.pdf).

---

### AgingLaserTask

#### What it does (high level)

* Selects laser events in specific bunch crossings (BCs).
* Separately identifies:

  * detector-channel laser signals,
  * reference-channel peak-1 and peak-2 signals (two per laser shot).
* Fills per-channel **amplitude** and **time** histograms, split by ADC where relevant.
* (Optional) Produces a rich set of debug histograms.

This task is the *producer* of the raw per-channel spectra used later by post-processing.

#### Inputs

* Digits and channel streams (from workflow config), e.g.

  ```
  digits: FT0/DIGITSBC/0
  channels: FT0/DIGITSCH/0
  ```

#### Configuration

Example: `etc/ft0-aging-laser.json`.

| Key                      |         Type |           Default | Meaning                                                        |
| ------------------------ | -----------: | ----------------: | -------------------------------------------------------------- |
| `detectorChannelIDs`     | list `uint8` |       all `0–207` | Detector channels to monitor.                                  |
| `referenceChannelIDs`    | list `uint8` | `208,209,210,211` | Reference (laser monitor) channels.                            |
| `detectorAmpCut`         |          int |               `0` | Minimum ADC for detector channels (**currently not applied**). |
| `referenceAmpCut`        |          int |             `100` | Minimum ADC for reference channels (suppress cross-talk).      |
| `laserTriggerBCs`        |   list `int` |          `0,1783` | BCs where the laser is triggered.                              |
| `detectorBCdelay`        |          int |             `131` | BC shift from trigger to detector signal.                      |
| `referencePeak1BCdelays` |   list `int` | `115,115,115,115` | BC shifts for ref. peak-1 (per reference channel).             |
| `referencePeak2BCdelays` |   list `int` | `136,142,135,141` | BC shifts for ref. peak-2 (per reference channel).             |
| `debug`                  |         bool |           `false` | Enable extra (heavy) debug histograms.                         |

> The BC delays and channel ID ranges are hardware-driven and stable; adjust only if the LCS timing changes.

#### Outputs (Monitor Objects)

Always produced (names correspond to *amplitude/time vs channel*; all TH2I unless noted):

* **Amplitude vs channel**

  * `AmpPerChannel` - used by postprocessing task
  * `AmpVsChADC0` - detector + reference, ADC0
  * `AmpVsChADC1` - detector + reference, ADC1
  * `AmpVsChPeak1ADC0` / `AmpVsChPeak1ADC1` - reference peak-1
  * `AmpVsChPeak2ADC0` / `AmpVsChPeak2ADC1` - reference peak-2
* **Time vs channel**

  * `TimeVsCh` - detector + reference
  * `TimeVsChPeak1` - reference peak-1 (both ADCs)
  * `TimeVsChPeak2` - reference peak-2 (both ADCs)

Debug (enabled with `debug=true`; types indicated):

* **Reference-channel 1D spectra (per channel, TH1I)**
  amplitude: `mMapDebugHistAmp*` (for all/ADC0/ADC1/peak1/peak2/combos)
  time: `mMapDebugHistTimePeak{1,2}*`
* **Time vs channel (TH2I)**: `DebugTimeVsChADC{0,1}`, `DebugTimeVsChPeak{1,2}ADC{0,1}`
* **BC distributions (TH1I)**: `DebugBC*`, split by detector/reference, ADC and with/without amplitude cuts
* **Amplitude vs BC (TH2I)** per reference channel: `mMapDebugHistAmpVsBC*`

> Note: the “time vs BC” debug maps are intentionally disabled in local-batch to avoid ROOT I/O size issues.

---

# AgingLaserPostProcTask

`o2::quality_control_modules::ft0::AgingLaserPostProcTask`

#### Purpose

Reduce the raw per-channel amplitude spectra to **one scalar per channel** representing the **weighted-mean amplitude**, normalized to the reference channels. Output is split into two histograms: A-side (channels 0–95) and C-side (channels 96–207).

#### Inputs

* From the QC repository path `FT0/MO/AgingLaser`:

  * `AmpPerChannel` (TH2): amplitude (ADC) vs channel, aggregated by the task.
    *(If you renamed the source MO, adjust the retrieval in the task config.)*

#### Algorithm (per update)

1. **Reference normalization**

   * For each configured reference channel:

     * Project its slice `AmpPerChannel(ch)` → `TH1`.
     * In `[adcSearchMin, adcSearchMax]` find the maximum `x_max`.
     * Fit a Gaussian in `[(1−a)·x_max, (1+b)·x_max]` → mean `μ_ref`.
   * `norm = average(μ_ref)` over all successful fits.
2. **Per-channel value**

   * For **every** detector channel:

     * Find the global maximum `x_max`.
     * In the same fractional window around `x_max`, compute **weighted mean**
       `⟨x⟩ = Σ w_i x_i / Σ w_i` with weights `w_i = bin content`.
     * Store `value = ⟨x⟩ / norm`.
3. **Publish two TH1F MOs**

   * `AmpPerChannelNormWeightedMeanA`: 96 bins, channels 0–95.
   * `AmpPerChannelNormWeightedMeanC`: 112 bins, channels 96–207.

> Tip: when booking the C-side histogram use upper edge **208** (exclusive) with 112 bins to avoid off-by-one bin widths.

#### Configuration (extendedTaskParameters)

| Key                   |         Type |   Default | Meaning                                        |
| --------------------- | -----------: | --------: | ---------------------------------------------- |
| `detectorChannelIDs`  | list `uint8` |   `0–207` | Detector channels to process (subset allowed). |
| `referenceChannelIDs` | list `uint8` | `208–210` | Reference channels used for normalization.     |
| `adcSearchMin`        |       double |     `150` | Lower ADC for reference peak search.           |
| `adcSearchMax`        |       double |     `600` | Upper ADC for reference peak search.           |
| `fracWindowA`         |       double |    `0.25` | Low fractional window, `a` in `(1−a)·x_max`.   |
| `fracWindowB`         |       double |    `0.25` | High fractional window, `b` in `(1+b)·x_max`.  |

#### Output objects (names & types)

* `FT0/MO/AgingLaserPostProc/AmpPerChannelNormWeightedMeanA` (TH1F)
* `FT0/MO/AgingLaserPostProc/AmpPerChannelNormWeightedMeanC` (TH1F)

#### Trending (example)

Having two output MOs, allows us to create two time series, one per side. An example workflow that accomplishes this is in `etc/ft0-aging-laser-postproc.json`.

#### Notes & gotchas

* **Off-by-one binning (C side)**: for channels `96–207` you need **112 bins** and an **exclusive** upper edge at `208`. Using `207` with 112 bins yields non-unit bin width and will trip histogram helpers.
* **Reference fits**: if all Gaussian fits fail, the post-proc update exits early and publishes nothing for that cycle.
* **Amplitude cuts**: `detectorAmpCut` is currently not used by the task’s filling logic; if you need a cut in the derived quantity, apply it in post-processing or trending.
