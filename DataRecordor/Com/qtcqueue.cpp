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

//----------------------------------------
//	移动读指针n步
void QTCQueue::MoveReadP(unsigned int iSteps)
{
    //::EnterCriticalSection( &this->m_Lock );
        m_Lock.lock();
        this->m_iRead += iSteps;
        this->m_iRead %= m_iMaxSize;
    //::LeaveCriticalSection( &this->m_Lock );
        m_Lock.unlock();
}

//----------------------------------------
//	移动写指针n步
void QTCQueue::MoveWriteP(unsigned int iSteps)
{
    //::EnterCriticalSection( &this->m_Lock );
    m_Lock.lock();
        this->m_iWrite += iSteps;
        this->m_iWrite %= m_iMaxSize;
    //::LeaveCriticalSection( &this->m_Lock );
    m_Lock.unlock();
}

//----------------------------------------
//	队列中数据的长度（字符数量）

int QTCQueue::InUseCount()
{
    m_Lock.lock();
        int iSize = (m_iWrite - m_iRead + m_iMaxSize) % m_iMaxSize;
    m_Lock.unlock();
    return iSize;
}

//----------------------------------------
//	队列中数据的长度
int QTCQueue::FreeCount()
{
    //::EnterCriticalSection( &this->m_Lock );
    int a=InUseCount();
    m_Lock.lock();
        int iSize = m_iMaxSize-a-1;
    //::LeaveCriticalSection( &this->m_Lock );
    m_Lock.unlock();
    return ( (iSize>0) ? iSize : 0 );
}

//--------------------------------------
//	插入一个字符
//	Para:
//		c:	inserted char

int QTCQueue::Add(char c)
{
    //没有空间返回
    if( FreeCount()<=0 )	return -1;
    //::EnterCriticalSection( &this->m_Lock );
    m_Lock.lock();
        //插入字符
        m_sBuffer[m_iWrite] = c;
        //移动写指针
        this->m_iWrite += 1;
        this->m_iWrite %= m_iMaxSize;
    //::LeaveCriticalSection( &this->m_Lock );
    m_Lock.unlock();
    return 1;				//不检测是否满(因为空/满情况下，都是iWrite==iRead)
}

//--------------------------------------
//	插入指定个数的字符（多态-插入多个字符）
//	Para:
//		buf:	inserted chars
//		iLen:	inserted char count
int QTCQueue::Add(void *buf,int iLen)
{
    //要插入数目=0，返回
    if( iLen<=0 )			return -1;
    //插入数据
    if( FreeCount()< iLen )
  return -1;

    //::EnterCriticalSection( &this->m_Lock );
    m_Lock.lock();
        //求分割点
        int iDiv = m_iMaxSize - m_iWrite;
        //插入数据
        if( m_iWrite+iLen <= (m_iMaxSize-1) )
            memcpy( (char *)m_sBuffer + m_iWrite, (char *)buf, iLen );
        else{
            memcpy( (char *)m_sBuffer + m_iWrite, (char *)buf, iDiv );
            memcpy( (char *)m_sBuffer, (char *)buf + iDiv, iLen - iDiv );
        }
        //移动指针
        this->m_iWrite += iLen;
        this->m_iWrite %= m_iMaxSize;
    //::LeaveCriticalSection( &this->m_Lock );
    m_Lock.unlock();
    return iLen;
}

//----------------------------------------
//	取出一个字符
int QTCQueue::Get()
{
    unsigned char c;

    //无数据返回
    if( this->InUseCount() <=0 )	return -1;

    //::EnterCriticalSection( &this->m_Lock );
    m_Lock.lock();
        //取字符
        c = m_sBuffer[m_iRead];
        //移动读指针
        this->m_iRead += 1;
        this->m_iRead %= m_iMaxSize;
    //::LeaveCriticalSection( &this->m_Lock );
    m_Lock.unlock();

    return c;
}

//--------------------------------------
//	取出指定个数的字符（多态-取出多个字符）
//	Para:
//		buf:	inserted chars
//		iLen:	inserted char count
int QTCQueue::Get(void *buf,int iLen)
{
    //要取出数目=0，返回
    if( iLen==0 )	return -1;
    //无数据返回
    if( this->InUseCount() <=0 )	return -1;
    //求读取长度
    int iReadLen = (iLen<=InUseCount()) ? iLen : InUseCount();

    //::EnterCriticalSection( &this->m_Lock );
    m_Lock.lock();
    //求分割点
    int iDiv = m_iMaxSize - m_iRead;
    //插入数据
    if( m_iRead+iReadLen<=m_iMaxSize )
        memcpy((char *)buf,(char *)m_sBuffer + m_iRead, iReadLen );
    else{
        memcpy((char *)buf,(char *)m_sBuffer + m_iRead, iDiv );
        memcpy((char *)buf + iDiv,(char *)m_sBuffer, iReadLen - iDiv );
    }
    //移动读指针
    this->m_iRead += iReadLen;
    this->m_iRead %= m_iMaxSize;
    //::LeaveCriticalSection( &this->m_Lock );
    m_Lock.unlock();
    return iReadLen;
}

//----------------------------------------
//	查看首字符
int QTCQueue::Peek()
{
    unsigned char c;

    //无数据返回
    if( this->InUseCount() <=0 )	return -1;

    //::EnterCriticalSection( &this->m_Lock );
    m_Lock.lock();
        //取字符
        c = m_sBuffer[m_iRead];
    //::LeaveCriticalSection( &this->m_Lock );
    m_Lock.unlock();

    return c;
}

//--------------------------------------
//	查看指定个数的字符
//	Para:
//		buf:	inserted chars
//		iLen:	inserted char count
int	QTCQueue::Peek(void *buf,int iLen)
{
    //要取出的数目=0，返回
    if( iLen == 0 )					return -1;
    //无数据返回
    if( this->InUseCount() <= 0 )	return -1;
    //求读取长度
    int iReadLen = ( iLen <= InUseCount() ) ? iLen : InUseCount();

    //::EnterCriticalSection( &this->m_Lock );
    m_Lock.lock();
        //求分割点
        int iDiv = m_iMaxSize - m_iRead;
        //取数据
        if( m_iRead+iReadLen<=m_iMaxSize )
            memcpy( (char *)buf, (char *)(m_sBuffer+m_iRead), iReadLen );
        else{
            memcpy( (char *)buf, (char *)(m_sBuffer+m_iRead), iDiv );
            memcpy( (char *)buf+iDiv, (char *)m_sBuffer, iReadLen-iDiv );
        }
    //::LeaveCriticalSection( &this->m_Lock );
    m_Lock.unlock();
    return iReadLen;
}
// 在文件末尾或合适位置添加实现
int QTCQueue::IndexOf(unsigned char value)
{
    // 一次加锁，结束前统一释放
    m_Lock.lock();

    // 计算当前有效字节数（不再调用 InUseCount()，避免二次加锁）
    unsigned int used = (m_iWrite + m_iMaxSize - m_iRead) % m_iMaxSize;
    for (unsigned int i = 0; i < used; ++i) {
        if (m_sBuffer[(m_iRead + i) % m_iMaxSize] == value) {
            m_Lock.unlock();
            return static_cast<int>(i);   // 找到，返回偏移
        }
    }

    m_Lock.unlock();
    return -1;                            // 未找到
}
