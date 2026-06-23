#include <stdlib.h>
#include <string.h>

#include "../include/client.h"

int
rpc_client_init (rpc_client_t *client, rpc_client_cb on_message, void *data) {
  memset(client, 0, sizeof(*client));
  client->next_id = 1;
  client->on_message = on_message;
  client->on_message_data = data;
  return 0;
}

void
rpc_client_destroy (rpc_client_t *client) {
  free(client->pending);
  free(client->buffer);
  memset(client, 0, sizeof(*client));
}

uint64_t
rpc_client_next_id (rpc_client_t *client) {
  return client->next_id++;
}

int
rpc_client_read (rpc_client_t *client, const uint8_t *buf, size_t len) {
  if (client->reading) return rpc_client_err_reentrant;
  client->reading = 1;

  if (len > 0) {
    size_t need = client->buffer_len + len;
    if (need > client->buffer_cap) {
      size_t cap = client->buffer_cap ? client->buffer_cap : 64;
      while (cap < need) cap *= 2;
      uint8_t *next = realloc(client->buffer, cap);
      if (next == NULL) {
        client->reading = 0;
        return rpc_client_err_alloc;
      }
      client->buffer = next;
      client->buffer_cap = cap;
    }
    memcpy(client->buffer + client->buffer_len, buf, len);
    client->buffer_len += len;
  }

  size_t start = 0;
  while (start < client->buffer_len) {
    compact_state_t state = {start, client->buffer_len, client->buffer};
    rpc_message_t msg = {0};
    int r = rpc_decode_message(&state, &msg);
    if (r == rpc_partial) break;
    if (r < 0) { // any other negative is a hard decode error (malformed frame)
      client->reading = 0;
      return rpc_client_err_decode;
    }
    if (client->on_message) client->on_message(client->on_message_data, &msg);
    start = state.start; // post-decode start is the consumed offset
  }

  if (start > 0) {
    memmove(client->buffer, client->buffer + start, client->buffer_len - start);
    client->buffer_len -= start;
  }

  client->reading = 0;
  return 0;
}
