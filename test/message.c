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

int
main(void) {
  test_request();
  test_response();
  test_stream_data();
  test_stream_error();
  return 0;
}
