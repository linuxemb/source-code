#include <servicepool.h>
using namespace std;
namespace bbt{
ServicePool::ServicePool(int size):cond_(mutex_)
{
	int i;
	MutexLock lock(mutex_);
	for(i=0;i<size;i++)
	{
		pool_.push(new SocketService);
	}
}

SocketService* ServicePool::getService()
{
	MutexLock lock(mutex_);
	while(pool_.empty())
		cond_.wait();
	SocketService *svc=pool_.front();
	pool_.pop();
	return svc;
}

void ServicePool::freeService(SocketService *svc)
{
	MutexLock lock(mutex_);
	pool_.push(svc);
	cond_.signal(0);
}

ServicePool::~ServicePool()
{
	SocketService *svc;
	int size;
	MutexLock lock(mutex_);
	size=pool_.size();
	for(int i=0;i<size;i++)
	{
		svc=pool_.front();
		pool_.pop();
		delete svc;
	}
}

	
} //namespace bbt
