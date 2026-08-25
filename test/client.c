#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../include/rpc/client.h"

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
test_read_malformed_body (void) {
  recorder_t rec = {0};
  rpc_client_t client;
  rpc_client_init(&client, on_fallthrough, &rec);

  // A well formed header - length 2, both promised bytes present - over a body
  // that ends after `id`, so `command` cannot be decoded. The read must fail,
  // and nothing may reach the fallthrough.
  uint8_t frame[] = {0x02, 0x00, 0x00, 0x00, rpc_request, 7};

  assert(rpc_client_read(&client, frame, sizeof(frame)) == rpc_client_err_decode);
  assert(rec.count == 0);

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

typedef struct {
  int count;
  uint64_t last_id;
  bool last_error;
  int64_t last_status;
} reply_t;

static void
on_reply (void *data, const rpc_message_t *msg) {
  reply_t *r = data;
  r->count++;
  r->last_id = msg->id;
  r->last_error = msg->error;
  if (msg->error) r->last_status = msg->status;
}

static uint8_t *
error_frame (uint64_t id, size_t *len) {
  rpc_message_t msg = {0};
  msg.type = rpc_response;
  msg.id = id;
  msg.error = true;
  msg.stream = 0;
  msg.message = (utf8_string_view_t){(const utf8_t *) "boom", 4};
  msg.code = (utf8_string_view_t){(const utf8_t *) "ERR", 3};
  msg.status = 500;
  return encode_message(&msg, len);
}

static void
test_track_resolves_once (void) {
  recorder_t fall = {0};
  reply_t reply = {0};
  rpc_client_t client;
  rpc_client_init(&client, on_fallthrough, &fall);

  assert(rpc_client_track(&client, 7, on_reply, &reply) == 0);

  uint8_t payload[] = {1, 2, 3};
  size_t len;
  uint8_t *frame = response_frame(7, payload, sizeof(payload), &len);

  // first response resolves the pending callback, not the fallthrough
  assert(rpc_client_read(&client, frame, len) == 0);
  assert(reply.count == 1);
  assert(reply.last_id == 7);
  assert(reply.last_error == false);
  assert(fall.count == 0);

  // a second response for the same id falls through (entry was removed)
  assert(rpc_client_read(&client, frame, len) == 0);
  assert(reply.count == 1);
  assert(fall.count == 1);

  free(frame);
  rpc_client_destroy(&client);
}

static void
test_error_reply_resolves (void) {
  reply_t reply = {0};
  rpc_client_t client;
  rpc_client_init(&client, noop, NULL);

  rpc_client_track(&client, 4, on_reply, &reply);

  size_t len;
  uint8_t *frame = error_frame(4, &len);

  assert(rpc_client_read(&client, frame, len) == 0);
  assert(reply.count == 1);
  assert(reply.last_id == 4);
  assert(reply.last_error == true);
  assert(reply.last_status == 500);

  free(frame);
  rpc_client_destroy(&client);
}

static void
test_untrack (void) {
  recorder_t fall = {0};
  reply_t reply = {0};
  rpc_client_t client;
  rpc_client_init(&client, on_fallthrough, &fall);

  rpc_client_track(&client, 9, on_reply, &reply);
  assert(rpc_client_untrack(&client, 9) == 0);
  assert(rpc_client_untrack(&client, 999) == 0); // idempotent: absent id

  uint8_t payload[] = {1};
  size_t len;
  uint8_t *frame = response_frame(9, payload, sizeof(payload), &len);

  // untracked: routes to fallthrough, pending cb never fires
  assert(rpc_client_read(&client, frame, len) == 0);
  assert(reply.count == 0);
  assert(fall.count == 1);

  free(frame);
  rpc_client_destroy(&client);
}

static void
test_event_falls_through_even_when_tracking (void) {
  recorder_t fall = {0};
  reply_t reply = {0};
  rpc_client_t client;
  rpc_client_init(&client, on_fallthrough, &fall);

  rpc_client_track(&client, 0, on_reply, &reply); // contrived: id 0

  // an event is a request frame (type 1) with id 0 - resolution is response-
  // only, so it must NOT resolve a pending entry; it goes to the fallthrough
  rpc_message_t ev = {0};
  ev.type = rpc_request;
  ev.id = 0;
  ev.command = 1;
  ev.stream = 0;
  uint8_t epayload[] = {42};
  ev.data = epayload;
  ev.len = sizeof(epayload);
  size_t len;
  uint8_t *frame = encode_message(&ev, &len);

  assert(rpc_client_read(&client, frame, len) == 0);
  assert(reply.count == 0);
  assert(fall.count == 1);
  assert(fall.last_type == rpc_request);

  free(frame);
  rpc_client_destroy(&client);
}

static void
test_track_grows_past_cap (void) {
  reply_t replies[8] = {0};
  rpc_client_t client;
  rpc_client_init(&client, noop, NULL);

  // more than the initial cap (4) to exercise the pending realloc growth path
  for (uint64_t i = 1; i <= 8; i++) {
    assert(rpc_client_track(&client, i, on_reply, &replies[i - 1]) == 0);
  }
  for (uint64_t i = 1; i <= 8; i++) {
    uint8_t payload[] = {1};
    size_t len;
    uint8_t *frame = response_frame(i, payload, sizeof(payload), &len);
    assert(rpc_client_read(&client, frame, len) == 0);
    free(frame);
  }
  for (int i = 0; i < 8; i++) assert(replies[i].count == 1);

  rpc_client_destroy(&client);
}

typedef struct {
  rpc_client_t *client;
  int count;
} tracker_ctx_t;

// a resolution callback that registers follow-up requests, forcing a pending
// realloc mid-callback - must not corrupt the in-flight resolution (the entry
// was copied to the stack and swap-removed before this ran)
static void
on_reply_then_track (void *data, const rpc_message_t *msg) {
  (void) msg;
  tracker_ctx_t *ctx = data;
  ctx->count++;
  for (uint64_t i = 100; i < 110; i++) {
    assert(rpc_client_track(ctx->client, i, noop, NULL) == 0);
  }
}

static void
test_callback_tracks_during_resolution (void) {
  rpc_client_t client;
  rpc_client_init(&client, noop, NULL);
  tracker_ctx_t ctx = {&client, 0};

  rpc_client_track(&client, 7, on_reply_then_track, &ctx);

  uint8_t payload[] = {1};
  size_t len;
  uint8_t *frame = response_frame(7, payload, sizeof(payload), &len);
  assert(rpc_client_read(&client, frame, len) == 0);
  assert(ctx.count == 1); // resolved exactly once despite the realloc

  free(frame);
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
  test_read_malformed_body();
  test_read_empty();
  test_track_resolves_once();
  test_error_reply_resolves();
  test_untrack();
  test_event_falls_through_even_when_tracking();
  test_track_grows_past_cap();
  test_callback_tracks_during_resolution();
  return 0;
}
