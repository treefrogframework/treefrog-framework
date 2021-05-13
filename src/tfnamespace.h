#pragma once
class TWebApplication;

/*!
  \namespace Tf
  \brief The Tf namespace contains miscellaneous identifiers used
  throughout the library of TreeFrog Framework.
*/
namespace Tf {
enum QuotedStrSplitBehavior {
    SplitWhereverSep = 0,
    SplitSkipQuotedString,
};

enum CaseSensitivity {
    CaseInsensitive = Qt::CaseInsensitive,
    CaseSensitive = Qt::CaseSensitive,
};

enum HttpMethod {
    Invalid = 0,
    Get,
    Head,
    Post,
    Options,
    Put,
    Delete,
    Trace,
    Connect,
    Patch,
};

enum HttpStatusCode {
    // Informational 1xx
    Continue = 100,
    SwitchingProtocols = 101,
    // Successful 2xx
    OK = 200,
    Created = 201,
    Accepted = 202,
    NonAuthoritativeInformation = 203,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,
    // Redirection 3xx
    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    UseProxy = 305,
    TemporaryRedirect = 307,
    // Client Error 4xx
    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthenticationRequired = 407,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    RequestEntityTooLarge = 413,
    RequestURITooLong = 414,
    UnsupportedMediaType = 415,
    RequestedRangeNotSatisfiable = 416,
    ExpectationFailed = 417,
    // Server Error 5xx
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HTTPVersionNotSupported = 505,
};

// Common options for AJAX
enum AjaxOption {
    Asynchronous = 0,  // true or false, default:true
    ContentType,  // default:"application/x-www-form-urlencoded"
    Encoding,  // default:"UTF-8"
    Method,  // Tf::Get or Tf::Post, default:Post
    Parameters,  // Default:null
    PostBody,  // Specific contents for the request body on a 'post' method
    RequestHeaders,  // See Prototype API docs
    EvalJS,  // true or false, default:true
    EvalJSON,  // true or false, default:true
    SanitizeJSON,  // true or false, See Prototype API docs
};

enum AjaxEvent {
    Create = 100,  // Before request is initiated
    Uninitialized,  // Immediately after request is initiated and before loading
    Loading,  // When the remote response is being loaded by the browser
    Loaded,  // When the browser has finished loading the remote response
    Interactive,  // When the user can interact with the remote response, even though it has not finished loading
    Success,  // When the XMLHttpRequest is completed, and the HTTP status code is in the 2XX range
    Failure,  // When the XMLHttpRequest is completed, and the HTTP status code is not in the 2XX range
    Complete,  // When the XMLHttpRequest is complete (fires after success or failure, if they are present)
};

enum ValidationRule {
    Required = 0,  // This value is required.
    MaxLength,  // This value is too long.
    MinLength,  // This value is too short.
    IntMax,  // This value is too big.
    IntMin,  // This value is too small.
    DoubleMax,  // This value is too big.
    DoubleMin,  // This value is too small.
    EmailAddress,  // This value is not email address.
    Url,  // This value is invalid URL.
    Date,  // This value is invalid date.
    Time,  // This value is invalid time.
    DateTime,  // This value is invalid date or time.
    Pattern,  // This value is bad format.
    Custom,

