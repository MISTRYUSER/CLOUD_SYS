#include "thread_pool.h"

/**
 * @brief 将 TLV 数据打包为网络字节序格式
 * @param packet   [out] 输出缓冲区（需保证足够空间）
 * @param type     [in]  数据类型
 * @param len      [in]  数据长度
 * @param value    [in]  数据内容
 * @return 成功返回0，失败返回错误码
 */

/**
 * @brief 从网络字节序格式解析 TLV 数据
 * @param packet   [in]  输入数据缓冲区
 * @param type     [out] 解析出的数据类型
 * @param len      [out] 解析出的数据长度
 * @param value    [out] 数据缓冲区（需保证足够空间）
 * @return 成功返回0，失败返回错误码
 */
int tlv_unpack(const tlv_packet_t *tlv_packet, TLV_TYPE *type, uint16_t *len, char *value) {
    

    *type = tlv_packet->type;
    *len = tlv_packet->length;
    strncpy(value, tlv_packet->value, tlv_packet->length);
    value[tlv_packet->length] = '\0';
    return SUCCESS;
}
