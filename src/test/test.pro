TEMPLATE = subdirs
CONFIG  += testcase
SUBDIRS  = htmlescape httpheader hmac htmlparser
SUBDIRS += mailmessage multipartformdata  smtpmailer viewhelper paginator
SUBDIRS += fieldnametovariablename rand urlrouter urlrouter2
SUBDIRS += sharedmemorylogstream buildtest stack queue forlist
SUBDIRS += jscontext compression sqlitedb url malloc
!mac {
  SUBDIRS += sharedmemoryhash sharedmemorymutex
}
unix {
  SUBDIRS += redis memcached
}

fwtests.target = test
fwtests.commands = make check
QMAKE_EXTRA_TARGETS += fwtests
