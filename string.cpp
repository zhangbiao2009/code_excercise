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
        String& operator=(String tmp)	// exception safe assignment
        {
            cout<<"call ="<<endl;
            swap(sp, tmp.sp);
            return *this;
        }
        int refCount(){
            return sp->refCount;
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
    cout<<s.refCount()<<endl;
    return 0;
}
