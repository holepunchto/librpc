#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../include/client.h"

static void
noop (void *data, const rpc_message_t *msg) {
  (void) data;
  (void) msg;
}

static void
test_next_id (void) {
  rpc_client_t client;
  assert(rpc_client_init(&client, noop, NULL) == 0);

  assert(rpc_client_next_id(&client) == 1);
  assert(rpc_client_next_id(&client) == 2);
  assert(rpc_client_next_id(&client) == 3);

  rpc_client_destroy(&client);
}

static void
test_double_destroy (void) {
  rpc_client_t client;
  rpc_client_init(&client, noop, NULL);
  rpc_client_destroy(&client);
  rpc_client_destroy(&client); // safe: pointers are NULL after the first destroy
}

// Encode an rpc_message into a fresh malloc'd buffer (caller frees).
static uint8_t *
encode_message (const rpc_message_t *msg, size_t *len) {
  compact_state_t enc = {0, 0, NULL};
  assert(rpc_preencode_message(&enc, msg) == 0);
  enc.buffer = malloc(enc.end);
  assert(rpc_encode_message(&enc, msg) == 0);
  *len = enc.end;
  return enc.buffer;
}

typedef struct {
  int count;
  uint64_t last_id;
  uint64_t last_type;
} recorder_t;

static void
on_fallthrough (void *data, const rpc_message_t *msg) {
  recorder_t *r = data;
  r->count++;
  r->last_id = msg->id;
  r->last_type = msg->type;
}

// Build a unary response frame (type 2, stream 0) carrying `payload`.
static uint8_t *
response_frame (uint64_t id, uint8_t *payload, size_t payload_len, size_t *len) {
  rpc_message_t msg = {0};
  msg.type = rpc_response;
  msg.id = id;
  msg.error = false;
  msg.stream = 0;
  msg.data = payload;
  msg.len = payload_len;
  return encode_message(&msg, len);
}

static void
test_read_routes_to_fallthrough (void) {
  recorder_t rec = {0};
  rpc_client_t client;
  rpc_client_init(&client, on_fallthrough, &rec);

  uint8_t payload[] = {1, 2, 3};
  size_t len;
  uint8_t *frame = response_frame(7, payload, sizeof(payload), &len);

  assert(rpc_client_read(&client, frame, len) == 0);
  assert(rec.count == 1);
  assert(rec.last_id == 7);
  assert(rec.last_type == rpc_response);

  free(frame);
  rpc_client_destroy(&client);
}

static void
test_read_partial_frame (void) {
  recorder_t rec = {0};
  rpc_client_t client;
  rpc_client_init(&client, on_fallthrough, &rec);

  uint8_t payload[] = {9, 8, 7, 6};
  size_t len;
  uint8_t *frame = response_frame(3, payload, sizeof(payload), &len);

  assert(rpc_client_read(&client, frame, len - 2) == 0);
  assert(rec.count == 0);
  assert(rpc_client_read(&client, frame + len - 2, 2) == 0);
  assert(rec.count == 1);
  assert(rec.last_id == 3);

  free(frame);
  rpc_client_destroy(&client);
}

static void
test_read_multiple_frames (void) {
  recorder_t rec = {0};
  rpc_client_t client;
  rpc_client_init(&client, on_fallthrough, &rec);

  uint8_t p1[] = {1};
  uint8_t p2[] = {2};
  size_t l1, l2;
  uint8_t *f1 = response_frame(1, p1, sizeof(p1), &l1);
  uint8_t *f2 = response_frame(2, p2, sizeof(p2), &l2);

  uint8_t *both = malloc(l1 + l2);
  memcpy(both, f1, l1);
  memcpy(both + l1, f2, l2);

  assert(rpc_client_read(&client, both, l1 + l2) == 0);
  assert(rec.count == 2);
  assert(rec.last_id == 2);

  free(f1);
  free(f2);
  free(both);
  rpc_client_destroy(&client);
}

static void
on_reenter (void *data, const rpc_message_t *msg) {
  rpc_client_t *client = data;
  (void) msg;
  assert(rpc_client_read(client, (const uint8_t *) "", 0) == rpc_client_err_reentrant);
}

static void
test_read_reentrancy_guard (void) {
  rpc_client_t client;
  rpc_client_init(&client, on_reenter, &client);

  uint8_t payload[] = {1};
  size_t len;
  uint8_t *frame = response_frame(5, payload, sizeof(payload), &len);

  assert(rpc_client_read(&client, frame, len) == 0);

  free(frame);
  rpc_client_destroy(&client);
}

static void
test_read_malformed (void) {
  recorder_t rec = {0};
  rpc_client_t client;
  rpc_client_init(&client, on_fallthrough, &rec);

  uint8_t payload[] = {1};
  size_t len;
  uint8_t *frame = response_frame(1, payload, sizeof(payload), &len);
  // The frame length is a fixed-width 4-byte uint32, so byte 4 is the type
  // varint; 0x7f decodes to 127, an unknown type -> rpc_error -> err_decode.
  frame[4] = 0x7f;

  assert(rpc_client_read(&client, frame, len) == rpc_client_err_decode);

  free(frame);
  rpc_client_destroy(&client);
}

static void
test_read_empty (void) {
  recorder_t rec = {0};
  rpc_client_t client;
  rpc_client_init(&client, on_fallthrough, &rec);

  // a zero-length read is a no-op: returns 0, fires nothing, state unchanged
  assert(rpc_client_read(&client, NULL, 0) == 0);
  assert(rec.count == 0);
  assert(client.buffer_len == 0);

  rpc_client_destroy(&client);
}

int
main (void) {
  test_next_id();
  test_double_destroy();
  test_read_routes_to_fallthrough();
  test_read_partial_frame();
  test_read_multiple_frames();
  test_read_reentrancy_guard();
  test_read_malformed();
  test_read_empty();
  return 0;
}
