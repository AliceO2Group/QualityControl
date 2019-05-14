import datetime
import logging
import sys
from typing import List

import dryable
import requests


class ObjectVersion:

    def __init__(self, path, uuid, validFrom, validTo):
        self.path = path
        self.uuid = uuid
        self.validFrom = datetime.datetime.fromtimestamp(int(validFrom) / 1000)  # /1000 because we get ms
        self.validFromTimestamp = validFrom
        self.validToTimestamp = validTo
        
    def __repr__(self):
        return f"Version of object {self.path} valid from {self.validFrom} (uuid {self.uuid}, ts {self.validFromTimestamp})"


class Ccdb:

    def __init__(self, url):
        logging.info(f"Instantiate CCDB at {url}")
        self.url = url

    def getObjectsList(self) -> List[str]:
        url_for_all_obj = self.url + '/latest/.*'
        logging.debug(f"Ccdb::getObjectsList -> {url_for_all_obj}")
        headers = {'Accept':'application/json'}
        r = requests.get(url_for_all_obj, headers=headers)
        json = r.json()
        paths = []
        for item in json['objects']:
            paths.append(item['path'])
        return paths

    def getVersionsList(self, item: str) -> List[ObjectVersion]:
        """Get the list of all versions for the given object"""
        url_browse_all_versions = self.url + '/browse/' + item
        logging.debug(f"Ccdb::getVersionsList -> {url_browse_all_versions}")
        headers = {'Accept':'application/json'}
        r = requests.get(url_browse_all_versions, headers=headers)
        json = r.json()
        versions = []
        for item in json['objects']:
            version = ObjectVersion(path=item['path'], uuid=item['id'], validFrom=item['validFrom'], validTo=item['validUntil'])
            versions.insert(0, version)
        return versions

    @dryable.Dryable()
    def deleteVersion(self, version: ObjectVersion):
        logging.debug(version)
        url_delete = self.url + '/' + version.path + '/' + version.validFromTimestamp + '/' + version.uuid
        logging.info(f"Delete version at url {url_delete}")
        try:
            r = requests.delete(url_delete)
            r.raise_for_status()
        except requests.exceptions.RequestException as e:  
            print(e)
            sys.exit(1) # really ? 
        
    @dryable.Dryable()
    def updateValidity(self, version: ObjectVersion, validFrom: str, validTo: str):    
        if version.validToTimestamp == validTo:
            logging.debug("The new timestamp for validTo is identical to the existing one. Skipping.")
            return
        url_update_validity = self.url + '/' + version.path + '/' + validFrom + '/' + validTo
        logging.info(f"Update end limit validity of {version.path} from {version.validToTimestamp} to {validTo}")
        try:
            r = requests.put(url_update_validity)
            r.raise_for_status()
        except requests.exceptions.RequestException as e:  
            print(e)
            sys.exit(1) # really ? 
        
    
def main():
    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    objectsList = ccdb.getObjectsList()
    print(f"{objectsList}")

if __name__== "__main__": # to be able to run the test code above when not imported.
    main()
