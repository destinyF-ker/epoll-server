#include "binary_protocol.h"
#include <stdio.h>

/**
 * @brief 将消息打包到缓冲区中，包括消息头和可选的数据部分。
 * 
 * 此函数接受消息类型、可选的数据及其长度，以及一个缓冲区。它构造一个消息头，并将其与数据（如果存在）连接在一起，
 * 放入一个临时缓冲区中，然后将结果复制到目标缓冲区。通过 `dataLength` 参数更新打包后的消息的总大小（消息头 + 数据）。
 * 
 * @param type 消息的类型（`MessageType` 枚举或常量值），用于消息头中。
 * @param data 指向要包含在消息中的数据的指针。如果 `data` 为 NULL，则不会打包任何额外数据。
 * @param dataLength 指向 `uint16_t` 的指针，表示要打包的数据长度。输出时，该值会更新为打包后的消息总大小（消息头 + 数据）。
 * @param buffer 指向字符数组的指针，存储打包好的消息（包括消息头和数据）。该缓冲区必须足够大以容纳消息头和数据。
 * 
 * @note `MessageHeader` 结构体假设包含一个 `length` 字段（表示消息的总大小，包括消息头和数据）以及一个 `type` 字段（表示消息的类型）。
 * 
 * @note 消息头的大小由常量 `header_size` 定义，表示缓冲区中消息头占据的字节数。
 */
void pack_message(MessageType type, void *data, uint16_t *dataLength, char *buffer)
{
    // printf("pack_message: %d\n", type);
    MessageHeader header;
    header.length = header_size + *dataLength;
    header.type = type;

    char tmp[512];

    memmove(tmp, &header, header_size);
    if (data != NULL && *dataLength > 0)
    {
        memmove(tmp + header_size, data, *dataLength);
    }

    memmove(buffer, tmp, header_size + *dataLength);
    *dataLength = header_size + *dataLength;
}

/**
 * @brief 解包消息，将消息头从缓冲区中提取，并调整缓冲区以去除消息头。
 * 
 * 此函数从给定的缓冲区中解包消息头，并将其存储在提供的 `MessageHeader` 结构体中。
 * 同时，缓冲区中的消息头部分会被移除，剩余的内容向前移动。该函数还会检查缓冲区的长度是否足够存储消息头。
 * 
 * @param header 指向 `MessageHeader` 结构体的指针，用于存储从缓冲区解包出的消息头。
 * @param buffer 指向字符数组的指针，表示包含消息内容的缓冲区。消息头部分会被移除，剩余数据向前移动。
 * @param len 缓冲区的长度，以字节为单位。如果缓冲区长度小于消息头的大小，则解包失败。
 * 
 * @return 如果成功解包并移除消息头，返回 0。如果缓冲区长度不足以包含完整的消息头，则返回 -1。
 * 
 * @note `MessageHeader` 结构体假设其大小由 `sizeof(MessageHeader)` 表示，该函数依赖此结构体的大小来解包消息。
 */
int unpack_message(MessageHeader *header, char *buffer, int len)
{
    if (len < sizeof(MessageHeader))
    {
        return -1;
    }
    memmove(header, buffer, sizeof(MessageHeader));
    header = (MessageHeader *)header;

    // 去掉消息头
    memmove(buffer, buffer + sizeof(MessageHeader), len - sizeof(MessageHeader));

    return 0;
}