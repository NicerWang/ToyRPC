#ifndef TOYRPC_PROTOCOL_H
#define TOYRPC_PROTOCOL_H

#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <stdio.h>


#define STATUS_SUCCESS (0)
#define STATUS_ERROR (-1)
#define RPC_FIND 1
#define RPC_EXIST 2
#define RPC_CALL 3

/*
 * 64bit
 * From host_byte_order to net_byte_order(big_endian)
 * Or reverse
 */
uint64_t htonll(uint64_t val)
{
    if (BYTE_ORDER == LITTLE_ENDIAN) return ((uint64_t)htonl((int)(val>>32&0xffffffff)) )|( ((uint64_t)htonl((int)(val&0xffffffff)))<<32 );
}

/*
 * Receive status byte from byte stream
 */
int get_status(int socket){
    int8_t statusCode;
    ssize_t count = read(socket, &statusCode, sizeof(statusCode));
    if(count != sizeof(statusCode)){
        fprintf(stderr,"protocol > get_status");
        return STATUS_ERROR;
    }
    return statusCode;
}

/*
 * Send status byte to byte stream
 */
int set_status(int socket, int8_t statusCode){
    ssize_t count = write(socket, &statusCode, sizeof(statusCode));
    if(count != sizeof(statusCode)){
        fprintf(stderr,"protocol > get_status");
        return -1;
    }
    return 0;
}

/*
 * Receive function ID from byte stream
 */
int get_call_id(int socket){
    int call_id;
    ssize_t count = read(socket, &call_id, sizeof(call_id));
    if(count != sizeof(call_id)){
        fprintf(stderr,"protocol > get_call_id");
        return -1;
    }
    return ntohl(call_id);
}

/*
 * Send function ID to byte stream
 */
int set_call_id(int socket, int call_id){
    call_id = htonl(call_id);
    ssize_t count = write(socket, &call_id, sizeof(call_id));
    if(count != sizeof(call_id)){
        fprintf(stderr,"protocol > set_call_id");
        return -1;
    }
    return 0;
}

/*
 * Send function name (query) to byte stream
 */
int set_query(int socket, char* query){
    unsigned int query_len = strlen(query) + 1;
    unsigned int query_len_big = htonl(query_len);
    ssize_t count = write(socket, &query_len_big, sizeof(query_len_big));
    if(count != sizeof(query_len_big)){
        fprintf(stderr,"protocol > set_query(query_len)");
        return -1;
    }
    count = write(socket, query, query_len);
    if(count != query_len){
        fprintf(stderr,"protocol > set_query(query)");
        return -1;
    }
    return 0;
}

/*
 * Receive function name (query) to byte stream
 */
char* get_query(int socket){
    unsigned int query_len;
    ssize_t count = read(socket, &query_len, sizeof(query_len));
    if(sizeof(query_len) != count){
        fprintf(stderr,"protocol > get_query(query_len)");
        return NULL;
    }
    query_len = ntohl(query_len);
    char* name = malloc(query_len);
    count = read(socket, name, query_len);
    if(query_len != count){
        fprintf(stderr,"protocol > get_query(query)");
        return NULL;
    }
    return name;
}

/*
 * Send payload to byte stream
 */
int set_payload(int socket, rpc_data* payload){
    unsigned int data_len = htonl(payload->data_len);
    ssize_t count = write(socket, &data_len, sizeof(data_len));
    if (count != sizeof(data_len)) {
        fprintf(stderr,"protocol > set_payload(data_len)");
        return -1;
    }
    count = write(socket, payload->data, payload->data_len);
    if (count != payload->data_len) {
        fprintf(stderr,"protocol > set_payload(data)");
        return -1;
    }
    return 0;
}

/*
 * Receive payload from byte stream
 */
int get_payload(int socket, rpc_data* payload){
    ssize_t count = read(socket, &(payload->data_len), sizeof(payload->data_len));
    if (count != sizeof(payload->data_len)) {
        fprintf(stderr,"protocol > get_payload(data_len)");
        return -1;
    }
    payload->data_len = ntohl(payload->data_len);
    payload->data = NULL;
    if(payload->data_len == 0) return 0;
    payload->data = malloc(payload->data_len);
    count = read(socket, payload->data, payload->data_len);
    if (count != payload->data_len) {
        fprintf(stderr,"protocol > get_payload(data)");
        return -1;
    }
    return 0;
}


#endif //TOYRPC_PROTOCOL_H