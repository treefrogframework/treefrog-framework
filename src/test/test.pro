TEMPLATE = subdirs
SUBDIRS  = htmlescape httpheader hmac htmlparser
SUBDIRS += mailmessage  multipartformdata  smtpmailer viewhelper paginator
SUBDIRS += fieldnametovariablename rand urlrouter currenttime urlrouter

!msvc {
  SUBDIRS += sharedmemorylogstream buildtest
}
