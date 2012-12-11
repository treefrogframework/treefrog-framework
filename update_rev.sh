#!/bin/sh

FILE_PATH=src/tglobal.h

cd `dirname $0`
. ./tfbase.pri

sed -i -e "s/^#define TF_VERSION_NUMBER.*/#define TF_VERSION_NUMBER 0x0${TF_VER_MAJ}0${TF_VER_MIN}0${TF_VER_PAT}/" $FILE_PATH
sed -i -e "s/^#define TF_VERSION_STR.*/#define TF_VERSION_STR \"${TF_VER_MAJ}\.${TF_VER_MIN}\.${TF_VER_PAT}\"/" $FILE_PATH


[ which git >/dev/null 2>&1 ] && exit 1

PROJ_REV=`git rev-list HEAD | wc -l`

if [ -n "$PROJ_REV" ]; then
  REV=`expr $PROJ_REV + 1`
  sed -i -e "s/^#define TF_SRC_REVISION.*/#define TF_SRC_REVISION $REV/" $FILE_PATH
  echo "revision string updated : $REV"
fi
