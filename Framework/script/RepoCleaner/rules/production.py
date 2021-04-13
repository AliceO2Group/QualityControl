import logging
from typing import Dict
from collections import defaultdict
from datetime import datetime
import time
from datetime import timedelta, date

from Ccdb import Ccdb, ObjectVersion


def in_grace_period(version: ObjectVersion, delay: int):
    return version.validFromAsDt + timedelta(minutes=delay) > datetime.now()

eor_dict = {}  # to have fake eor numbers


def process(ccdb: Ccdb, object_path: str, delay: int, extra_params: Dict[str, str]):
    '''
    Process this deletion rule on the object. We use the CCDB passed by argument.
    
    This is the rule we use in production for the objects that need to be migrated.

    What it does:
      - Versions without run number -> delete after the `delay`
         - (The run number is set in "RunNumber" metadata)
      - For a given run
         - Keep everything for 30 minutes (configurable: delay_first_trimming)
         - Keep 1 per 10 minutes (configurable: period_btw_versions_first) after this delay.
         - Keep 1 per 1 hour (configurable: period_btw_versions_final)
           as well as first and last at EOR+3h (configurable: delay_final_trimming)
         - What has not been deleted at this stage is marked to be migrated (preservation = true)

    Extra parameters:
      - delay_first_trimming: Delay in minutes before first trimming. (default: 30)
      - period_btw_versions_first: Period in minutes between the versions we will keep after first trimming. (default: 10)
      - delay_final_trimming: Delay in minutes, counted from the EOR, before we do the final cleanup and mark for migration. (default: 180)
      - period_btw_versions_final: Period in minutes between the versions we will migrate. (default: 60)

    Implementation :
      - Go through all objects:
         - if a run is set, add the object to the corresponding map element.
         - if not, if the delay has passed, delete.
      - Go through the map: for each run
         - Check if run has finished and get the time of EOR if so.
         - if run is over for more than 3 hours
            - Final trimming
         - else
            - For each version
              - First trimming (1 per 10 minutes)
    During the first trimming we mark (trim1=true) the versions we have already treated to avoid redoing the work.


    :param ccdb: the ccdb in which objects are cleaned up.
    :param object_path: path to the object, or pattern, to which a rule will apply.
    :param delay: the grace period during which a new object is not deleted although it has no run number.
    :param extra_params: a dictionary containing extra parameters for this rule.
    :return a dictionary with the number of deleted, preserved and updated versions. Total = deleted+preserved.
    '''

    logging.debug(f"Plugin 'production' processing {object_path}")

    # Variables
    preservation_list: List[ObjectVersion] = []
    deletion_list: List[ObjectVersion] = []
    update_list: List[ObjectVersion] = []
    runs_dict: DefaultDict[str, List[ObjectVersion]] = defaultdict(list)

    # Extra parameters
    delay_first_trimming = int(extra_params.get("delay_first_trimming", 30))
    logging.debug(f"delay_first_trimming : {delay_first_trimming}")
    period_btw_versions_first = int(extra_params.get("period_btw_versions_first", 10))
    logging.debug(f"period_btw_versions_first : {period_btw_versions_first}")
    delay_final_trimming = int(extra_params.get("delay_final_trimming", 180))
    logging.debug(f"delay_final_trimming : {delay_final_trimming}")
    period_btw_versions_final = int(extra_params.get("period_btw_versions_final", 60))
    logging.debug(f"period_btw_versions_final : {period_btw_versions_final}")

    # Find all the runs and group the versions
    versions = ccdb.getVersionsList(object_path)
    logging.debug(f"Dispatching versions to runs")
    for v in versions:
        if "RunNumber" in v.metadata:
            runs_dict[v.metadata['RunNumber']].append(v)
        else:
            runs_dict[-1].append(v)  # the ones with no run specified
    logging.debug(f"   Number of runs : {len(runs_dict)}")
    logging.debug(f"   Number of versions without runs : {len(runs_dict[-1])}")

    # Versions without runs: spare if more recent than the delay
    logging.debug(f"Eliminating versions without runs if older than the grace period")
    for run_version in runs_dict[-1]:
        if in_grace_period(run_version, delay):
            preservation_list.append(run_version)
        else:
            logging.debug(f"   delete {run_version}")
            deletion_list.append(run_version)
            ccdb.deleteVersion(run_version)
    del runs_dict[-1]  # remove this "run" from the list

    # For each run
    logging.debug(f"Trimming the versions with a run number")
    for run, run_versions in runs_dict.items():
        logging.debug(f"   Processing run {run}")
        # TODO get the EOR if it happened, meanwhile we use `eor_dict` or compute first object time + 15 hours
        eor = eor_dict.get(int(run), run_versions[0].validFromAsDt + timedelta(hours=15))
        logging.debug(f"   EOR : {eor}")

        # run is finished for long enough
        if eor is not None and datetime.now() > eor + timedelta(minutes=delay_final_trimming):
            logging.debug("      Run is over for long enough, let's do the final trimming")
            final_trimming(ccdb, period_btw_versions_final, run_versions, preservation_list, update_list, deletion_list)
        else:  # trim the versions as the run is ongoing or too fresh
            logging.debug("      Run is too fresh or still ongoing, we do the light trimming")
            first_trimming(ccdb, delay_first_trimming, period_btw_versions_first, run_versions,
                           preservation_list, update_list, deletion_list)

    # Print result
    logging.debug("*** Results ***")
    logging.debug(f"deleted ({len(deletion_list)}) : ")
    for v in deletion_list:
        logging.debug(f"   {v}")
    logging.debug(f"preserved ({len(preservation_list)}) : ")
    for v in preservation_list:
        logging.debug(f"   {v}")
    logging.debug(f"updated ({len(update_list)}) : ")
    for v in update_list:
        logging.debug(f"   {v}")

    return {"deleted": len(deletion_list), "preserved": len(preservation_list), "updated": len(update_list)}


