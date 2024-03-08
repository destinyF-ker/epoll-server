#include "binary_protocol.h"
#include <stdio.h>

void pack_message(MessageType type, void *data, uint16_t *dataLength, char *buffer)
{
    // printf("pack_message: %d\n", type);
    MessageHeader header;
    header.length = header_size + *dataLength;
    header.type = type;

    char tmp[512];

    memmove(tmp, &header, header_size);
    // for (int i = 0; i < header_size; i++)
    // {
    //     printf("%d ", tmp[i]);
    // }
    if (data != NULL && *dataLength > 0)
    {
        memmove(tmp + header_size, data, *dataLength);
    }

    memmove(buffer, tmp, header_size + *dataLength);
    *dataLength = header_size + *dataLength;
}

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