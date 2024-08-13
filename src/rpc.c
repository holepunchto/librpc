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

    err = compact_preencode_int(state, response->status);
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
rpc_preencode__stream (compact_state_t *state, const rpc_message_t *stream) {
  int err;

  err = compact_preencode_uint(state, stream->state);
  assert(err == 0);

  if (stream->state & rpc_stream_error) {
    err = compact_preencode_utf8(state, stream->message);
    assert(err == 0);

    err = compact_preencode_utf8(state, stream->code);
    assert(err == 0);

    err = compact_preencode_int(state, stream->status);
    assert(err == 0);
  } else if (stream->state & rpc_stream_data) {
    err = compact_preencode_uint8array(state, stream->data, stream->len);
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

  err = compact_preencode_uint(state, message->id);
  assert(err == 0);

  switch (message->type) {
  case rpc_request:
    err = rpc_preencode__request(state, message);
    break;
  case rpc_response:
    err = rpc_preencode__response(state, message);
    break;
  case rpc_stream:
    err = rpc_preencode__stream(state, message);
    break;
  }

  return err;
}

int
rpc_encode__request (compact_state_t *state, const rpc_message_t *request) {
  int err;

  err = compact_encode_utf8(state, request->command);
  assert(err == 0);

  err = compact_encode_uint8array(state, request->data, request->len);
  assert(err == 0);

  err = compact_encode_uint(state, 0); // Reserved
  assert(err == 0);

  return err;
}

int
rpc_encode__response (compact_state_t *state, const rpc_message_t *response) {
  int err;

  err = compact_encode_bool(state, response->error);
  assert(err == 0);

  if (response->error) {
    err = compact_encode_utf8(state, response->message);
    assert(err == 0);

    err = compact_encode_utf8(state, response->code);
    assert(err == 0);

    err = compact_encode_int(state, response->status);
    assert(err == 0);
  } else {
    err = compact_encode_uint8array(state, response->data, response->len);
    assert(err == 0);
  }

  err = compact_encode_uint(state, 0); // Reserved
  assert(err == 0);

  return err;
}

int
rpc_encode__stream (compact_state_t *state, const rpc_message_t *stream) {
  int err;

  err = compact_encode_uint(state, stream->state);
  assert(err == 0);

  if (stream->state & rpc_stream_error) {
    err = compact_encode_utf8(state, stream->message);
    assert(err == 0);

    err = compact_encode_utf8(state, stream->code);
    assert(err == 0);

    err = compact_encode_int(state, stream->status);
    assert(err == 0);
  } else if (stream->state & rpc_stream_data) {
    err = compact_encode_uint8array(state, stream->data, stream->len);
    assert(err == 0);
  }

  err = compact_encode_uint(state, 0); // Reserved
  assert(err == 0);

  return err;
}

int
rpc_encode_message (compact_state_t *state, const rpc_message_t *message) {
  int err;

  size_t frame = state->start;

  err = compact_encode_uint32(state, 0); // Frame
  assert(err == 0);

  size_t start = state->start;

  err = compact_encode_uint(state, message->type);
  assert(err == 0);

  err = compact_encode_uint(state, message->id);
  assert(err == 0);

  switch (message->type) {
  case rpc_request:
    err = rpc_encode__request(state, message);
    break;
  case rpc_response:
    err = rpc_encode__response(state, message);
    break;
  case rpc_stream:
    err = rpc_encode__stream(state, message);
    break;
  }

  size_t end = state->start;

  state->start = frame;

  err = compact_encode_uint32(state, end - start); // Frame
  assert(err == 0);

  state->start = end;

  return err;
}

int
rpc_decode__request (compact_state_t *state, uintmax_t id, rpc_message_t *result) {
  int err;

  rpc_message_t request = {rpc_request, id};

  err = compact_decode_utf8(state, &request.command);
  if (err < 0) return rpc_error;

  err = compact_decode_uint8array(state, &request.data, &request.len);
  if (err < 0) return rpc_error;

  err = compact_decode_uint(state, NULL); // Reserved
  if (err < 0) return rpc_error;

  if (result) *result = request;

  return 0;
}

int
rpc_decode__response (compact_state_t *state, uintmax_t id, rpc_message_t *result) {
  int err;

  rpc_message_t response = {rpc_response, id};

  err = compact_decode_bool(state, &response.error);
  if (err < 0) return rpc_error;

  if (response.error) {
    err = compact_decode_utf8(state, &response.message);
    if (err < 0) return rpc_error;

    err = compact_decode_utf8(state, &response.code);
    if (err < 0) return rpc_error;

    err = compact_decode_int(state, &response.status);
    if (err < 0) return rpc_error;
  } else {
    err = compact_decode_uint8array(state, &response.data, &response.len);
    if (err < 0) return rpc_error;
  }

  err = compact_decode_uint(state, NULL); // Reserved
  if (err < 0) return rpc_error;

  if (result) *result = response;

  return 0;
}

int
rpc_decode__stream (compact_state_t *state, uintmax_t id, rpc_message_t *result) {
  int err;

  rpc_message_t response = {rpc_response, id};

  err = compact_decode_uint(state, &response.state);
  if (err < 0) return rpc_error;

  if (response.state & rpc_stream_error) {
    err = compact_decode_utf8(state, &response.message);
    if (err < 0) return rpc_error;

    err = compact_decode_utf8(state, &response.code);
    if (err < 0) return rpc_error;

    err = compact_decode_int(state, &response.status);
    if (err < 0) return rpc_error;
  } else if (response.state & rpc_stream_data) {
    err = compact_decode_uint8array(state, &response.data, &response.len);
    if (err < 0) return rpc_error;
  }

  err = compact_decode_uint(state, NULL); // Reserved
  if (err < 0) return rpc_error;

  if (result) *result = response;

  return 0;
}

int
rpc_decode_message (compact_state_t *state, rpc_message_t *result) {
  int err;

  uint32_t frame;
  err = compact_decode_uint32(state, &frame);
  if (err < 0) return rpc_partial;

  if (state->end - state->start < frame) return rpc_partial;

  uintmax_t type;
  err = compact_decode_uint(state, &type);
  if (err < 0) return rpc_error;

  uintmax_t id;
  err = compact_decode_uint(state, &id);
  if (err < 0) return rpc_error;

  switch (type) {
  case rpc_request:
    err = rpc_decode__request(state, id, result);
    break;
  case rpc_response:
    err = rpc_decode__response(state, id, result);
    break;
  case rpc_stream:
    err = rpc_decode__stream(state, id, result);
    break;
  default:
    return rpc_error;
  }

  return 0;
}
