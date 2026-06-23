#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../include/client.h"

static void
noop (void *data, const rpc_message_t *msg) {
  (void) data;
  (void) msg;
}

static void
test_next_id (void) {
  rpc_client_t client;
  assert(rpc_client_init(&client, noop, NULL) == 0);

  assert(rpc_client_next_id(&client) == 1);
  assert(rpc_client_next_id(&client) == 2);
  assert(rpc_client_next_id(&client) == 3);

  rpc_client_destroy(&client);
}

static void
test_double_destroy (void) {
  rpc_client_t client;
  rpc_client_init(&client, noop, NULL);
  rpc_client_destroy(&client);
  rpc_client_destroy(&client); // safe: pointers are NULL after the first destroy
}

int
main (void) {
  test_next_id();
  test_double_destroy();
  return 0;
}
