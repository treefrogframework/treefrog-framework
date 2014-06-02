TEMPLATE = subdirs
SUBDIRS = htmlescape httpheader hmac sharedmemorylogstream htmlparser mailmessage  multipartformdata  smtpmailer viewhelper paginator fieldnametovariablename rand urlrouter

greaterThan(QT_MAJOR_VERSION, 4) {
  SUBDIRS += buildtest
}