def first_trimming(ccdb, delay_first_trimming, period_btw_versions_first, run_versions, preservation_list,
                   update_list, deletion_list):
    last_preserved: ObjectVersion = None
    limit_first_trimming = datetime.now() - timedelta(minutes=delay_first_trimming)
    metadata = {'trim1': 'done'}
    for v in run_versions:
        logging.debug(f"      Processing {v} ")

        if 'trim1' in v.metadata:  # check if it is already in the cache
            logging.debug(f"         Already processed - skip")
            last_preserved = v
            preservation_list.append(v)
            continue

        if v.validFromAsDt < limit_first_trimming:  # delay for 1st trimming is exhausted
            # if it is the first or if it is "far enough" from the previous one
            if last_preserved is None or \
                    last_preserved.validFromAsDt < v.validFromAsDt - timedelta(minutes=period_btw_versions_first):
                if last_preserved is not None:
                    # first extend validity of the previous preserved and set flag
                    ccdb.updateValidity(last_preserved, last_preserved.validFrom, str(int(v.validFrom) - 1), metadata)
                    update_list.append(last_preserved)
                    logging.debug(f"      Extension of {last_preserved}")
                last_preserved = v
                preservation_list.append(v)
            else:  # too close to the previous one, delete
                deletion_list.append(v)
                logging.debug(f"      Deletion of {v}")
                ccdb.deleteVersion(v)
        else:
            preservation_list.append(v)


def final_trimming(ccdb, period_btw_versions_final, run_versions, preservation_list, update_list, deletion_list):
    # go through the whole run, keep only one version every period_btw_versions_final minutes
    last_preserved: ObjectVersion = None
    metadata = {'trim1': '', 'preservation':'true'}
    for v in run_versions:
        logging.debug(f"      Processing {v} ")
        # if it is the first or if the last_preserved is older than `period_btw_versions_final`
        if last_preserved is None \
                or last_preserved.validFromAsDt < v.validFromAsDt - timedelta(minutes=period_btw_versions_final) \
                or v == run_versions[-1]:  # v is last element, which we must preserve
            if last_preserved is not None:  # first extend validity of the previous preserved and set flag
                ccdb.updateValidity(last_preserved, last_preserved.validFrom, str(int(v.validFrom) - 1), metadata)
                update_list.append(last_preserved)
                logging.debug(f"      Extension of {last_preserved}")
            last_preserved = v
            preservation_list.append(v)
        else:  # too close to the previous one, delete
            deletion_list.append(v)
            logging.debug(f"      Deletion of {v}")
            ccdb.deleteVersion(v)


def main():
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                        datefmt='%d-%b-%y %H:%M:%S')
    logging.getLogger().setLevel(int(10))

    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    extra = {"delay_first_trimming": "25", "period_btw_versions_first": "11", "delay_final_trimming": "179",
             "period_btw_versions_final": "15"}
    path = "qc/TST/MO/repo/test"
    run = 123456

    prepare_test_data(ccdb, path, run)

    process(ccdb, path, 15, extra)


def prepare_test_data(ccdb, path, run):
    current_timestamp = int(time.time() * 1000)
    data = {'part': 'part'}
    metadata = {'RunNumber': str(run)}
    # 1 version every 1 minutes starting 1 hour ago
    for x in range(60):
        from_ts = current_timestamp - (60 - x) * 60 * 1000
        to_ts = from_ts + 24 * 60 * 60 * 1000  # a day
        version_info = ObjectVersion(path=path, validFrom=from_ts, validTo=to_ts, metadata=metadata)
        ccdb.putVersion(version=version_info, data=data)
    # 1 version every 1 minutes starting 1/2 hour ago WITHOUT run
    current_timestamp = int(time.time() * 1000)
    metadata = {}
    for x in range(30):
        from_ts = current_timestamp - (60 - x) * 60 * 1000
        to_ts = from_ts + 24 * 60 * 60 * 1000  # a day
        version_info = ObjectVersion(path=path, validFrom=from_ts, validTo=to_ts, metadata=metadata)
        ccdb.putVersion(version=version_info, data=data)


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
