#!/usr/bin/sh

# TODO : use a python script

echo "Configuration of MySQL database for Quality Control"

QC_DB_MYSQL_HOST="localhost"
QC_DB_MYSQL_DBNAME="quality_control"
QC_DB_MYSQL_USER="qc_user"
QC_DB_MYSQL_PASSWORD="qc_user"

stty -echo
read -p "Please enter root password for mysql : " PWD
stty echo
echo
if [ "$PWD" != "" ]; then
  PWD="-p$PWD"
fi

# Create database
mysql -h $QC_DB_MYSQL_HOST -u root $PWD -e "create database \`$QC_DB_MYSQL_DBNAME\`"

# try connection
mysql -h $QC_DB_MYSQL_HOST -u root $PWD -e "exit"
if [ "$?" != "0" ]; then 
  echo "Database could not be created."
  exit
else
	echo "Database successfully created."
fi

# Create account
#HERE=`hostname -f`
#   grant INSERT,SELECT,UPDATE,DELETE on $QC_DB_MYSQL_DBNAME.* to \"$QC_DB_MYSQL_USER\"@\"%\" identified by \"$QC_DB_MYSQL_PASSWORD\";
#  grant all privileges on $QC_DB_MYSQL_DBNAME.* to \"$QC_DB_MYSQL_USER\"@\"$HERE\" identified by \"$QC_DB_MYSQL_PASSWORD\";
mysql -h $QC_DB_MYSQL_HOST -u root $PWD -e "
  grant all privileges on $QC_DB_MYSQL_DBNAME.* to \"$QC_DB_MYSQL_USER\"@\"localhost\" identified by \"$QC_DB_MYSQL_PASSWORD\";
"

# try connection
mysql -h $QC_DB_MYSQL_HOST -u $QC_DB_MYSQL_USER -p$QC_DB_MYSQL_PASSWORD -e "exit"
if [ "$?" != "0" ]; then 
  echo "Failure to create user $QC_DB_MYSQL_USER."
  exit
else
	echo "User $QC_DB_MYSQL_USER created."
fi