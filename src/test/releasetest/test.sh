#!/bin/bash

# 
APPNAME=blogapp
BASEDIR=`pwd`/`dirname $0`
APPROOT=$BASEDIR/$APPNAME
WORKDIR=$APPROOT/_work.$$
DBFILE=$APPROOT/db/dbfile
RANDOMTXT="1234567890123456asdfghjklLKJHGFDSA_____"


# Function
row_count()
{
  echo "SELECT COUNT(1) FROM blog;" | sqlite3 $DBFILE
}

## Main ##

# Create app
cd $BASEDIR
if [ -d "$APPNAME" ]; then
  treefrog -k abort $APPNAME
  rm -rf $APPNAME
fi

tspawn new $APPNAME
echo "CREATE TABLE blog (id INTEGER PRIMARY KEY AUTOINCREMENT, title VARCHAR(20), body VARCHAR(200), created_at TIMESTAMP, updated_at TIMESTAMP);" | sqlite3 $DBFILE || exit

# Build
cd $APPROOT
tspawn s blog || exit
qmake -r || exit
make -j4
make || exit

# Start Check
treefrog -e dev -d || exit
sleep 1
treefrog -k abort || exit
sleep 1
treefrog -e dev -d || exit
sleep 1

# GET method
mkdir -p $WORKDIR
cd $WORKDIR
wget http://localhost:8800/blog/index || exit
wget http://localhost:8800/blog/entry || exit

# POST method
# create1
curl --data-urlencode 'blog[title]=hello' --data-urlencode 'blog[body]=Hello world.' http://localhost:8800/blog/create > create1 || exit
[ `row_count` = "1" ] || exit

wget http://localhost:8800/blog/show/1 || exit

# create2
curl --data-urlencode 'blog[title]=Hi!' --data-urlencode 'blog[body]=Hi, all' http://localhost:8800/blog/create > create2 || exit
[ `row_count` = "2" ] || exit

wget http://localhost:8800/blog/show/2 || exit

# update
curl --data-urlencode 'blog[id]=1' --data-urlencode 'blog[title]=Hi!' --data-urlencode "blog[body]=$RANDOMTXT" http://localhost:8800/blog/save/1 > save1 || exit
[ `row_count` = "2" ] || exit

# update check
wget http://localhost:8800/blog/show/1 -O show1 || exit
if ! grep "$RANDOMTXT" show1 >/dev/null 2>&1; then exit; fi

# delete
curl  --data-urlencode 'dummy'  http://localhost:8800/blog/remove/1  > remove1 || exit
[ `row_count` = "1" ] || exit

curl  --data-urlencode 'dummy'  http://localhost:8800/blog/remove/2  > remove2 || exit
[ `row_count` = "0" ] || exit


# Cleanup
cd $APPROOT
treefrog -k stop || exit

cd $BASEDIR
rm -rf $WORKDIR
rm -rf $APPNAME

echo
echo "Test completed."
echo "Congratulations!"

#EOF
