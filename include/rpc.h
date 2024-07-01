#ifndef RPC_H
#define RPC_H

#include "utf/string.h"
#ifdef __cplusplus
extern "C" {
#endif

#include <compact.h>
#include <stdint.h>
#include <utf.h>

typedef struct rps_s rpc_t;
typedef struct rpc_message_s rpc_message_t;

enum {
  rpc_error = -1,
  rpc_partial = -2,
};

struct rpc_message_s {
  enum {
    rpc_request = 1,
    rpc_response = 2,
  } type;

  uintmax_t id;

  union {
    // For `rpc_request`
    utf8_string_view_t command;

    // For `rpc_response`
    bool error;
  };

  union {
    // For `rpc_request` and `rpc_response` if `error == false`
    struct {
      uint8_t *data;
      size_t len;
    };

    // For `rpc_response` if `error == true`
    struct {
      utf8_string_view_t message;
      utf8_string_view_t code;
      intmax_t errno;
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
