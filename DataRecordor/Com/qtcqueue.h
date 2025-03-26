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
    inline int 	InUseCount();
	//	队列中空闲的空间
    inline int 	FreeCount();
	//	在队列中插入数据
    inline int 	Add( char c );
    inline int	Add( void *buf,int iLen=1 );
	//  从队列中取数据
    inline int 	Get();
    inline int	Get( void *buf,int iLen = _QUEUE_SIZE );
	//  查看队列中的数据
    inline int 	Peek();
    inline int	Peek( void *buf,int iLen = _QUEUE_SIZE );
	//	移动指定队列的指针
	inline void	MoveReadP(  unsigned int iSteps=1 );		//????????n
	inline void	MoveWriteP( unsigned int iSteps=1 );		//????????n
	//	取得队列的指针
	inline unsigned int GetWriteP()	{ return m_iWrite; }
	inline unsigned int GetReadP()	{ return m_iRead;  }
	//清空队列
	void Empty();
};

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

///////////////////////////////////////////////////

#endif // QTCQUEUE_H
