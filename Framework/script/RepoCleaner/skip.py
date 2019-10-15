from datetime import datetime
from datetime import timedelta
import logging

from Ccdb import Ccdb, ObjectVersion


def process(ccdb: Ccdb, object_path: str, delay: int):
    '''
    Process this deletion rule on the object. We use the CCDB passed by argument.
    
    This policy does nothing and allows to preserve some directories.

    :param ccdb: the ccdb in which objects are cleaned up.
    :param object_path: path to the object, or pattern, to which a rule will apply.
    :param delay: the grace period during which a new object is never deleted.
    :return a dictionary with the number of deleted, preserved and updated versions. Total = deleted+preserved.
    '''
    
    logging.debug(f"Plugin 'skip' processing {object_path}")
        
    return {"deleted" : 0, "preserved": 0}

    
def main():
    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    process(ccdb, "asdfasdf/example", 60)


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
