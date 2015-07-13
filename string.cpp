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

class String;

class CRef{
    public:
        CRef(String* ss, int nn)
            : s(ss), n(nn) { }
        char operator = (char c);
        operator char ();
    private:
        String* s;
        int n;
};

class String{
    public:
        friend class CRef;
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
        char operator[](int n) const
        {
            cout<<"called 1"<<endl;
            return sp->p[n];
        }
        CRef operator[](int n)
        {
            cout<<"called 2"<<endl;
            return CRef(this, n);
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

char CRef::operator = (char c){
    cout<< "operator = called" <<endl;
    if(s->sp->refCount > 1){
        //clone string
        StringRep* p = new StringRep(s->sp->p);
        s->sp->refCount--;
        s->sp = p;
    }
    s->sp->p[n] = c;
    return s->sp->p[n];
}

CRef::operator char (){
    cout<< "operator char() called" <<endl;
    return s->sp->p[n];
}

int main()
{
    String s1("hello");
    cout<<s1.refCount()<<endl;
    String s2;
    s2 = s1;
    cout<<"s1: "<<s1.str()<<endl;
    cout<<"s2: "<<s2.str()<<endl;
    String s3("hi");
    s2 = s3;
    cout<<s1.refCount()<<endl;
    cout<<"s1: "<<s1.str()<<endl;
    cout<<"s2: "<<s2.str()<<endl;
    cout<<"s3: "<<s3.str()<<endl;
    //s2 = "bye";                         // 不需要为C字符串另外提供assignment operator.
    s3[1] = s2[0] = 'c';
    //cout<<s2[0]<<endl;
    cout<<"s1: "<<s1.str()<<endl;
    cout<<"s2: "<<s2.str()<<endl;
    cout<<"s3: "<<s3.str()<<endl;
    return 0;
}
