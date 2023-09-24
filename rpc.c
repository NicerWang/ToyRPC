#include "rpc.h"
#include "protocol.h"

#define SERVICE_CAPACITY 200
#define LISTEN_CAPACITY 200

struct service {
    char *name;
    rpc_handler handler;
};

struct rpc_server {
    int socket;
    struct service services[SERVICE_CAPACITY];
    int size;
};

void _error_handler(int sockfd){
    set_status(sockfd, STATUS_ERROR);
    close(sockfd);
    exit(EXIT_FAILURE);
}

rpc_server *rpc_server_init(char *addr, int port) {
    if (0 >= port || 65536 <= port) return NULL;
    rpc_server *server = malloc(sizeof(rpc_server));

    // Initialize address info
    struct sockaddr_in6 address = {0};
    address.sin6_port = htons(port);
    address.sin6_family = AF_INET6;
    inet_pton(AF_INET6, addr, &(address.sin6_addr));

    server->socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (server->socket < 0) {
        perror("rpc_server_init > socket");
        exit(EXIT_FAILURE);
    }
    server->size = 0;

    int enable = 1;
    if (setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        perror("rpc_server_init > setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(server->socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("rpc_server_init > bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server->socket, LISTEN_CAPACITY) < 0) {
        perror("rpc_server_init > listen");
        exit(EXIT_FAILURE);
    }
    return server;
}

int rpc_register(rpc_server *server, char *name, rpc_handler handler) {
    if (server == NULL || server->size >= SERVICE_CAPACITY || name == NULL || handler == NULL || strlen(name) == 0) return -1;
    for (int i = 0; i < server->size; i++) {
        if (strcmp(server->services[i].name, name) == 0) {
            server->services[i].handler = handler;
            return 0;
        }
    }
    server->services[server->size].handler = handler;
    server->services[server->size].name = malloc(strlen(name) + 1);
    strcpy(server->services[server->size].name, name);
    server->size++;
    return 0;
}

void rpc_serve(rpc_server *server) {
    if (server == NULL) return;

    struct sockaddr_in6 remote_address;
    socklen_t remote_address_size = sizeof(remote_address);

    while (true) {
        int remote_socket;
        // Handle EINTR in slow system call
        while ((remote_socket = accept(server->socket, (struct sockaddr *) &remote_address, &remote_address_size)) < 0 && errno == EINTR) {}
        pid_t pid = fork();
        if (pid == 0) {
            // Ignore SIGPIPE causing exit
            signal(SIGPIPE, SIG_IGN);
            close(server->socket);
            int status = get_status(remote_socket);
            if (status == RPC_FIND) {
                char *query = get_query(remote_socket);
                if (query == NULL) _error_handler(remote_socket);
                int target = -1;
                for (int i = 0; i < server->size; i++) {
                    if (strcmp(server->services[i].name, query) == 0) {
                        target = i;
                        break;
                    }
                }
                if (target != -1) {
                    int ret = set_status(remote_socket, RPC_EXIST);
                    if (ret < 0) _error_handler(remote_socket);
                    ret = set_call_id(remote_socket, target);
                    if (ret < 0) _error_handler(remote_socket);
                } else _error_handler(remote_socket);
                free(query);
            } else if (status == RPC_CALL) {
                rpc_data *input = malloc(sizeof(rpc_data));
                int id = get_call_id(remote_socket);
                if (id < 0 || id >= server->size) _error_handler(remote_socket);
                get_payload(remote_socket, input);
                rpc_data *output = server->services[id].handler(input);
                if (output == NULL || (output->data_len == 0 && output->data != NULL) ||
                    (output->data_len != 0 && output->data == NULL))
                    _error_handler(remote_socket);
                int ret = set_status(remote_socket, STATUS_SUCCESS);
                if (ret < 0) _error_handler(remote_socket);
                ret = set_payload(remote_socket, output);
                if (ret < 0) _error_handler(remote_socket);
                rpc_data_free(output);
                rpc_data_free(input);
            }
            if (close(remote_socket) < 0) exit(EXIT_FAILURE);
        } else if (pid > 0) {
            close(remote_socket);
            continue;
        } else {
            exit(EXIT_FAILURE);
        }
    }
}

struct rpc_client {
    struct sockaddr_in6 address;
};

struct rpc_handle {
    int id; // Index in rpc_server:services
};

rpc_client *rpc_client_init(char *addr, int port) {
    if (addr == NULL || 0 >= port || 65536 <= port) return NULL;
    rpc_client *client = malloc(sizeof(rpc_client));
    memset(client, 0, sizeof(rpc_client));

    // ONLY Initialize address info
    client->address.sin6_family = AF_INET6;
    client->address.sin6_port = htons(port);
    inet_pton(AF_INET6, addr, &(client->address.sin6_addr));
    return client;
}

rpc_handle *rpc_find(rpc_client *client, char *name) {
    if (client == NULL || name == NULL) return NULL;

    int client_sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (client_sock < 0) {
        perror("rpc_find > socket");
        return NULL;
    }

    int ret = connect(client_sock, (struct sockaddr *) &(client->address), sizeof(struct sockaddr_in6));
    if (ret < 0) {
        perror("rpc_find > connect");
        return NULL;
    }

    // Send&Receive info
    ret = set_status(client_sock, RPC_FIND);
    if (ret < 0) {
        perror("rpc_find > set_status");
        return NULL;
    }
    ret = set_query(client_sock, name);
    if (ret < 0) {
        perror("rpc_find > set_query");
        return NULL;
    }
    int status = get_status(client_sock);

    if (status == RPC_EXIST) {
        int call_id = get_call_id(client_sock);
        struct rpc_handle *handle = malloc(sizeof(struct rpc_handle));
        handle->id = call_id;
        return handle;
    } else {
        return NULL;
    }
}

rpc_data *rpc_call(rpc_client *client, rpc_handle *handle, rpc_data *payload) {
    if (client == NULL || handle == NULL || payload == NULL) return NULL;

    // Data consistency check
    if (payload == NULL || (payload->data_len == 0 && payload->data != NULL) || (payload->data_len != 0 && payload->data == NULL)) {
        perror("rpc_call > data inconsistent");
        return NULL;
    }

    int client_sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (client_sock < 0) {
        perror("rpc_call > socket");
        return NULL;
    }
    int ret = connect(client_sock, (struct sockaddr *) &(client->address), sizeof(struct sockaddr_in6));
    if (ret < 0) {
        perror("rpc_call > connect");
        return NULL;
    }

    // Send&Receive info
    ret = set_status(client_sock, RPC_CALL);
    if (ret < 0) {
        perror("rpc_call > set_status");
        return NULL;
    }
    ret = set_call_id(client_sock, handle->id);
    if (ret < 0) {
        perror("rpc_call > set_call_id");
        return NULL;
    }
    ret = set_payload(client_sock, payload);
    if (ret < 0) {
        perror("rpc_call > set_payload");
        return NULL;
    }

    int status = get_status(client_sock);
    if (status == STATUS_SUCCESS) {
        rpc_data *result = malloc(sizeof(rpc_data));
        ret = get_payload(client_sock, result);
        if (ret < 0) {
            perror("rpc_call > get_payload");
            return NULL;
        }
        return result;
    } else {
        return NULL;
    }
}

void rpc_client_close(rpc_client *client) {
    free(client);
}

void rpc_data_free(rpc_data *data) {
    if (data == NULL) {
        return;
    }
    if (data->data != NULL) {
        free(data->data);
    }
    free(data);
}
