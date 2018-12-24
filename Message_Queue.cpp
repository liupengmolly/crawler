#include <deque>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>

using namespace std;
extern int sockets_num;
template<class DataType>

class Message_Queue
{
public:
    //构造函数，初始化2个互斥锁和1个条件变量

     Message_Queue():_nready(0)
    {
     pthread_mutex_init(&_mutex, NULL);
     pthread_mutex_init(&_ready_mutex, NULL);
     pthread_cond_init(&_cond, NULL);
    }

    int get_nready(){
        return _nready;
    }
    //推消息

    int push_msg(DataType &d)
    {
        //加锁_mutex，向queue里推消息

        pthread_mutex_lock(&_mutex);
         _queue.push_back(d);
        //printf("push_back\n");
        pthread_mutex_unlock(&_mutex);

        //加锁_ready_mutex，判断是否需要唤醒挂起的消费者

        pthread_mutex_lock(&_ready_mutex);
        if (!_nready)
            pthread_cond_signal(&_cond); //唤醒阻塞在此消息队列上的消费者

         _nready++; //计数器++

        pthread_mutex_unlock(&_ready_mutex);
        
        return 0;        
    }
    //取消息

    int get_msg(DataType &d)
    {
        //加锁，查看计数器，看是否需要挂起

        pthread_mutex_lock(&_ready_mutex);
        while (_nready == 0) //计数器为0，队列为空，挂起
              pthread_cond_wait(&_cond, &_ready_mutex); //为空时，所有消费者会阻塞在此

        //当被生产者唤醒时，消费者重新加_ready_mutex锁，_nready > 0, 程序跳出while(_nready)循环继续运行
        pthread_mutex_unlock(&_ready_mutex);
        
        //加锁，取队列操作
        pthread_mutex_lock(&_mutex); 
        d = _queue.front();
        _queue.pop_front();
        _nready--;
        // if(_nready==0 && (d.host!="news.sohu.com" || d.pagepath!="/")){
        //     cout<<"crawl end"<<endl;
        //     exit(0);
        // }
        //printf("pop_front\n");
        pthread_mutex_unlock(&_mutex);

        return 0;
    }


private:
    pthread_cond_t      _cond;
    int                 _nready;
    pthread_mutex_t     _ready_mutex;
    pthread_mutex_t     _mutex;
    deque<DataType>     _queue;
};

