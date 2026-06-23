#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "rpc.h"

// Resolved reply, or a routed event/stream frame. The views in *msg (data,
// message, code) point into the runtime's internal buffer and are valid ONLY
// for the duration of the callback - copy out anything kept past it.
typedef void (*rpc_client_cb)(void *data, const rpc_message_t *msg);

typedef struct rpc_client_pending_s {
  uint64_t id;
  rpc_client_cb cb;
  void *data;
} rpc_client_pending_t;

typedef struct rpc_client_s {
  uint64_t next_id; // initialised to 1; next_id() returns then increments

  rpc_client_pending_t *pending; // growable, linear scan, swap-remove
  size_t pending_len;
  size_t pending_cap;

  uint8_t *buffer; // accumulation buffer for partial frames
  size_t buffer_len;
  size_t buffer_cap;

  rpc_client_cb on_message; // fallthrough: events, stream frames, untracked
  void *on_message_data;

  uint8_t reading; // reentrancy guard for rpc_client_read
} rpc_client_t;

// client error codes (< 0), distinct from rpc_error / rpc_partial
enum {
  rpc_client_err_alloc = -10,
  rpc_client_err_decode = -11,
  rpc_client_err_reentrant = -12,
};

int
rpc_client_init (rpc_client_t *client, rpc_client_cb on_message, void *data);

void
rpc_client_destroy (rpc_client_t *client);

uint64_t
rpc_client_next_id (rpc_client_t *client);

// Safe to call from within a callback. Returns 0 or rpc_client_err_alloc.
int
rpc_client_track (rpc_client_t *client, uint64_t id, rpc_client_cb cb, void *data);

// Remove a pending entry by id (idempotent). Safe from within a callback.
int
rpc_client_untrack (rpc_client_t *client, uint64_t id);

// Feed inbound bytes. Decodes complete frames and routes them. Returns 0, or
// rpc_client_err_* (< 0). MUST NOT be called from within a callback.
int
rpc_client_read (rpc_client_t *client, const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif // RPC_CLIENT_H
