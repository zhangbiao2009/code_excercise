#include <iostream>
#include <cstring>

using namespace std;

struct StringRep{
    StringRep(const char* str)
    {
        cout<<"create"<<endl;
        p = strdup(str);
        refCount = 1;
    }
    ~StringRep(){
        delete p;
    }
    char* p;
    int refCount;
};

class String{
    public:
        String():sp(NULL) {}
        String(const char* str)
            :sp(new StringRep(str)) {}
        String(const String& s)
        {
            sp = s.sp;
            sp->refCount++;
        }
        ~String(){
            cout<<"destruct"<<endl;
            if(sp && --sp->refCount == 0){
                cout<<"call delete"<<endl;
                delete sp;
            }
        }
        String& operator=(String tmp)	// exception safe assignment，这个利用了上面实现的copy constructor
        {
            cout<<"call ="<<endl;
            swap(sp, tmp.sp);   // 如果原来sp有内容，不必费心显式销毁，因为已经交给了tmp，tmp析构时会替你销毁掉
            return *this;
        }
        int refCount(){
            return sp->refCount;
        }
        char* str(){
            if(sp)
                return sp->p;
            return NULL;
        }

    private:
        StringRep* sp;
};

int main()
{
    String s("hello");
    cout<<s.refCount()<<endl;
    String s2;
    s2 = s;
    cout<<"s: "<<s.str()<<endl;
    cout<<"s2: "<<s2.str()<<endl;
    String s3("hi");
    s2 = s3;
    cout<<s.refCount()<<endl;
    cout<<"s: "<<s.str()<<endl;
    cout<<"s2: "<<s2.str()<<endl;
    cout<<"s3: "<<s3.str()<<endl;
    return 0;
}
