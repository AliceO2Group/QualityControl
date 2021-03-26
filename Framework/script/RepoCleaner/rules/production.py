from datetime import datetime
from datetime import timedelta
import logging
from typing import Dict
from collections import defaultdict

from Ccdb import Ccdb, ObjectVersion


def process(ccdb: Ccdb, object_path: str, delay: int, extra_params: Dict[str, str]):
    '''
    Process this deletion rule on the object. We use the CCDB passed by argument.
    
    This is the rule we use in production for the objects that need to be migrated.

    What it does:
      - Versions without run number -> delete after the `delay`
      - For a given run
         - Delete half the versions after 30 minutes (configurable: delay_first_trimming)
           !!! this is actually very difficult to implement as the tool is stateless. We would have to store some
           metadata for each version to know whether it should be deleted or not after the first delay.
           It is perhaps easier to keep 1 per 10 minutes (configurable: period_btw_versions_first).
         - Delete all versions but 1 per 1 hour (configurable: period_btw_versions_final) + first and last at EOR+3h (configurable: delay_final_trimming)
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
         - For each version
            - if <

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
    delay_first_trimming = extra_params.get("delay_first_trimming", 30)
    logging.debug(f"delay_first_trimming : {delay_first_trimming}")
    period_btw_versions_first = extra_params.get("period_btw_versions_first", 10)
    logging.debug(f"period_btw_versions_first : {period_btw_versions_first}")
    delay_final_trimming = extra_params.get("delay_final_trimming", 180)
    logging.debug(f"delay_final_trimming : {delay_final_trimming}")
    period_btw_versions_final = extra_params.get("period_btw_versions_final", 60)
    logging.debug(f"period_btw_versions_final : {period_btw_versions_final}")

    # Find all the runs and group the versions
    versions = ccdb.getVersionsList(object_path)
    for v in versions:
        logging.debug(f"Processing {v}")
        if "Run" in v.metadata:
            runs_dict[v.metadata['Run']].append(v)
        else:
            runs_dict[-1].append(v) # the ones with no run specified

    logging.debug(f"Number of runs : {len(runs_dict)}")
    logging.debug(f"Number of versions without runs : {len(runs_dict[-1])}")

    return {"deleted" : len(deletion_list), "preserved": len(preservation_list), "updated" : len(update_list)}

    
def main():
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S')
    logging.getLogger().setLevel(int(10))

    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    extra = {"delay_first_trimming": "25", "period_btw_versions_first": "11", "delay_final_trimming": "179", "period_btw_versions_final": "62"}




    process(ccdb, "qc/TST/MO/task1/example", 60, extra)


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