    // Add new rules before this line
    RuleCount
};

enum EscapeFlag {
    Compatible = 0,  // Converts double-quotes and leaves single-quotes alone.
    Quotes,  // Converts both double and single quotes.
    NoQuotes,  // Leaves both double and single quotes unconverted.
};

enum SortOrder {
    AscendingOrder = Qt::AscendingOrder,
    DescendingOrder = Qt::DescendingOrder,
};

enum AppAttribute {
    ListenPort = 0,
    InternalEncoding,
    HttpOutputEncoding,
    Locale,
    MultiProcessingModule,
    UploadTemporaryDirectory,
    SqlDatabaseSettingsFiles,
    MongoDbSettingsFile,
    SqlQueriesStoredDirectory,
    DirectViewRenderMode,
    SystemLogFile,
    SqlQueryLogFile,
    ApplicationAbortOnFatal,
    LimitRequestBody,
    EnableCsrfProtectionModule,
    EnableHttpMethodOverride,
    SessionName,
    SessionStoreType,
    SessionAutoIdRegeneration,
    SessionCookiePath,
    SessionGcProbability,
    SessionGcMaxLifeTime,
    SessionSecret,
    SessionCsrfProtectionKey,
    MPMThreadMaxAppServers,
    MPMThreadMaxThreadsPerAppServer,
    MPMEpollMaxAppServers,
    SystemLogFilePath,
    SystemLogLayout,
    SystemLogDateTimeFormat,
    AccessLogFilePath,
    AccessLogLayout,
    AccessLogDateTimeFormat,
    ActionMailerDeliveryMethod,
    ActionMailerCharacterSet,
    ActionMailerDelayedDelivery,
    ActionMailerSmtpHostName,
    ActionMailerSmtpPort,
    ActionMailerSmtpAuthentication,
    ActionMailerSmtpUserName,
    ActionMailerSmtpPassword,
    ActionMailerSmtpEnablePopBeforeSmtp,
    ActionMailerSmtpPopServerHostName,
    ActionMailerSmtpPopServerPort,
    ActionMailerSmtpPopServerEnableApop,
    ActionMailerSendmailCommandLocation,
    //
    HttpKeepAliveTimeout,
    RedisSettingsFile,
    LDPreload,
    JavaScriptPath,
    ListenAddress,
    //
    SessionCookieMaxAge,
    SessionCookieDomain,
    //
    CacheSettingsFile,
    CacheBackend,
    CacheGcProbability,
    CacheEnableCompression,
    //
    SessionCookieSameSite,
    //
    EnableForwardedForHeader,
    TrustedProxyServers,
    ActionMailerSmtpRequireTLS,
};

// Reason codes why a web socket has been closed
enum CloseCode {
    NormalClosure = 1000,
    GoingAway = 1001,
    ProtocolError = 1002,
    UnsupportedData = 1003,
    Reserved = 1004,
    NoStatusRcvd = 1005,
    AbnormalClosure = 1006,
    InvalidFramePayloadData = 1007,
    PolicyViolation = 1008,
    MessageTooBig = 1009,
    MandatoryExtension = 1010,
    InternalError = 1011,
    ServiceRestart = 1012,
    TryAgainLater = 1013,
    TLSHandshake = 1015,
};

enum LogPriority {
    FatalLevel = 0,  //!< Severe error events that will presumably lead the app to abort.
    ErrorLevel,  //!< Error events that might still allow the app to continue running.
    WarnLevel,  //!< Potentially harmful situations.
    InfoLevel,  //!< Informational messages that highlight the progress of the app.
    DebugLevel,  //!< Informational events that are most useful to debug the app.
    TraceLevel,  //!< Finer-grained informational events than the DEBUG.
};

enum class KvsEngine {
    MongoDB = 0,
    Redis,
    CacheKvs,  // For internal use
    Num  // = 3
};


#if QT_VERSION >= 0x050e00  // 5.14.0
constexpr auto KeepEmptyParts = Qt::KeepEmptyParts;
constexpr auto SkipEmptyParts = Qt::SkipEmptyParts;
#else
constexpr auto KeepEmptyParts = QString::KeepEmptyParts;
constexpr auto SkipEmptyParts = QString::SkipEmptyParts;
#endif
}


/*!
  \namespace TSql
  \brief The TSql namespace contains miscellaneous identifiers used
  throughout the SQL library.
*/
namespace TSql {
enum ComparisonOperator {
    Invalid = 0,
    Equal,  // = val
    NotEqual,  // <> val
    LessThan,  // < val
    GreaterThan,  // > val
    LessEqual,  // <= val
    GreaterEqual,  // >= val
    IsNull,  // IS NULL
    IsEmpty,  // (column IS NULL OR column = '')
    IsNotNull,  // IS NOT NULL
    IsNotEmpty,  // column IS NOT NULL AND column <> ''
    Like,  // LIKE val
    NotLike,  // NOT LIKE val
    LikeEscape,  // LIKE val1 ESCAPE val2
    NotLikeEscape,  // NOT LIKE val1 ESCAPE val2
    ILike,  // ILIKE val
    NotILike,  // NOT ILIKE  val
    ILikeEscape,  // ILIKE val1 ESCAPE val2
    NotILikeEscape,  // NOT ILIKE val1 ESCAPE val2
    In,  // IN (val1, ...)
    NotIn,  // NOT IN (val1, ...)
    Between,  // BETWEEN val1 AND val2
    NotBetween,  // NOT BETWEEN val1 AND val2
    Any,  // ANY (val1, ...)
    All,  // ALL (val1, ...)
};

enum JoinMode {
    InnerJoin = 0,
    LeftJoin,
    RightJoin,
};
}

/*!
  \namespace TMongo
  \brief The TMongo namespace contains miscellaneous identifiers used
  throughout the MongoDB library.
*/
namespace TMongo {
enum ComparisonOperator {
    Invalid = 0,
    Equal = TSql::Equal,  // == val
    NotEqual = TSql::NotEqual,  // != val
    LessThan = TSql::LessThan,  // < val
    GreaterThan = TSql::GreaterThan,  // > val
    LessEqual = TSql::LessEqual,  // <= val
    GreaterEqual = TSql::GreaterEqual,  // >= val
    Exists = 100,  // $exists : true
    NotExists,  // $exists : false
    All,  // $all : [ val1, ... ]
    In,  // $in : [ val1, ... ]
    NotIn,  // $nin : [ val1, ... ]
    Mod,  // $mod : [ val1, val2 ]
    Size,  // $size : val
    Type,  // $type : val
};
}

