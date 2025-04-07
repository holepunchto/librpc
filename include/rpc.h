#ifndef RPC_H
#define RPC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <compact.h>
#include <stdint.h>
#include <utf.h>

#include "utf/string.h"

typedef struct rps_s rpc_t;
typedef struct rpc_message_s rpc_message_t;

enum {
  rpc_error = -1,
  rpc_partial = -2,
};

enum {
  rpc_request = 1,
  rpc_response = 2,
  rpc_stream = 3,
};

enum {
  rpc_stream_open = 0x1,
  rpc_stream_close = 0x2,
  rpc_stream_pause = 0x4,
  rpc_stream_resume = 0x8,
  rpc_stream_data = 0x10,
  rpc_stream_end = 0x20,
  rpc_stream_destroy = 0x40,
  rpc_stream_error = 0x80,
  rpc_stream_request = 0x100,
  rpc_stream_response = 0x200,
};

struct rpc_message_s {
  uintmax_t type;
  uintmax_t id;

  union {
    // For `rpc_request`
    uintmax_t command;

    // For `rpc_response`
    bool error;
  };

  uintmax_t stream;

  union {
    // For:
    //   1. `rpc_request` if `stream == 0`
    //   2. `rpc_response` if `stream == 0` and `error == false`
    //   3. `rpc_stream` if `stream & rpc_stream_data`
    struct {
      uint8_t *data;
      size_t len;
    };

    // For:
    //   1. `rpc_response` if `stream == 0` and `error == true`
    //   2. `rpc_stream` if `stream & rpc_stream_error`
    struct {
      utf8_string_view_t message;
      utf8_string_view_t code;
      intmax_t status;
    };
  };
};

int
rpc_preencode_message (compact_state_t *state, const rpc_message_t *message);

int
rpc_encode_message (compact_state_t *state, const rpc_message_t *message);

int
rpc_decode_message (compact_state_t *state, rpc_message_t *result);

#ifdef __cplusplus
}
#endif

#endif // RPC_H
