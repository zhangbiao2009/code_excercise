#include <iostream>
#include <queue>
#include <thread>
#include <condition_variable>
#include <memory>
#include <vector>
#include <unistd.h>
#include <assert.h>

using namespace std;

class ThreadInfo;
mutex queueMutex;
queue<shared_ptr<ThreadInfo>> threadQueue;

class ThreadInfo{
    private:
        condition_variable _cv;
        int _id;
    public:
        ThreadInfo(int id):_id(id){}
        void start()
        {
            while(1){
                unique_lock<mutex> lck(queueMutex);
                assert(!threadQueue.empty());
                auto sp = threadQueue.front();
                while(this != sp.get()){
                    _cv.wait(lck);
                    sp = threadQueue.front();
                }
                threadQueue.pop();      // pop myself
                if(!threadQueue.empty()){       // wake up the new leader if exist
                    auto newLeader = threadQueue.front();
                    newLeader->wakeUp();
                }
                lck.unlock();
                cout<<"hello, i'm thread "<<_id<<endl;
                sleep(1);
                lck.lock();
                threadQueue.push(sp);
            }
        }
        void wakeUp(){
            _cv.notify_one();
        }
};

int main()
{

    vector<thread> tvec;
    for(int i=0; i<5; i++){
        shared_ptr<ThreadInfo> sp(new ThreadInfo(i));
        queueMutex.lock();
        threadQueue.push(sp);
        queueMutex.unlock();
        tvec.push_back(thread(&ThreadInfo::start, sp.get()));
    }
    sleep(5000);

    return 0;
}
