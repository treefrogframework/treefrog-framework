TEMPLATE = subdirs
CONFIG  += testcase
SUBDIRS  = htmlescape httpheader hmac htmlparser
SUBDIRS += mailmessage multipartformdata  smtpmailer viewhelper paginator
SUBDIRS += fieldnametovariablename rand urlrouter urlrouter2
SUBDIRS += sharedmemorylogstream buildtest stack queue forlist

greaterThan(QT_MAJOR_VERSION, 4) {
  SUBDIRS += jscontext
}

fwtests.target = test
fwtests.commands = make check
QMAKE_EXTRA_TARGETS += fwtests
