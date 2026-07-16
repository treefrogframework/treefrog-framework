/*
 * Copyright 2009-present MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <mongoc/mongoc-prelude.h>

#ifndef MONGOC_ASYNC_CMD_PRIVATE_H
#define MONGOC_ASYNC_CMD_PRIVATE_H

#include <mongoc/mongoc-array-private.h>
#include <mongoc/mongoc-async-private.h>
#include <mongoc/mongoc-buffer-private.h>
#include <mongoc/mongoc-cmd-private.h>

#include <mongoc/mcd-rpc.h>
#include <mongoc/mongoc-client.h>
#include <mongoc/mongoc-stream.h>

#include <bson/bson.h>
#include <bson/macros.h>

#include <mlib/duration.h>
#include <mlib/time_point.h>
#include <mlib/timer.h>

BSON_BEGIN_DECLS

typedef enum {
   // The command has no stream and needs to connect to a peer
   MONGOC_ASYNC_CMD_PENDING_CONNECT,
   // The command has connected and has a stream, but needs to run stream setup (e.g. TLS handshake)
   MONGOC_ASYNC_CMD_STREAM_SETUP,
   // The command has data to send to the peer
   MONGOC_ASYNC_CMD_SEND,
   // The command is ready to receive the response length header
   MONGOC_ASYNC_CMD_RECV_LEN,
   // The command is ready to receive the RPC message
   MONGOC_ASYNC_CMD_RECV_RPC,
   // The command is in an invalid error state
   MONGOC_ASYNC_CMD_ERROR_STATE,
   // The command has been cancelled.
   MONGOC_ASYNC_CMD_CANCELLED_STATE,
} mongoc_async_cmd_state_t;

/**
 * @brief Command callback/state result code
 */
typedef enum {
   MONGOC_ASYNC_CMD_CONNECTED,
   MONGOC_ASYNC_CMD_IN_PROGRESS,
   MONGOC_ASYNC_CMD_SUCCESS,
   MONGOC_ASYNC_CMD_ERROR,
   MONGOC_ASYNC_CMD_TIMEOUT,
} mongoc_async_cmd_result_t;

/**
 * @brief Callback type associated with an asynchronous command object.
 *
 * The callback will be invoked after a new connection is established, and
 * again when the command completes.
 *
 * @param acmd Pointer to the async command object that invoked the callback
 * @param result The result/state of the asynchronous command object
 * @param bson Result data associated with the command's state, if any.
 * @param duration The elapsed duration that the command object has been running.
 * This will be zero when the CONNECTED state is invoked.
 */
typedef void (*mongoc_async_cmd_event_cb)(struct _mongoc_async_cmd *acmd,
                                          mongoc_async_cmd_result_t result,
                                          const bson_t *bson,
                                          mlib_duration duration);

/**
 * @brief Callback that is used to open a new stream for a command object.
 *
 * If the function returns a null pointer, it is considered to have failed.
 */
typedef mongoc_stream_t *(*mongoc_async_cmd_connect_cb)(struct _mongoc_async_cmd *);

/**
 * @brief Stream setup callback for asynchronous commands
 *
 * This callback will be invoked by the async command runner after a stream has
 * been opened, allowing the command creator time to do setup on the stream
 * before the command tries to use it.
 *
 * @param stream Pointer to the valid stream object that was just created
 * @param events Pointer to a poll() events bitmask. The function can modify this
 * value to change what the stream will await on next
 * @param ctx Pointer to arbitrary user data for the setup function.
 * @param timeout A timer that gives a deadline for the setup operation
 * @return int The function should return -1 on failure, 1 if the stream
 * immediately has data to send, or 0 for generic success.
 */
typedef int (*mongoc_async_cmd_stream_setup_cb)(
   mongoc_stream_t *stream, int *events, void *ctx, mlib_timer timeout, bson_error_t *error);


