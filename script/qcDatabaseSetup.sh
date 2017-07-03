#!/usr/bin/env bash

# TODO : use a python script

echo "Configuration of MySQL database for Quality Control"

QC_DB_MYSQL_HOST="localhost"
QC_DB_MYSQL_DBNAME="quality_control"
QC_DB_MYSQL_USER="qc_user"
QC_DB_MYSQL_PASSWORD="qc_user"

if [ "$QC_DB_MYSQL_ROOT_PASSWORD" != "" ]; then
  QC_DB_MYSQL_ROOT_PASSWORD="-p$QC_DB_MYSQL_ROOT_PASSWORD"
fi

mysqladmin -uroot $QC_DB_MYSQL_ROOT_PASSWORD ping | grep -q "is alive"
if [ "$?" != "0" ]; then
  echo "Cannot authenticate with MySQL. If the root password of your MySQL instance is not empty, please provide it with the environment variable QC_DB_MYSQL_ROOT_PASSWORD."
  exit 3
fi

# Create database
mysql -h $QC_DB_MYSQL_HOST -u root $QC_DB_MYSQL_ROOT_PASSWORD -e "create database \`$QC_DB_MYSQL_DBNAME\`"

# try connection
mysql -h $QC_DB_MYSQL_HOST -u root $QC_DB_MYSQL_ROOT_PASSWORD -e "exit"
if [ "$?" != "0" ]; then
  echo "Database could not be created."
  exit 4
else
	echo "Database successfully created."
fi

# Create account
#HERE=`hostname -f`
#   grant INSERT,SELECT,UPDATE,DELETE on $QC_DB_MYSQL_DBNAME.* to \"$QC_DB_MYSQL_USER\"@\"%\" identified by \"$QC_DB_MYSQL_PASSWORD\";
#  grant all privileges on $QC_DB_MYSQL_DBNAME.* to \"$QC_DB_MYSQL_USER\"@\"$HERE\" identified by \"$QC_DB_MYSQL_PASSWORD\";
mysql -h $QC_DB_MYSQL_HOST -u root $QC_DB_MYSQL_ROOT_PASSWORD -e "
  grant all privileges on $QC_DB_MYSQL_DBNAME.* to \"$QC_DB_MYSQL_USER\"@\"localhost\" identified by \"$QC_DB_MYSQL_PASSWORD\";
"

# try connection
mysql -h $QC_DB_MYSQL_HOST -u $QC_DB_MYSQL_USER -p$QC_DB_MYSQL_PASSWORD -e "exit"
if [ "$?" != "0" ]; then
  echo "Failure to create user $QC_DB_MYSQL_USER."
  exit 5
else
	echo "User $QC_DB_MYSQL_USER created."
fi
