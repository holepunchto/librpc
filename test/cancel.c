#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <utf.h>

#include "../include/rpc.h"

int
main () {
  int err;

  rpc_message_t message = {rpc_cancel, 1};

  message.message = utf8_string_view_init((utf8_t *) "Cancelled", 9);
  message.code = utf8_string_view_init((utf8_t *) "CANCELLED", 9);
  message.status = -1;

  compact_state_t state = {0, 0};

  err = rpc_preencode_message(&state, &message);
  assert(err == 0);

  state.buffer = malloc(state.end);

  err = rpc_encode_message(&state, &message);
  assert(err == 0);

  uint8_t expected[] = {
    0x17, 0x00, 0x00, 0x00, // Frame
    0x04, // Type
    0x01, // ID
    0x09, 'C', 'a', 'n', 'c', 'e', 'l', 'l', 'e', 'd',
    0x09, 'C', 'A', 'N', 'C', 'E', 'L', 'L', 'E', 'D',
    0x01, // Status
  };

  assert(state.end == sizeof(expected));
  assert(memcmp(state.buffer, expected, sizeof(expected)) == 0);

  state.start = 0;

  rpc_message_t decoded;
  err = rpc_decode_message(&state, &decoded);
  assert(err == 0);

  assert(decoded.type == rpc_cancel);
  assert(decoded.id == 1);
  assert(utf8_string_view_compare(decoded.message, message.message) == 0);
  assert(utf8_string_view_compare(decoded.code, message.code) == 0);
  assert(decoded.status == -1);
}
