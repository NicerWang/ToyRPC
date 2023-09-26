#include "rpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {

    rpc_client *state = rpc_client_init("::1", 3000);
    rpc_handle *handle_add2_16 = rpc_find(state, "add2_16");
    rpc_data* request_data = malloc(sizeof(request_data));
    request_data->data_len = 4;
    short number_1 = 1;
    short number_2 = 5;
    request_data->data = malloc(request_data->data_len);
    memcpy(request_data->data, &number_1, 2);
    memcpy(request_data->data + 2, &number_2, 2);
    rpc_data *response_data = rpc_call(state, handle_add2_16, request_data);
    printf("add2_int16: result %d\n", *(int*)response_data->data);
    rpc_data_free(request_data);
    free(handle_add2_16);
    rpc_client_close(state);
    state = NULL;
    return 0;
}
