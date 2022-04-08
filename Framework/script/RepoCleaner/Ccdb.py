import datetime
import logging
import sys
from typing import List

import dryable
import requests


logger = logging  # default logger


class ObjectVersion:
    '''
    A version of an object in the CCDB. 
    
    In the CCDB an object can have many versions with different validity intervals. 
    This class represents a single version. 
    '''

    def __init__(self, path, validFrom, validTo, uuid=None, metadata=None):
        '''
        Construct an ObjectVersion.
        :param path: path to the object
        :param uuid: unique id of the object
        :param validFrom: validity range smaller limit (in ms)
        :param validTo: validity range bigger limit (in ms)
        '''
        self.path = path
        self.uuid = uuid
        self.validFrom = validFrom
        # precomputed Datetime ("Dt") of the timestamp `validFrom`
        self.validFromAsDt = datetime.datetime.fromtimestamp(int(validFrom) / 1000)  # /1000 because we get ms
        self.validTo = validTo
        self.metadata = metadata
        
    def __repr__(self):
        if "Run" in self.metadata or "RunNumber" in self.metadata:
            run_number = self.metadata["Run"] if "Run" in self.metadata else self.metadata["RunNumber"]
            return f"Version of object {self.path} valid from {self.validFromAsDt}, run {run_number}"
        else:
            return f"Version of object {self.path} valid from {self.validFromAsDt} (uuid {self.uuid}, " \
                   f"ts {self.validFrom})"


class Ccdb:
    '''
    Class to interact with the CCDB.
    '''
    
    counter_deleted: int = 0
    counter_validity_updated: int = 0
    counter_preserved: int = 0

    def __init__(self, url):
        logger.info(f"Instantiate CCDB at {url}")
        self.url = url

    def getObjectsList(self, added_since: int = 0) -> List[str]:
        '''
        Get the full list of objects in the CCDB that have been created since added_since.

        :param added_since: if specified, only return objects added since this timestamp in epoch milliseconds.
        :return A list of strings, each containing a path to an object in the CCDB.
        '''
        logger.debug(f"added_since : {added_since}")
        url_for_all_obj = self.url + '/latest/.*'
        logger.debug(f"Ccdb::getObjectsList -> {url_for_all_obj}")
        headers = {'Accept':'application/json', 'If-Not-Before':str(added_since)}
        r = requests.get(url_for_all_obj, headers=headers)
        r.raise_for_status()
        try:
            json = r.json()
        except JSONDecodeError as err:
            logger.error(f"JSON decode error: {err}")
            raise
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
        logger.debug(f"Ccdb::getVersionsList -> {url_browse_all_versions}")
        headers = {'Accept':'application/json', 'Connection': 'close'}
        r = requests.get(url_browse_all_versions, headers=headers)
        r.raise_for_status()
        try:
            json_result = r.json(strict=False) # to survive bad characters in the strings of the json
        except ValueError as e:
            print(f"Error while reading json for object {object_path} from CCDB: {e}")
            exit(1)
        versions = []
        for object_path in json_result['objects']:
            # print(f"\n***object_path : {object_path}")
            version = ObjectVersion(path=object_path['path'], uuid=object_path['id'], validFrom=object_path['validFrom'], validTo=object_path['validUntil'], metadata=object_path)
            versions.insert(0, version)
        return versions

    @dryable.Dryable()
    def deleteVersion(self, version: ObjectVersion):
        '''
        Delete the specified version of an object. 
        :param version: The version of the object to delete, as an instance of ObjectVersion.
        '''
        url_delete = self.url + '/' + version.path + '/' + str(version.validFrom) + '/' + version.uuid
        logger.debug(f"Delete version at url {url_delete}")
        headers = {'Connection': 'close'}
        try:
            r = requests.delete(url_delete, headers=headers)
            r.raise_for_status()
            self.counter_deleted += 1
        except requests.exceptions.RequestException as e:  
            print(e)
            sys.exit(1)  # really ? 
        
    @dryable.Dryable()
    def updateValidity(self, version: ObjectVersion, valid_from: int, valid_to: int, metadata=None):
        '''
        Update the validity range of the specified version of an object.
        :param version: The ObjectVersion to update.
        :param valid_from: The new "from" validity.
        :param valid_to: The new "to" validity.
        :param metadata: Add or modify metadata
        '''
        full_path = self.url + '/' + version.path + '/' + str(valid_from) + '/' + str(valid_to) + '/' + str(version.uuid) + '?'
        logger.debug(f"Update end limit validity of {version.path} ({version.uuid}) from {version.validTo} to {valid_to}")
        if metadata is not None:
            logger.debug(f"{metadata}")
            for key in metadata:
                full_path += key + "=" + metadata[key] + "&"
        try:
            headers = {'Connection': 'close'}
            r = requests.put(full_path, headers=headers)
            r.raise_for_status()
            self.counter_validity_updated += 1
        except requests.exceptions.RequestException as e:  
            print(e)
            sys.exit(1)  # really ?

    def putVersion(self, version: ObjectVersion, data):
        '''
        :param version: An ObjectVersion that describes the data to be uploaded.
        :param data: the actual data to send. E.g.:{'somekey': 'somevalue'}
        :return A list of ObjectVersion.
        '''
        full_path=self.url + "/" + version.path + "/" + str(version.validFrom) + "/" + str(version.validTo) + "/"
        if version.metadata is not None:
            for key in version.metadata:
                full_path += key + "=" + version.metadata[key] + "/"
        logger.debug(f"fullpath: {full_path}")
        headers = {'Connection': 'close'}
        r = requests.post(full_path, files=data, headers=headers)
        if r.ok:
            logger.debug(f"Version pushed to {version.path}")
        else:
            logger.error(f"Could not post a new version of {version.path}: {r.text}")

def main():
    logger.basicConfig(level=logger.INFO, format='%(asctime)s - %(levelname)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S')
    logger.getLogger().setLevel(int(10))

    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')

    data = {'somekey': 'somevalue'}
    metadata = {'RunNumber': '213564', 'test': 'on'}
    version_info = ObjectVersion(path="qc/TST/MO/repo/test", validFrom=1605091858183, validTo=1920451858183, metadata=metadata)
    ccdb.putVersion(version_info, data)


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
