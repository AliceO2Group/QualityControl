import datetime
import logging
import sys
from typing import List

import dryable
import requests


class ObjectVersion:
    '''
    A version of an object in the CCDB. 
    
    In the CCDB an object can have many versions with different validity intervals. 
    This class represents a single version. 
    '''

    def __init__(self, path, uuid, validFrom, validTo):
        '''
        Construct an ObjectVersion.
        :param path: path to the object
        :param uuid: unique id of the object
        :param validFromAsDatetime: validity range smaller limit (in ms)
        :param validTo: validity range bigger limit (in ms)
        '''
        self.path = path
        self.uuid = uuid
        self.validFromAsDatetime = datetime.datetime.fromtimestamp(int(validFrom) / 1000)  # /1000 because we get ms
        self.validFrom = validFrom
        self.validTo = validTo
        
    def __repr__(self):
        return f"Version of object {self.path} valid from {self.validFromAsDatetime} (uuid {self.uuid}, ts {self.validFrom})"


class Ccdb:
    '''
    Class to interact with the CCDB.
    '''
    
    counter_deleted: int = 0
    counter_validity_updated: int = 0
    counter_preserved: int = 0

    def __init__(self, url):
        logging.info(f"Instantiate CCDB at {url}")
        self.url = url

    def getObjectsList(self) -> List[str]:
        '''
        Get the full list of objects in the CCDB. 
        
        :return A list of strings, each containing a path to an object in the CCDB.
        '''
        url_for_all_obj = self.url + '/latest/.*'
        logging.debug(f"Ccdb::getObjectsList -> {url_for_all_obj}")
        headers = {'Accept':'application/json'}
        r = requests.get(url_for_all_obj, headers=headers)
        r.raise_for_status()
        json = r.json()
        paths = []
        for item in json['objects']:
            paths.append(item['path'])
        return paths

    def getVersionsList(self, object_path: str) -> List[ObjectVersion]:
        '''
        Get the list of all versions for a given object.
        :param object_path: Path to the object for which we want the list of versions.
        :return A list of ObjectVersion.
        '''
        url_browse_all_versions = self.url + '/browse/' + object_path
        logging.debug(f"Ccdb::getVersionsList -> {url_browse_all_versions}")
        headers = {'Accept':'application/json'}
        r = requests.get(url_browse_all_versions, headers=headers)
        r.raise_for_status()
        try:
            json_result = r.json(strict=False) # to survive bad characters in the strings of the json
        except ValueError as e:
            print(f"Error while reading json for object {object_path} from CCDB: {e}")
            exit(1)
        versions = []
        for object_path in json_result['objects']:
            version = ObjectVersion(path=object_path['path'], uuid=object_path['id'], validFrom=object_path['validFrom'], validTo=object_path['validUntil'])
            versions.insert(0, version)
        return versions

    @dryable.Dryable()
    def deleteVersion(self, version: ObjectVersion):
        '''
        Delete the specified version of an object. 
        :param version: The version of the object to delete, as an instance of ObjectVersion.
        '''
        url_delete = self.url + '/' + version.path + '/' + version.validFrom + '/' + version.uuid
        logging.debug(f"Delete version at url {url_delete}")
        try:
            r = requests.delete(url_delete)
            r.raise_for_status()
            self.counter_deleted += 1
        except requests.exceptions.RequestException as e:  
            print(e)
            sys.exit(1)  # really ? 
        
    @dryable.Dryable()
    def updateValidity(self, version: ObjectVersion, validFrom: str, validTo: str):    
        '''
        Update the validity range of the specified version of an object.
        :param version: The ObjectVersion to update.
        :param validFrom: The new "from" validity.
        :param validTo: The new "to" validity.
        '''
        if version.validTo == validTo:
            logging.debug("The new timestamp for validTo is identical to the existing one. Skipping.")
            return
        url_update_validity = self.url + '/' + version.path + '/' + validFrom + '/' + validTo
        logging.debug(f"Update end limit validity of {version.path} from {version.validTo} to {validTo}")
        try:
            r = requests.put(url_update_validity)
            r.raise_for_status()
            self.counter_validity_updated += 1
        except requests.exceptions.RequestException as e:  
            print(e)
            sys.exit(1)  # really ? 
        
    
def main():
    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    objectsList = ccdb.getObjectsList()
    print(f"{objectsList}")


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
