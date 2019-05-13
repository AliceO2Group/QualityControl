from Ccdb import Ccdb, ObjectVersion
from datetime import timedelta
from datetime import datetime


def process(ccdb: Ccdb, object_path: str, delay: int):
    
    print(f"1_per_hour : {object_path}")

    # take the first record, delete everything for the next hour, find the next one, extend validity of the previous record to match the next one, loop.
    versions = ccdb.getVersionsList(object_path)

    last_preserved: ObjectVersion = None
    preservation_list: List[ObjectVersion] = []
    deletion_list: List[ObjectVersion] = []
    for v in versions:
        if last_preserved == None or last_preserved.validFrom < v.validFrom - timedelta(hours=1):
            # first extend validity of the previous preserved (should we take into account the run ?)
            if last_preserved != None:
                ccdb.updateValidity(last_preserved, last_preserved.validFromTimestamp, str(int(v.validFromTimestamp) - 1))
            last_preserved = v
            preservation_list.append(v)
        else:
            deletion_list.append(v)
            if v.validFrom < datetime.now() - timedelta(minutes=delay):
                print("not in the grace period, we delete")
                # todo ccdb.deleteVersion(v.uuid)

    print("to be deleted : ")
    for v in deletion_list:
        print(f"   {v}")

    print("to be preserved : ")
    for v in preservation_list:
        print(f"   {v}")

# ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
# process(ccdb, "asdfasdf/example")
