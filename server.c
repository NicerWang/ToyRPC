#include "rpc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Adds 2 16-bit integers in little endian */
rpc_data *add2_int16(rpc_data*);

int main(int argc, char *argv[]) {
    rpc_server *state;
    state = rpc_server_init("::",3000);
    if (state == NULL) exit(EXIT_FAILURE);
    if (rpc_register(state, "add2_16", add2_int16) != 0) exit(EXIT_FAILURE);
    rpc_serve(state);

    return 0;
}

rpc_data *add2_int16(rpc_data *in) {
    if(in->data_len != 4) return NULL;
    /* Parse request */
    short number_1 = *(short *)(in->data);
    short number_2 = *((short*)(in->data) + 1);

    /* Perform calculation */
    printf("add2_int16: arguments %d and %d\n", number_1, number_2);
    int result = (int)number_1 + (int)number_2;

    rpc_data* out = malloc(sizeof(rpc_data));
    out->data_len = 4;
    out->data = malloc(4);
    memcpy(out->data, &result, 4);
    return out;
}
