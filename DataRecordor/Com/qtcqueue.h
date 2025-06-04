#ifndef QTCQUEUE_H
#define QTCQUEUE_H
//  QTCQueue.h
//  环形队列头文件
//  包含使用QTCQueue类所需的定义
#include <QMutex>
#include <string.h>
//#pragma pack(1)
//常量
const unsigned int _QUEUE_SIZE = 10240;
const unsigned int _HIGH_WATER_LEVEL = _QUEUE_SIZE * 3/4;
const unsigned int _LOW_WATER_LEVEL  = _QUEUE_SIZE /4;
//队列类
class QTCQueue
{
public:
    QTCQueue(unsigned int iSize = _QUEUE_SIZE);
    ~QTCQueue();
    
private:
    //通用成员
    unsigned int 			m_iMaxSize;					//队列大小
    volatile unsigned int 	m_iRead;					//队列头指针
    volatile unsigned int 	m_iWrite;					//队列尾指针
    volatile unsigned char	*m_sBuffer;		            //队列缓冲区
    //qt中防止线程冲突
    QMutex                   m_Lock;

public:    //inline内联函数可以提高执行效率，但会消耗多余的内存，在调用点展开该函数，把定义放在声明中会避免引起不匹配的情况
    //	队列中数据的长度
    int InUseCount();
    //	队列中空闲的空间
    int FreeCount();
    //	在队列中插入数据
    int Add( char c );
    int	Add( void *buf,int iLen=1 );
    //  从队列中取数据
    int Get();
    int	Get( void *buf,int iLen = _QUEUE_SIZE );
    //  查看队列中的数据
    int Peek();
    int	Peek( void *buf,int iLen = _QUEUE_SIZE );
    //	移动指定队列的指针
    void MoveReadP(  unsigned int iSteps=1 );
    void MoveWriteP( unsigned int iSteps=1 );
    //	取得队列的指针
    unsigned int GetWriteP()	{ return m_iWrite; }
    unsigned int GetReadP()	{ return m_iRead;  }
    //清空队列
    void Empty();
    int IndexOf(unsigned char value);
};


///////////////////////////////////////////////////

#endif // QTCQUEUE_H
