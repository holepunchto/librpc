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

int
main(void) {
  test_request();
  test_response();
  return 0;
}
