#include <string>
#include <iostream>

using namespace std;

bool stoi(const string& s, int& n)
{
    if(s.length() == 0)
        return false;
    bool minus = false;
    for(int i=0; i<s.length(); i++){
        if(i == 0){
            if(s[i] == '+')
                continue;
            else if(s[i] == '-'){
                minus = true;
                continue;
            }
            else if(!isdigit(s[i]))
                return false;
            else{
                n = s[i]-'0';
            }
        }
        else{
            if(!isdigit(s[i]))
                return false;
            n = n*10 + (s[i]-'0');
        }
    }
    if(minus)
        n = -n;
    return true;
}

int main()
{
    while(1){
        string str;
        int n;
        cin >> str;
        bool res = stoi(str, n);
        if(res)
            cout<<n<<endl;
        else
            cout<<"illegal integer"<<endl;
    }
    return 0;
}
