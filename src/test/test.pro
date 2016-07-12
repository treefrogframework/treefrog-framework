TEMPLATE = subdirs
CONFIG  += testcase
SUBDIRS  = htmlescape httpheader hmac htmlparser
SUBDIRS += mailmessage multipartformdata  smtpmailer viewhelper paginator
SUBDIRS += fieldnametovariablename rand urlrouter urlrouter2 jscontext
SUBDIRS += sharedmemorylogstream buildtest

fwtests.target = test
fwtests.commands = make check
QMAKE_EXTRA_TARGETS += fwtests
