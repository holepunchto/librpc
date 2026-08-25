#include <assert.h>
#include <compact.h>
#include <stdlib.h>
#include <string.h>

#include "../include/rpc.h"

static void
test_request(void) {
  uint8_t payload[] = {1, 2, 3};

  rpc_message_t msg = {0};
  msg.type = rpc_request;
  msg.id = 7;
  msg.command = 42;
  msg.stream = 0;
  msg.data = payload;
  msg.len = sizeof(payload);

  compact_state_t enc = {0, 0, NULL};
  assert(rpc_preencode_message(&enc, &msg) == 0);
  enc.buffer = malloc(enc.end);
  assert(rpc_encode_message(&enc, &msg) == 0);

  compact_state_t dec = {0, enc.end, enc.buffer};
  rpc_message_t out = {0};
  assert(rpc_decode_message(&dec, &out) == 0);

  assert(out.type == rpc_request);
  assert(out.id == 7);
  assert(out.command == 42);
  assert(out.stream == 0);
  assert(out.len == sizeof(payload));
  assert(memcmp(out.data, payload, sizeof(payload)) == 0);

  free(enc.buffer);
}

static void
test_response(void) {
  uint8_t payload[] = {9, 8};

  rpc_message_t msg = {0};
  msg.type = rpc_response;
  msg.id = 7;
  msg.error = false;
  msg.stream = 0;
  msg.data = payload;
  msg.len = sizeof(payload);

  compact_state_t enc = {0, 0, NULL};
  assert(rpc_preencode_message(&enc, &msg) == 0);
  enc.buffer = malloc(enc.end);
  assert(rpc_encode_message(&enc, &msg) == 0);

  compact_state_t dec = {0, enc.end, enc.buffer};
  rpc_message_t out = {0};
  assert(rpc_decode_message(&dec, &out) == 0);

  assert(out.type == rpc_response);
  assert(out.id == 7);
  assert(out.error == false);
  assert(out.len == sizeof(payload));
  assert(memcmp(out.data, payload, sizeof(payload)) == 0);

  free(enc.buffer);
}

static void
test_stream_data(void) {
  uint8_t payload[] = {5, 6, 7, 8};

  rpc_message_t msg = {0};
  msg.type = rpc_stream;
  msg.id = 3;
  msg.stream = rpc_stream_data;
  msg.data = payload;
  msg.len = sizeof(payload);

  compact_state_t enc = {0, 0, NULL};
  assert(rpc_preencode_message(&enc, &msg) == 0);
  enc.buffer = malloc(enc.end);
  assert(rpc_encode_message(&enc, &msg) == 0);

  compact_state_t dec = {0, enc.end, enc.buffer};
  rpc_message_t out = {0};
  assert(rpc_decode_message(&dec, &out) == 0);

  assert(out.type == rpc_stream);
  assert(out.id == 3);
  assert(out.stream == rpc_stream_data);
  assert(out.len == sizeof(payload));
  assert(memcmp(out.data, payload, sizeof(payload)) == 0);

  free(enc.buffer);
}

static void
test_stream_error(void) {
  rpc_message_t msg = {0};
  msg.type = rpc_stream;
  msg.id = 4;
  msg.stream = rpc_stream_error;
  msg.message = (utf8_string_view_t){(const utf8_t *) "boom", 4};
  msg.code = (utf8_string_view_t){(const utf8_t *) "ERR", 3};
  msg.status = 500;

  compact_state_t enc = {0, 0, NULL};
  assert(rpc_preencode_message(&enc, &msg) == 0);
  enc.buffer = malloc(enc.end);
  assert(rpc_encode_message(&enc, &msg) == 0);

  compact_state_t dec = {0, enc.end, enc.buffer};
  rpc_message_t out = {0};
  assert(rpc_decode_message(&dec, &out) == 0);

  assert(out.type == rpc_stream);
  assert(out.id == 4);
  assert(out.stream == rpc_stream_error);
  assert(out.status == 500);
  assert(out.message.len == 4);
  assert(memcmp(out.message.data, "boom", 4) == 0);
  assert(out.code.len == 3);
  assert(memcmp(out.code.data, "ERR", 3) == 0);

  free(enc.buffer);
}

static void
test_malformed_body_is_signalled(void) {
  // A well formed frame header - length 2, and both promised bytes present -
  // wrapping a body that ends after `id`, so `command` cannot be decoded. The
  // frame is not partial, so the decoder has to signal rather than report a
  // message.
  uint8_t missing_command[] = {0x02, 0x00, 0x00, 0x00, rpc_request, 7};

  compact_state_t dec = {0, sizeof(missing_command), missing_command};
  rpc_message_t out = {0};
  assert(rpc_decode_message(&dec, &out) < 0);

  // Same shape one field further in: a payload length that overruns the body.
  uint8_t payload_overruns[] = {0x05, 0x00, 0x00, 0x00, rpc_request, 7, 42, 0, 9};

  compact_state_t dec2 = {0, sizeof(payload_overruns), payload_overruns};
  rpc_message_t out2 = {0};
  assert(rpc_decode_message(&dec2, &out2) < 0);
}

int
main(void) {
  test_request();
  test_response();
  test_stream_data();
  test_stream_error();
  test_malformed_body_is_signalled();
  return 0;
}
