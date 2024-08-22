#include "qtcqueue.h"


//构造函数
QTCQueue::QTCQueue(unsigned int iSize )
{
    //根据给定大小分配队列内存
    m_iMaxSize = (iSize>_QUEUE_SIZE) ? _QUEUE_SIZE : iSize;			//队列的最大值为QUEUESIZE 字节

    //创建队列缓冲区
    this->m_sBuffer = new unsigned char[this->m_iMaxSize];
    Q_ASSERT( this->m_sBuffer != NULL );
    //队列指针初始化
    m_iRead=0;
    m_iWrite=0;
//	::InitializeCriticalSection( &this->m_Lock );
}
//析构函数
QTCQueue::~QTCQueue()
{
  delete  this->m_sBuffer;
}
//清空队列
void QTCQueue::Empty()
{
    //::EnterCriticalSection( &this->m_Lock );
    m_Lock.lock();
    m_iRead=m_iWrite=0;
    //::LeaveCriticalSection( &this->m_Lock );
    m_Lock.unlock();
}
