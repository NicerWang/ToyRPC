#ifndef TOYRPC_RPC_H
#define TOYRPC_RPC_H
/* Header for ToyRPC */
#include <stddef.h>

/* Server */
typedef struct rpc_server rpc_server;
/* Client */
typedef struct rpc_client rpc_client;

/* The payload for request&response */
typedef struct {
    size_t data_len;
    void *data;
} rpc_data;

/* Handler for remote function */
typedef struct rpc_handle rpc_handle;

/*
 * Remote function format (pointer)
 * takes rpc_data* as input and produces rpc_data* as output
 */
typedef rpc_data *(*rpc_handler)(rpc_data *);

/* ################ */
/* Server functions */
/* ################ */


/* Initialises server
 * RETURNS: rpc_server* on success, NULL on error
 */
rpc_server *rpc_server_init(char* addr, int port);

/*
 * Register a function to server
 * RETURNS: -1 on failure
 */
int rpc_register(rpc_server *srv, char *name, rpc_handler handler);

/* Start serving requests */
void rpc_serve(rpc_server *server);

/* ################ */
/* Client Functions */
/* ################ */

/*
 * Initialise client
 * RETURNS: rpc_client* on success, NULL on error
 */
rpc_client *rpc_client_init(char *addr, int port);

/*
 * Find a remote function by name
 * RETURNS: rpc_handler* on success, NULL on error
 * FREE: rpc_handler* NEED to be freed with a single call to free
 */
rpc_handler *rpc_find(rpc_client *client, char *name);

/*
 * Call remote function by handle
 * RETURNS: rpc_data* on success, NULL on error
 * FREE: rpc_data NEED to be freed with a single call to rpc_data_free
 */
rpc_data *rpc_call(rpc_client *client, rpc_handler *handler, rpc_data *payload);

/*
 * Close client
 */
void rpc_client_close(rpc_client *client);

/* ################ */
/* Shared Functions */
/* ################ */

/* Frees rpc_data */
void rpc_data_free(rpc_data *data);

#endif //TOYRPC_RPC_H
