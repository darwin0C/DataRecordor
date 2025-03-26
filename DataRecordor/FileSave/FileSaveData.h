#ifndef FILESAVEDATA_H
#define FILESAVEDATA_H
#include <QDateTime>
typedef unsigned char byte;

#pragma pack(push, 1) //按字节对齐begin

struct TDataBuffer
{
    byte *pBuffer;
    int nLen;
};
typedef struct
{
    unsigned short  year    ;//年
    unsigned char   month   ;//月
    unsigned char   day     ;//日
    unsigned char   hour    ;//时
    unsigned char   min     ;//分钟
    unsigned char   sec     ;//秒
    unsigned short  msecond ;//毫秒

}_TIME_Type;



struct CanDataBody
{
    QDateTime dateTime;
    unsigned int    dataid;
    unsigned char   data[8];    //8字节的数据
    unsigned char   len;        //
};

#pragma pack(pop) //按字节对齐end


#endif // FILESAVEDATA_H
