## Developers' tips

This is a resource meant for the developers of the QC. Whenever we learn something useful we put it
here. It is not sanitized or organized. Just a brain dump.

### Trick used to load old data
Until version 3 of the class MonitorObject, objects were stored in the repository directly. They are now stored within TFiles. The issue with the former way is that the StreamerInfo are lost. To be able to load old data, the StreamerInfos have been saved in a root file "streamerinfos.root". The CcdbDatabase access class loads this file and the StreamerInfos upon creation which allows for a smooth reading of the old objects. The day we are certain nobody will add objects in the old format and that the old objects have been removed from the database, we can delete this file and remove the loading from CcdbDatabase. Moreover, the following lines can be removed : 
```
// We could not open a TFile we should now try to open an object directly serialized
object = ccdbApi.retrieve(path, metadata, getCurrentTimestamp());
cout << "We could retrieve the object " << path << " as a streamed object." << endl;
if(object == nullptr) {
  return nullptr;
}
```

### Doxygen generation

To generate locally the doxygen doc, do `cd sw/BUILD/QualityControl-latest/QualityControl; make doc`.
It will be available in doc/html, thus to open it quickly do `[xdg-]open doc/html/index.html`.

### Monitoring debug

When we don't see the monitoring data in grafana, here is what to do to pinpoint the source of the problem.

1. `ssh root@aido2mon-gpn.cern.ch`
2. See if the data reach the servers :
    1. `systemctl stop influxdb`
    2. `nc -u -l 8087`  <-- make sure to use the proper port (cf your monitoring url)
    3. The metrics should appear in clear in the terminal as they arrive. If not, check the monitoring url in your code/config.
    4. systemctl start influxdb
3. If they arrive, we want to check whether the metrics are stored
    1. emacs /etc/influxdb/influxdb.conf
        1. Search for the port you are using
        2. Note the name of the database (for 8087 it is "qc")
    2. `influx`
    2. `show databases`
    3. `use qc`  <-- use the proper database as found in influxdb.conf
    4. `show series`
    5. `select count(*) from cpuUsedPercentage` <-- use the correct metrics name
    6. Repeat the last command and see if the number increases. If it increases it denotes that the metrics is stored correctly in the database. If it is the case, the problem lies in your grafana.

### Avoid writing QC objects to a repository

In case of a need to avoid writing QC objects to a repository, one can choose the "Dummy" database implementation in the config file. This is might be useful when one expects very large amounts of data that would be stored, but not actually needed (e.g. benchmarks).

### QCG 

#### Generalities

The QCG server for qcg-test.cern.ch is hosted on qcg@barth-qcg. The config file is `/home/qcg/QualityControlGui/config.js`.

#### Access rights

Any one in alice-member has access. We use the egroup alice-o2-qcg-access to grant access or not and this egroup contains alice-member plus a few extra. This allows for non ALICE members to access the QCG. 

#### Start and stop

`systemctl restart qcg`
