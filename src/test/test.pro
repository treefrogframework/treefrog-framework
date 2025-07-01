TEMPLATE = subdirs
CONFIG  += testcase
SUBDIRS  = htmlescape httpheader htmlparser
SUBDIRS += mailmessage multipartformdata  smtpmailer viewhelper paginator
SUBDIRS += fieldnametovariablename rand urlrouter urlrouter2
SUBDIRS += buildtest stack queue forlist
SUBDIRS += jscontext compression sqlitedb url malloc
SUBDIRS += sharedmemory sharedmemoryhash sharedmemorymutex
unix {
  SUBDIRS += redis memcached
}

fwtests.target = test
fwtests.commands = make check
QMAKE_EXTRA_TARGETS += fwtests