typedef struct _mongoc_async_cmd {
   /**
    * @brief The stream that is associated with an in-progress command.
    *
    * This may start as a null pointer, but is updated when a connection is
    * established.
    */
   mongoc_stream_t *stream;

   // Non-owning pointer to the asynchrony engine associated with this command
   mongoc_async_t *async;
   /**
    * @brief The current state of the asynchronous command.
    *
    * Used to control the state machine that is executed with async_cmd_run()
    */
   mongoc_async_cmd_state_t state;
   // Bitmask of poll() events that this command is waiting to see
   int events;
   /**
    * @brief User-provided callback that will be used to lazily create the I/O stream
    * for the command.
    */
   mongoc_async_cmd_connect_cb _stream_connect;
   /**
    * @brief User-provided callback function to do setup on the command's stream
    * after the stream has been created automatically.
    */
   mongoc_async_cmd_stream_setup_cb _stream_setup;
   // Arbitrary userdata pointer passed to the stream setup function
   void *_stream_setup_userdata;
   /**
    * @brief User-provided command event callback. Invoked after a new
    * connection is established, and again when the command completes.
    */
   mongoc_async_cmd_event_cb _event_callback;
   // Arbitrary userdata passed when the object was created
   void *_userdata;
   /**
    * @brief Timer to when the command should attempt to lazily initiate a new
    * connection with the _stream_connect callback. This does not apply if the
    * command was given a stream upon construction.
    */
   mlib_timer _connect_delay_timer;
   /**
    * @brief The "start" reference point-in-time for the command object
    *
    * This is used to determine how long the command has been in progress,
    * including for when to consider the command to have timed-out.
    *
    * NOTE: This value can change! See: `_acmd_reset_elapsed`
    */
   mlib_time_point _start_time;
   /**
    * @brief The timeout duration allotted to the command object.
    *
    * We need to store it as a duration rather than a timer, because we need to
    * reset the timeout at certain points (see: `_acmd_reset_elapsed`)
    */
   mlib_duration _timeout;

   bson_error_t error;
   /**
    * @brief The BSON document of the command to be executed on the server.
    */
   bson_t _command;
   mongoc_buffer_t buffer;
   mongoc_iovec_t *iovec;
   size_t niovec;
   size_t bytes_written;
   size_t bytes_to_read;
   mcd_rpc_message *rpc;
   /**
    * @brief The response data from the peer.
    *
    * Initialized with BSON_INITIALIZER, so safe to pass/destroy upon construction.
    */
   bson_t _response_data;
   char *ns;
   /**
    * @brief The DNS address info that was associated with the command when
    * it was created. May be null if no DNS result was provided.
    */
   struct addrinfo *dns_result;

   struct _mongoc_async_cmd *next;
   struct _mongoc_async_cmd *prev;
} mongoc_async_cmd_t;

/**
 * @brief Create a new asynchronous command object associated with a collection
 * of async commands
 *
 * @param async The async engine that will own this new command
 * @param stream (Optional) a stream to be associated with the new command. If
 *    NULL, then a stream will be created lazily for the command object.
 * @param dns_result Pointer to a DNS result associated with the command
 * @param connect_callback Callback function that will be used to establish a
 *    new stream for the command object if `stream` is null.
 * @param connect_delay The amount of time that the command should wait before
 *    we try to connect the deferred stream for the command.
 * @param setup The stream setup callback for the command object.
 * @param setup_ctx Arbitrary data passed to the `setup` callback.
 * @param dbname The database name associated with the command. Required for OP_MSG
 * @param cmd The BSON data that will be sent in the command message
 * @param cmd_opcode The wire protocol opcode for the command
 * @param cb A callback that is invoked during events associated with the command.
 * @param userdata Arbitrary data pointer associated with the command object
 * @param timeout A timeout for the command. @see _acmd_reset_elapsed
 * @return mongoc_async_cmd_t* A newly created asynchronous command object
 */
mongoc_async_cmd_t *
mongoc_async_cmd_new(mongoc_async_t *async,
                     mongoc_stream_t *stream,
                     bool is_setup_done,
                     struct addrinfo *dns_result,
                     mongoc_async_cmd_connect_cb connect_callback,
                     mlib_duration connect_delay,
                     mongoc_async_cmd_stream_setup_cb setup,
                     void *setup_ctx,
                     const char *dbname,
                     const bson_t *cmd,
                     const int32_t cmd_opcode,
                     mongoc_async_cmd_event_cb cb,
                     void *userdata,
                     mlib_duration timeout);

