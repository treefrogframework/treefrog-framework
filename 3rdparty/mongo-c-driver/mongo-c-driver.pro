#
# MongoDB C driver project file
#

TEMPLATE = lib
TARGET = mongoc
CONFIG += static console
DEFINES += MONGOC_COMPILATION BSON_COMPILATION
*-g++:DEFINES += _GNU_SOURCE
DEPENDPATH += src
INCLUDEPATH += src src/libbson/src/bson src/libbson/src
OBJECTS_DIR = .obj

# Input
HEADERS += \
           src/mongoc/mongoc-bulk-operation.h \
           src/mongoc/mongoc-client-pool.h \
           src/mongoc/mongoc-client.h \
           src/mongoc/mongoc-collection.h \
           src/mongoc/mongoc-config.h \
           src/mongoc/mongoc-cursor.h \
           src/mongoc/mongoc-database.h \
           src/mongoc/mongoc-error.h \
           src/mongoc/mongoc-find-and-modify.h \
           src/mongoc/mongoc-flags.h \
           src/mongoc/mongoc-gridfs-file-list.h \
           src/mongoc/mongoc-gridfs-file-page.h \
           src/mongoc/mongoc-gridfs-file.h \
           src/mongoc/mongoc-gridfs.h \
           src/mongoc/mongoc-host-list.h \
           src/mongoc/mongoc-index.h \
           src/mongoc/mongoc-init.h \
           src/mongoc/mongoc-iovec.h \
           src/mongoc/mongoc-log.h \
           src/mongoc/mongoc-matcher.h \
           src/mongoc/mongoc-opcode.h \
           src/mongoc/mongoc-rand.h \
           src/mongoc/mongoc-read-concern.h \
           src/mongoc/mongoc-read-prefs.h \
           src/mongoc/mongoc-server-description.h \
           src/mongoc/mongoc-socket.h \
#           src/mongoc/mongoc-ssl.h \
           src/mongoc/mongoc-stream-buffered.h \
           src/mongoc/mongoc-stream-file.h \
           src/mongoc/mongoc-stream-gridfs.h \
           src/mongoc/mongoc-stream-socket.h \
           src/mongoc/mongoc-stream-tls.h \
           src/mongoc/mongoc-stream.h \
           src/mongoc/mongoc-trace.h \
           src/mongoc/mongoc-uri.h \
           src/mongoc/mongoc-version-functions.h \
           src/mongoc/mongoc-version.h \
           src/mongoc/mongoc-write-concern.h \
           src/mongoc/mongoc.h \
           src/mongoc/utlist.h \
           src/libbson/src/bson/b64_ntop.h \
           src/libbson/src/bson/b64_pton.h \
           src/libbson/src/bson/bcon.h \
           src/libbson/src/bson/bson-atomic.h \
           src/libbson/src/bson/bson-clock.h \
           src/libbson/src/bson/bson-compat.h \
           src/libbson/src/bson/bson-config.h \
           src/libbson/src/bson/bson-context.h \
           src/libbson/src/bson/bson-endian.h \
           src/libbson/src/bson/bson-error.h \
           src/libbson/src/bson/bson-iter.h \
           src/libbson/src/bson/bson-json.h \
           src/libbson/src/bson/bson-keys.h \
           src/libbson/src/bson/bson-macros.h \
           src/libbson/src/bson/bson-md5.h \
           src/libbson/src/bson/bson-memory.h \
           src/libbson/src/bson/bson-oid.h \
           src/libbson/src/bson/bson-reader.h \
           src/libbson/src/bson/bson-stdint-win32.h \
           src/libbson/src/bson/bson-stdint.h \
           src/libbson/src/bson/bson-string.h \
           src/libbson/src/bson/bson-types.h \
           src/libbson/src/bson/bson-utf8.h \
           src/libbson/src/bson/bson-value.h \
           src/libbson/src/bson/bson-version-functions.h \
           src/libbson/src/bson/bson-version.h \
           src/libbson/src/bson/bson-writer.h \
           src/libbson/src/bson/bson.h \
           src/libbson/src/yajl/yajl_alloc.h \
           src/libbson/src/yajl/yajl_buf.h \
           src/libbson/src/yajl/yajl_bytestack.h \
           src/libbson/src/yajl/yajl_common.h \
           src/libbson/src/yajl/yajl_encode.h \
           src/libbson/src/yajl/yajl_gen.h \
           src/libbson/src/yajl/yajl_lex.h \
           src/libbson/src/yajl/yajl_parse.h \
           src/libbson/src/yajl/yajl_parser.h \
           src/libbson/src/yajl/yajl_tree.h \
           src/libbson/src/yajl/yajl_version.h \
           src/libbson/build/cmake/bson/bson-stdint.h
