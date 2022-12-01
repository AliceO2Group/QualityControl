import logging

from qcrepocleaner.Ccdb import Ccdb


def process(ccdb: Ccdb, object_path: str, delay: int):
    '''
    This is the function one must implement. 
    
    It should take care of deleting extraneous or outdated versions of the object 
    located at 'object_path'. The object ccdb is used to interact with the CCDB. 
    The 'delay' is here to specify a grace period during which no objects should be 
    deleted. 

    :param ccdb: the ccdb in which objects are cleaned up.
    :param object_path: path to the object, or pattern, to which a rule will apply.
    :param delay: the grace period during which a new object is never deleted.
    '''
    
    logging.debug(f"Plugin XXX processing {object_path}")


# to quickly test, one can just run the plugin itself. 
def main():
    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    process(ccdb, "asdfasdf/example", 60)


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
