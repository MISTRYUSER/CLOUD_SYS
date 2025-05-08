#include "thread_pool.h"

/**
 * @brief 将 TLV 数据打包为网络字节序格式
 * @param packet   [out] 输出缓冲区（需保证足够空间）
 * @param type     [in]  数据类型
 * @param len      [in]  数据长度
 * @param value    [in]  数据内容
 * @return 成功返回0，失败返回错误码
 */
int tlv_pack(tlv_packet_t *packet, TLV_TYPE type, uint16_t len, const char *value) {
    // 参数校验
    if (!packet || !value || len > CHAR_SIZE) {
        return ERR_PARAM;
    }

    // 转换网络字节序
    uint16_t net_type = htons((uint16_t)type);
    uint16_t net_len = htons(len);

    // 结构体打包（使用memcpy避免对齐问题）
    char *ptr = (char*)packet;
    memcpy(ptr, &net_type, sizeof(net_type));
    ptr += sizeof(net_type);
    memcpy(ptr, &net_len, sizeof(net_len));
    ptr += sizeof(net_len);
    memcpy(ptr, value, len);

    // 填充剩余空间为0（可选）
    if (len < CHAR_SIZE) {
        memset(ptr + len, 0, CHAR_SIZE - len);
    }

    return SUCCESS;
}

/**
 * @brief 从网络字节序格式解析 TLV 数据
 * @param packet   [in]  输入数据缓冲区
 * @param type     [out] 解析出的数据类型
 * @param len      [out] 解析出的数据长度
 * @param value    [out] 数据缓冲区（需保证足够空间）
 * @return 成功返回0，失败返回错误码
 */
int tlv_unpack(const tlv_packet_t *packet, TLV_TYPE *type, uint16_t *len, char *value) {
    // 参数校验
    if (!packet || !type || !len || !value) {
        return ERR_PARAM;
    }

    // 解析网络字节序
    uint16_t net_type, net_len;
    const char *ptr = (const char*)packet;
    
    memcpy(&net_type, ptr, sizeof(net_type));
    ptr += sizeof(net_type);
    memcpy(&net_len, ptr, sizeof(net_len));
    ptr += sizeof(net_len);

    // 转换主机字节序
    *type = (TLV_TYPE)ntohs(net_type);
    *len = ntohs(net_len);

    // 长度校验
    if (*len > CHAR_SIZE) {
        return ERR_PROTOCOL;
    }

    // 复制数据
    memcpy(value, ptr, *len);
    value[*len] = '\0'; // 添加字符串终止符

    return SUCCESS;
}