/**
 * @brief Obtain a deadline timer that will expire when the given async command
 * will time out.
 *
 * Note that the initiation time of the command can be changed, which will also
 * adjust the point-in-time at which it expires.
 */
static inline mlib_timer
_acmd_deadline(const mongoc_async_cmd_t *self)
{
   BSON_ASSERT_PARAM(self);
   return mlib_expires_at(mlib_time_add(self->_start_time, self->_timeout));
}

/**
 * @brief Determine whether the given async command object has timed out
 */
static inline bool
_acmd_has_timed_out(const mongoc_async_cmd_t *self)
{
   return mlib_timer_is_expired(_acmd_deadline(self));
}

/**
 * @brief Cancel an in-progress command.
 *
 * This doesn't immediately destroy any resources or perform I/O, it just marks
 * the command to abort the next time it is polled.
 */
static inline void
_acmd_cancel(mongoc_async_cmd_t *self)
{
   BSON_ASSERT_PARAM(self);

   // Don't attempt to cancel a command in the error state, as it already has a
   // completion state waiting to be delivered.
   if (self->state != MONGOC_ASYNC_CMD_ERROR_STATE) {
      self->state = MONGOC_ASYNC_CMD_CANCELLED_STATE;
   }
}

/**
 * @brief Adjust the connect-delay timer for an async command by the given duration
 *
 * @param d A duration to be added/removed to the command's connect wait.
 *
 * This only effects commands that don't have an open stream and are pending a
 * connect. If this causes the connect-delay timer to expire, then the command
 * will attempt to connect the next time it is polled.
 */
static inline void
_acmd_adjust_connect_delay(mongoc_async_cmd_t *self, const mlib_duration d)
{
   BSON_ASSERT_PARAM(self);
   self->_connect_delay_timer.expires_at = mlib_time_add(self->_connect_delay_timer.expires_at, d);
}

/**
 * @brief Reset the elapsed time for the command, changing when it will timeout
 *
 * XXX: This is a HACK to fix CDRIVER-1571. The problem is that the provided deferred
 * connect (_stream_setup and/or _stream_connect) callbacks can perform blocking
 * I/O that delays everyone in the async pool, which can cause other commands
 * to exceed their timeout because one operation is blocking the entire pool.
 *
 * This function has the side effect that a command can exceed its allotted timeout
 * because this function is called multiple times, so only a single individual I/O
 * operation can actually timeout, rather than the entire composed operation.
 *
 * The proper fix is to force `_stream_setup` and `_stream_connect` to be
 * non-blocking, and the reference start time for the command can remain fixed.
 */
static inline void
_acmd_reset_elapsed(mongoc_async_cmd_t *self)
{
   self->_start_time = mlib_now();
}

/**
 * @brief Obtain the amount of time that the command has been running
 */
static inline mlib_duration
_acmd_elapsed(mongoc_async_cmd_t const *self)
{
   return mlib_elapsed_since(self->_start_time);
}

/**
 * @brief Obtain the userdata pointer associated with the given async command
 * object
 *
 * @param T The type to read from the pointer
 * @param Command Pointer to a command object
 */
#define _acmd_userdata(T, Command) ((T *)((Command)->_userdata))

void
mongoc_async_cmd_destroy(mongoc_async_cmd_t *acmd);

/**
 * @brief Pump the asynchronous command object state machine.
 *
 * If this function completes the command, it will destroy the async command
 * object and return `false`. Otherwise, it will return `true`.
 */
bool
mongoc_async_cmd_run(mongoc_async_cmd_t *acmd);

#ifdef MONGOC_ENABLE_SSL
/**
 * @brief Stream setup callback. Initializes TLS on the stream before the command runner tries to use it.
 *
 * @param ctx The userdata for the TLS setup is the hostname string for the peer.
 *
 * Refer to `mongoc_async_cmd_stream_setup_cb` for signature details
 */
int
mongoc_async_cmd_tls_setup(mongoc_stream_t *stream, int *events, void *ctx, mlib_timer deadline, bson_error_t *error);
#endif

BSON_END_DECLS


#endif /* MONGOC_ASYNC_CMD_PRIVATE_H */
