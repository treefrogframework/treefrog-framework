HEADER_CLASSES = ../include/TAbstractModel ../include/TAbstractUser ../include/TActionContext ../include/TActionController ../include/TActionHelper ../include/TActionThread ../include/TActionView ../include/TPrototypeAjaxHelper ../include/TApplicationServerBase ../include/TThreadApplicationServer ../include/TPreforkApplicationServer ../include/TContentHeader ../include/TCookie ../include/TCookieJar ../include/TCriteria ../include/TCriteriaConverter ../include/TCryptMac ../include/TDirectView ../include/TDispatcher ../include/TGlobal ../include/THtmlAttribute ../include/THtmlParser ../include/THttpHeader ../include/THttpRequest ../include/THttpRequestHeader ../include/THttpResponse ../include/THttpResponseHeader ../include/THttpUtility ../include/TInternetMessageHeader ../include/TJavaScriptObject ../include/TLog ../include/TLogger ../include/TLoggerPlugin ../include/TMailMessage ../include/TModelUtil ../include/TMultipartFormData ../include/TOption ../include/TSession ../include/TSessionStore ../include/TSessionStorePlugin ../include/TSharedMemoryLogStream ../include/TSmtpMailer ../include/TSqlORMapper ../include/TSqlORMapperIterator ../include/TSqlObject ../include/TSqlQuery ../include/TSqlQueryORMapper ../include/TSystemGlobal ../include/TTemporaryFile ../include/TViewHelper ../include/TWebApplication ../include/TfException ../include/TfNamespace ../include/TreeFrogController ../include/TreeFrogModel ../include/TreeFrogView ../include/TAbstractController ../include/TActionMailer ../include/TFormValidator ../include/TSqlQueryORMapperIterator ../include/TAccessValidator ../include/TSqlTransaction ../include/TPaginator ../include/TKvsDatabase ../include/TKvsDriver ../include/TModelObject ../include/TPopMailer ../include/TMultiplexingServer ../include/TAccessLog ../include/TActionWorker ../include/TAtomicQueue ../include/TJsonUtil ../include/TScheduler ../include/TApplicationScheduler ../include/TCommandLineInterface ../include/TSendmailMailer ../include/TAppSettings ../include/TWebSocketEndpoint ../include/TDatabaseContext ../include/TDatabaseContextThread ../include/TWebSocketSession ../include/TRedis ../include/TSqlJoin ../include/THazardPtrManager ../include/TAtomic ../include/TAtomicPtr ../include/TDebug ../include/TBackgroundProcess ../include/TBackgroundProcessHandler

HEADER_FILES = tabstractmodel.h tabstractuser.h tactioncontext.h tactioncontroller.h tactionhelper.h tactionthread.h tactionview.h tprototypeajaxhelper.h tapplicationserverbase.h tthreadapplicationserver.h tpreforkapplicationserver.h tcontentheader.h tcookie.h tcookiejar.h tcriteria.h tcriteriaconverter.h tcryptmac.h tdirectview.h tdispatcher.h tfcore.h tfexception.h tfnamespace.h tglobal.h thtmlattribute.h thtmlparser.h thttpheader.h thttprequest.h thttprequestheader.h thttpresponse.h thttpresponseheader.h thttputility.h tinternetmessageheader.h tjavascriptobject.h tlog.h tlogger.h tloggerplugin.h tmailmessage.h tmodelutil.h tmultipartformdata.h toption.h tsession.h tsessionstore.h tsessionstoreplugin.h tsharedmemorylogstream.h tsmtpmailer.h tsqlobject.h tsqlormapper.h tsqlormapperiterator.h tsqlquery.h tsqlqueryormapper.h tsystemglobal.h ttemporaryfile.h tviewhelper.h twebapplication.h tabstractcontroller.h tactionmailer.h tformvalidator.h tsqlqueryormapperiterator.h taccessvalidator.h tsqltransaction.h tpaginator.h tkvsdatabase.h tkvsdriver.h tmodelobject.h tpopmailer.h tmultiplexingserver.h taccesslog.h tactionworker.h tatomicqueue.h tjsonutil.h tscheduler.h tapplicationscheduler.h tcommandlineinterface.h tsendmailmailer.h tappsettings.h twebsocketendpoint.h tdatabasecontext.h tdatabasecontextthread.h tsystembus.h tprocessinfo.h twebsocketsession.h tredis.h tsqljoin.h thazardptrmanager.h tatomic.h tatomicptr.h tdebug.h tbackgroundprocess.h tbackgroundprocesshandler.h

HEADER_FILES += tsqldatabasepool.h tkvsdatabasepool.h tstack.h thazardobject.h thazardptr.h

HEADER_CLASSES += ../include/TJSLoader
HEADER_FILES   += tjsloader.h
HEADER_CLASSES += ../include/TJSModule
HEADER_FILES   += tjsmodule.h
HEADER_CLASSES += ../include/TJSInstance
HEADER_FILES   += tjsinstance.h
HEADER_CLASSES += ../include/TReactComponent
HEADER_FILES   += treactcomponent.h

unix {
  HEADER_FILES += tfcore_unix.h
}

MONGODB_CLASSES = ../include/TMongoCursor ../include/TBson ../include/TMongoDriver ../include/TMongoQuery ../include/TMongoObject ../include/TMongoODMapper ../include/TCriteriaMongoConverter

MONGODB_FILES = tmongocursor.h tbson.h tmongodriver.h tmongoquery.h tmongoobject.h tmongoodmapper.h tcriteriamongoconverter.h

TEST_CLASSES = ../include/TfTest/TfTest

TEST_FILES = tftest.h