SOURCES += \
           src/mongoc/mongoc-array.c \
           src/mongoc/mongoc-async-cmd.c \
           src/mongoc/mongoc-async.c \
           src/mongoc/mongoc-b64.c \
           src/mongoc/mongoc-buffer.c \
           src/mongoc/mongoc-bulk-operation.c \
           src/mongoc/mongoc-client-pool.c \
           src/mongoc/mongoc-client.c \
           src/mongoc/mongoc-cluster.c \
           src/mongoc/mongoc-collection.c \
           src/mongoc/mongoc-counters.c \
           src/mongoc/mongoc-cursor-array.c \
           src/mongoc/mongoc-cursor-cursorid.c \
           src/mongoc/mongoc-cursor-transform.c \
           src/mongoc/mongoc-cursor.c \
           src/mongoc/mongoc-database.c \
           src/mongoc/mongoc-find-and-modify.c \
           src/mongoc/mongoc-gridfs-file-list.c \
           src/mongoc/mongoc-gridfs-file-page.c \
           src/mongoc/mongoc-gridfs-file.c \
           src/mongoc/mongoc-gridfs.c \
           src/mongoc/mongoc-host-list.c \
           src/mongoc/mongoc-index.c \
           src/mongoc/mongoc-init.c \
           src/mongoc/mongoc-list.c \
           src/mongoc/mongoc-log.c \
           src/mongoc/mongoc-matcher-op.c \
           src/mongoc/mongoc-matcher.c \
           src/mongoc/mongoc-memcmp.c \
           src/mongoc/mongoc-opcode.c \
           src/mongoc/mongoc-queue.c \
           src/mongoc/mongoc-rand.c \
           src/mongoc/mongoc-read-concern.c \
           src/mongoc/mongoc-read-prefs.c \
           src/mongoc/mongoc-rpc.c \
           src/mongoc/mongoc-sasl.c \
           src/mongoc/mongoc-scram.c \
           src/mongoc/mongoc-server-description.c \
           src/mongoc/mongoc-server-stream.c \
           src/mongoc/mongoc-set.c \
           src/mongoc/mongoc-socket.c \
#           src/mongoc/mongoc-ssl.c \
           src/mongoc/mongoc-stream-buffered.c \
           src/mongoc/mongoc-stream-file.c \
           src/mongoc/mongoc-stream-gridfs.c \
           src/mongoc/mongoc-stream-socket.c \
           src/mongoc/mongoc-stream-tls.c \
           src/mongoc/mongoc-stream.c \
           src/mongoc/mongoc-topology-description.c \
           src/mongoc/mongoc-topology-scanner.c \
           src/mongoc/mongoc-topology.c \
           src/mongoc/mongoc-uri.c \
           src/mongoc/mongoc-util.c \
           src/mongoc/mongoc-version-functions.c \
           src/mongoc/mongoc-write-command.c \
           src/mongoc/mongoc-write-concern.c \
           src/tools/mongoc-stat.c \
           src/libbson/src/bson/bcon.c \
           src/libbson/src/bson/bson-atomic.c \
           src/libbson/src/bson/bson-clock.c \
           src/libbson/src/bson/bson-context.c \
           src/libbson/src/bson/bson-error.c \
           src/libbson/src/bson/bson-iso8601.c \
           src/libbson/src/bson/bson-iter.c \
           src/libbson/src/bson/bson-json.c \
           src/libbson/src/bson/bson-keys.c \
           src/libbson/src/bson/bson-md5.c \
           src/libbson/src/bson/bson-memory.c \
           src/libbson/src/bson/bson-oid.c \
           src/libbson/src/bson/bson-reader.c \
           src/libbson/src/bson/bson-string.c \
           src/libbson/src/bson/bson-timegm.c \
           src/libbson/src/bson/bson-utf8.c \
           src/libbson/src/bson/bson-value.c \
           src/libbson/src/bson/bson-version-functions.c \
           src/libbson/src/bson/bson-writer.c \
           src/libbson/src/bson/bson.c \
           src/libbson/src/yajl/yajl.c \
           src/libbson/src/yajl/yajl_alloc.c \
           src/libbson/src/yajl/yajl_buf.c \
           src/libbson/src/yajl/yajl_encode.c \
           src/libbson/src/yajl/yajl_gen.c \
           src/libbson/src/yajl/yajl_lex.c \
           src/libbson/src/yajl/yajl_parser.c \
           src/libbson/src/yajl/yajl_tree.c \
           src/libbson/src/yajl/yajl_version.c
