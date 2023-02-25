#!/bin/bash

FILE_PATH=src/tglobal.h

cd `dirname $0`
. ./tfbase.pri

function format
{
  if [ $1 -lt 16 ]; then
    echo -n "0"
  fi
  echo "$(printf %x $1)"
}

sed -i -e "s/auto TF_VERSION_STR = \S*/auto TF_VERSION_STR = \"${TF_VER_MAJ}\.${TF_VER_MIN}\.${TF_VER_PAT}\";/" $FILE_PATH
sed -i -e "s/auto TF_VERSION_NUMBER = \S*/auto TF_VERSION_NUMBER = 0x$(format ${TF_VER_MAJ})$(format ${TF_VER_MIN})$(format ${TF_VER_PAT});/" $FILE_PATH
sed -i -e "s/AssemblyVersionAttribute(\".*\")/AssemblyVersionAttribute(\"${TF_VER_MAJ}\.${TF_VER_MIN}\.${TF_VER_PAT}\")/" installer/treefrog-setup/treefrog-setup/AssemblyInfo.cpp
sed -i -e "s/set VERSION=\S*/set VERSION=${TF_VER_MAJ}\.${TF_VER_MIN}\.${TF_VER_PAT}/" configure.bat
sed -i -e "s/set VERSION=\S*/set VERSION=${TF_VER_MAJ}\.${TF_VER_MIN}\.${TF_VER_PAT}/" installer/create_installer.bat

[ which git >/dev/null 2>&1 ] && exit 1

PROJ_REV=`git rev-list HEAD | wc -l`

if [ -n "$PROJ_REV" ]; then
  REV=`expr $PROJ_REV + 1`
  sed -i -e "s/auto TF_SRC_REVISION.*/auto TF_SRC_REVISION = ${REV};/" $FILE_PATH
  echo "revision string updated : ${REV}"
fi
