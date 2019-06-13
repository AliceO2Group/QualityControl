[![aliBuild](https://img.shields.io/badge/aliBuild-dashboard-lightgrey.svg)](https://alisw.cern.ch/dashboard/d/000000001/main-dashboard?orgId=1&var-storagename=All&var-reponame=All&var-checkname=build%2FQualityControl%2Fo2-dataflow%2F0&var-upthreshold=30m&var-minuptime=30)
[![JIRA](https://img.shields.io/badge/JIRA-Report%20issue-blue.svg)](https://alice.its.cern.ch/jira/secure/CreateIssue.jspa?pid=11201&issuetype=1)


# QC on FLP

## Basic principles of the QC system

The ITS Quality Control during the commissioning at B167 reads raw data files, perform the analysis, and publish them to the database and GUI interface. At present QC runs as a service and checks if there is a new folder for a new run in a dedicated directory. If there is a new folder Run*, the histograms will be reset reset. The string * in the folder name is being used to tag the processed data in the database. If a new data file is detected in the folder, the data will be processed and the histograms updated. 

## Installing QC with Alibuild

The commands to install QC with Alibuild is shown as follows. You can make it into a shell script and run it:

git clone https://github.com/alisw/alidist.git alidist

git clone https://github.com/alisw/alibuild.git alibuild

git clone -b DataSampling https://github.com/MYOMAO/AliceO2.git O2

git clone https://github.com/MYOMAO/QualityControl.git QualityControl

alibuild/aliBuild build O2 --defaults o2

alibuild/aliBuild build QualityControl --default o2


## Using QC to display data files

In principle, QC is always running at FLP01 and you do not need to start the QC. To check if the QC is running, we can do the following steps:

### Step 1: Login to FLP01, go to the QC directory, and setup the QC enviornment


To work on the FLP, login in as follow

ssh -Y its@flpits1

Go to the work folder:

cd  /home/its/QCGeneral/workdir/

Run the QC on the run you desire to analyze. For example, run000184:

source run_ITS_QC.sh run000184

Wait for about 1 minutes, you should be able to see the histogram of that run uploaded to the database: http://ccdb-test.cern.ch:8080/browse/ITSRAWDS
And can be found on the GUI: https://qcg-test.cern.ch/?page=layoutList
 
Some test data are already available in the /home/its/zshi/workdir/infiles/. To have them processed again, it is enough to copy the run folder our of the infiles directory and copy it back again after > 1 min.
 
In case that the GUI does not update, some troubleshooting can be done:

### Step 2: Check the QC Task Status

To check the status of QC Task, simply go to the link:

https://qcg-test.cern.ch/?page=layoutShow&layoutId=5cf76c9a13e837a4dd3fd841&layoutName=%28DS%29+QC+Process+Infomation

If you see the QC Task light is green, that mean QC is running and processing a file.

If you see the QC Task is red, that means QC is waiting for a new file. At this point, the shifters should record decoding error and the 2D Hit maps.

When the QC Task is red and if you inject a new file and it it still red, that mean QC is not running and shifters will need to run the trouble shooting as below.

### Step 3: Check if the QC task exists

Now, you are in the work directory. First check if QC task exists. Do:

ps -A | grep qcRunGeneral


You should see 5 qcRunGeneral with different RunID and ? as the owner

33273 ?        00:00:01 qcRunGeneral

33291 ?        00:01:10 qcRunGeneral

33292 ?        00:07:36 qcRunGeneral

33293 ?        00:10:10 qcRunGeneral

33294 ?        00:10:07 qcRunGeneral

If you do not see this, you need to restart the run. Go to the restarting QC section to see how to restart the QC


### Step 4: Check if the QC task runs properly

First, see what files are available in the folder "infiles/Run1"
 
ls infiles/Run1
 
You should see some files. For my case
 
Split2.bin  Split3.bin Split5.bin  Split9.bin
 
Now create a pipe:

mkfifo data-link2

Inject the lz4 file to the pipe

lz4 -d -c -f data-link2.lz4 > data-link2
 
Now you should see your terminal is hanging there.

Now start a new terminal go to the directory /home/its/zshi/workdir/ 

Now you can move the file data-link2 to the checking folder infiles/Run1

mv data-link2 infiles/Run1
 
Now go back to the GUI and go to the layout "Current File Processing". Wait for a 1 - 3 minutes and refresh the page
 
You should be able to see the title "Current File Name: "infile/Run1/data-link2" (depending on the name of the files you copied to Run1)
 
If you do see that, that means QC is running properly.

If you do not see the update of the filename to you copied file. You will need restart the QC. Go to the restarting QC section to see how to restart the QC
 


## Restarting


In case that QC is not running. To restart the QC when it is not running, simply do the following commands:

Open a new terminal

ssh -Y its@flpits1

cd  /home/its/msitta/

alienv enter QualityControl/latest

killall -9 qcRunDPL

qcRunDPL &!

Then you can close the terminal. In your original terminal, repeat step 2 and step 3. If QC still does not work, you will need to contact QC experts by emailing: zzshi@mit.edu

## Changing the Configuration File

There is a config file called "Config.dat". The first value is the number of event send from the RawPixelReaderSpec to the QC per cycle. The second one turn on/off of the error tracker on the events basis. It is set to be 1 or 0. The last one is the folder path where the QC is checking constantly.
