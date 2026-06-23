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
