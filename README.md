## Setup

Install devtoolset-6:

```
yum install centos-release-scl
yum install devtoolset-6
source /opt/rh/devtoolset-6/enable # to be added to your .bashrc
```

For development we use alibuild (see [here](https://alisw.github.io/alibuild/o2-tutorial.html) for details). 

```
# Install alibuild
sudo pip install alibuild==1.4.0 # or a later version

# Check out flpproto repository
mkdir -p $HOME/alice
cd $HOME/alice
aliBuild init flpproto

# Check what dependencies you are missing and you would like to add.
aliDoctor --defaults o2-daq flpproto

# Build dependencies and flp prototype
aliBuild --defaults o2-daq build flpproto
```

### Post installation 

Load environment to use or develop (add it to .bashrc if you wish)

    ALICE_WORK_DIR=$HOME/alice/sw; eval "`alienv shell-helper`" 
    alienv load flpproto/latest

Start and populate database : 

```
sudo systemctl start mariadb
qcDatabaseSetup.sh
```

## QC Configuration

The QC requires a number of configuration items. An example config file is
provided in the repo under the name _example-default.ini_. Moreover, the
outgoing channel over which histograms are sent is defined in a JSON
file such as the one called _alfa.json_ in the repo.

**QC tasks** must be defined in the configuration :

```
[myTask_1]                      # define parameters for task myTask_1
taskDefinition=taskDefinition_1 # indirection to the actual definition
```

We use an indirect system to allow multiple tasks to share
most of their definition.

```
[taskDefinition_1]
className=AliceO2::QualityControlModules::Example::ExampleTask
moduleName=QcExample     # which library contains the class
cycleDurationSeconds=1
```

The JSON file contains a typical FairMQ device definition. One can
 change the or the address there:
```
{
    "fairMQOptions": {
        "devices": [
            {
                "id": "myTask_1",
                "channels": [
                    {
                        "name": "data-out",
                        "sockets": [
                            {
                                "type": "pub",
                                "method": "bind",
                                "address": "tcp://*:5556",
                                "sndBufSize": 10000,
                                "rcvBufSize": 10000,
                                "rateLogging": 0
                            }
                        ]
                    }
                ]
            }
        ]
    }
}
```

**QC checkers** are defined in the config file :

```
[checker_0]
broadcast=0 # whether to broadcast the plots in addition to storing them
broadcastAddress=tcp://*:5600
id=0        # only required item
```

And for the time, the mapping between checkers and tasks must be explicit :

```
[checkers] ; needed for the time being because we don't have an information service
numberCheckers=1
; can be less than the number of items in tasksAddresses
numberTasks=1
tasksAddresses=tcp://localhost:5556,tcp://localhost:5557,tcp://localhost:5558,tcp://localhost:5559
```

There are configuration items for many other aspects, for example the
database connection, the monitoring or the data sampling.

## Execution

### Basic
With the default config files provided, we run a qctask that processes
random data and forwards it to a GUI and a checker that will check and
store the MonitorObjects.

```
# Launch a task named myTask_1
qcTaskLauncher -n myTask_1 \
               -c file:///absolute/path/to/example-default.ini \
               --id myTask_1 \
               --mq-config path/to/alfa.json

# Launch a gui to check what is being sent
# don't forget to click the button Start !
qcSpy

# Launch a checker named checker_0
qcCheckerLauncher -c file:///absolute/path/to/example-default.ini -n checker_0
```

### Readout chain

1. Modify the config file of readout (Readout/configDummy.cfg) :
```
[readout]
exitTimeout=-1
(...)
[sampling]
# enable/disable data sampling (1/0)
enabled=1
# which class of datasampling to use (FairInjector, MockInjector)
class=FairInjector
```
2. Start Readout
```
readout.exe file:///absolute/path/to/configDummy.cfg
```
3. Modify the config file of the QC (QualityControl/example-default.ini) :
```
[DataSampling]
implementation=FairSampler
```
4. Launch the QC task
```
qcTaskLauncher -n myTask_1 \
               -c file:///absolute/path/to/configDummy.cfg \
               --id myTask_1 \
               --mq-config path/to/alfa.json
```
5. Launch the checker
```
qcCheckerLauncher -c file:///absolute/path/to/example-default.ini -n checker_0
```
6. Launch the gui
```
qcSpy file:///absolute/path/to/example-default.ini
# then click Start !
```
In the gui, one can change the source and point to the checker by
changing the path in the field at the bottom of the window (first click
`Stop` and `Start` after filling in the details). The port of the checker
is 5600.
One can also check what is stored in the database by clicking `Stop`
and then switching to `Database` source. This will only work if a config
file was passed to the `qcSpy` utility.
