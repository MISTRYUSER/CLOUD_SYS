#include "header.h"
int send_tlv(int sock, int type, const char* value) {
    tlv_packet_t tlv;
    tlv.type = type;
    strncpy(tlv.value, value, CHAR_SIZE - 1);
    tlv.value[CHAR_SIZE - 1] = '\0';
    tlv.length = strlen(tlv.value);

    ssize_t sent = send(sock, &tlv.type, sizeof(tlv.type), 0);
    ERROR_CHECK(sent, -1, "发送类型失败");
    sent = send(sock, &tlv.length, sizeof(tlv.length), 0);
    ERROR_CHECK(sent, -1, "发送长度失败");
    sent = send(sock, tlv.value, tlv.length, 0);
    ERROR_CHECK(sent, -1, "发送值失败");
    return 0;
}

int recv_tlv(int sock, tlv_packet_t *tlv) {
    ssize_t received = recv(sock, &tlv->type, sizeof(tlv->type), 0);
    if (received == 0) return 0;
    ERROR_CHECK(received, -1, "接收类型失败");

    received = recv(sock, &tlv->length, sizeof(tlv->length), 0);
    if (received == 0) return 0;
    ERROR_CHECK(received, -1, "接收长度失败");

    received = recv(sock, tlv->value, tlv->length, 0);
    if (received == 0) return 0;
    ERROR_CHECK(received, -1, "接收值失败");

    tlv->value[tlv->length] = '\0';
    return tlv->length;
}
