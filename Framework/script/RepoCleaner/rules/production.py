from datetime import datetime
from datetime import timedelta
import logging
from typing import Dict

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
      - delay_first_trimming: Delay in minutes before first trimming.
      - period_btw_versions_first: Period in minutes between the versions we will keep after first trimming.
      - delay_final_trimming: Delay in minutes, counted from the EOR, before we do the final cleanup and mark for migration.
      - period_btw_versions_final: Period in minutes between the versions we will migrate.

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

    preservation_list: List[ObjectVersion] = []
    deletion_list: List[ObjectVersion] = []
    update_list: List[ObjectVersion] = []
    runs_dict: DefaultDict[str, List[ObjectVersion]] = defaultdict(list)
        
    return {"deleted" : 0, "preserved": 0}

    
def main():
    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    process(ccdb, "asdfasdf/example", 60)


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
