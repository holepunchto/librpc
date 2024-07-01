#include <assert.h>
#include <compact.h>

#include "../include/rpc.h"

int
rpc_preencode__request (compact_state_t *state, const rpc_message_t *request) {
  int err;

  err = compact_preencode_utf8(state, request->command);
  assert(err == 0);

  err = compact_preencode_uint8array(state, request->data, request->len);
  assert(err == 0);

  err = compact_preencode_uint(state, 0); // Reserved
  assert(err == 0);

  return err;
}

int
rpc_preencode__response (compact_state_t *state, const rpc_message_t *response) {
  int err;

  err = compact_preencode_bool(state, response->error);
  assert(err == 0);

  if (response->error) {
    err = compact_preencode_utf8(state, response->message);
    assert(err == 0);

    err = compact_preencode_utf8(state, response->code);
    assert(err == 0);

    err = compact_preencode_int(state, response->errno);
    assert(err == 0);
  } else {
    err = compact_preencode_uint8array(state, response->data, response->len);
    assert(err == 0);
  }

  err = compact_preencode_uint(state, 0); // Reserved
  assert(err == 0);

  return err;
}

int
rpc_preencode_message (compact_state_t *state, const rpc_message_t *message) {
  int err;

  err = compact_preencode_uint32(state, 0); // Frame
  assert(err == 0);

  err = compact_preencode_uint(state, message->type);
  assert(err == 0);

  switch (message->type) {
  case rpc_request:
    err = rpc_preencode__request(state, message);
    break;
  case rpc_response:
    err = rpc_preencode__response(state, message);
    break;
  }

  return err;
}

int
rpc_encode_message (compact_state_t *state, const rpc_message_t *message);

int
rpc_decode_message (compact_state_t *state, rpc_message_t *result);
