## Developers' tips

This is a resource meant for the developers of the QC. Whenever we learn something useful we put it
here. It is not sanitized or organized. Just a brain dump.

### Release procedure / check list
1. Update the version number in [CMakeLists.txt](../CMakeLists.txt), commit and push
2. Prepare the release notes using the commits since the last release in github (see [this template](ReleaseNotesTemplate.md)).
3. Release in github, paste the release notes
4. A PR is automatically created in alidist
5. Once merged, send an email to alice-o2-wp7@cern.ch, alice-o2-qc-contact@cern.ch and alice-dpg-qa-tools@cern.ch to announce the new release. Use the email for the previous release as a template.

### Create a fix version
1. checkout last tagged version, e.g. `git checkout v0.26.1`
2. branch, e.g. `git checkout -b branch_v0.26.2`
2. push the branch upstream, e.g. `git push upstream -u branch_v0.26.2`
2. cherry-pick the commit from master, e.g. `git cherry-pick b187ddbe52058d53a9bbf3cbdd53121c6b936cd8`
3. change version in CMakeLists and commit
5. tag, e.g. `git tag -a v0.26.2 -m "v0.26.2"`
4. push the tag upstream, e.g. `git push upstream v0.26.2`

### Where and how to configure the repo_cleaner of the ccdb-test

The config file is stored in git in the branch `repo_cleaner` (careful not to update in master instead !). Check out the branch, update the file Framework/script/RepoCleaner/config.yaml and commit it. A PR is necessary but in case of emergency, force-merge it. As soon as it is merged, it will be used by the script.

The config file used to be in `aldaqci@aidrefflp01:~/alice-ccdb/config.yaml` but it is not the case any more.

The repo_cleaner is launched every 5 minutes by [Jenkins](https://alijenkins.cern.ch/job/FLP/job/CCDB%20Clean%20up/).

Documentation of the repo_cleaner can be found [here](../Framework/script/RepoCleaner/README.md).

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

### Logging

We use the infologger. There is a utility class, `QcInfoLogger`, that can be used. It is a singleton. See [the header](../Framework/include/QualityControl/QcInfoLogger.h) for its usage.

Related issues : https://alice.its.cern.ch/jira/browse/QC-224

### Service Discovery (Online mode)

Service discovery (Online mode) is used to list currently published objects by running QC tasks. It uses Consul to store:
 - List of running QC tasks that respond to health check, known as "services" in Consul
 - List of published object by each QC task ("service"), knows as "tags" of a "service" in Consul

Both lists are updated from within QC task using [Service Discovery C++ API](#Service-Discovery-C++-API-and-Consul-HTTP-API):
- `register` - when a tasks starts
- `deregister` - when tasks ends

#### Register (and health check)
When a QC task starts, it register its presence in Consul by calling [register endpoit of Consul HTTP API](https://www.consul.io/api/agent/service.html#register-service). The request needs the following fields:
- `Id` - Task ID (must be unique)
- `Name` - Task name, tasks can have same name when they run on mutiple machines
- `Tags` - List of published objects
- `Checks` - Array of health check details for Consul, each should contain `Name`, `Interval`, type of check with endpoint to be check by Consul (eg. `"TCP": "localhost:1234"`) and `DeregisterCriticalServiceAfter` that defines timeout to automatically deregister service when fails health checks (minimum value `1m`).

#### Deregister
In order to deregister a service [`deregister/:Id` endpoint of Consul HTTP API](https://www.consul.io/api/agent/service.html#deregister-service) needs to be called. It does not need any additional parameters.
