#include <iostream>
#include <string>
#include <vector>

using namespace std;

vector<vector<int> >* build_dfa(const string& p)
{
    vector<vector<int> >* dfa = new vector<vector<int> >(256);
    for(int c=0; c<dfa->size(); c++)
        (*dfa)[c].resize(p.length());

    (*dfa)[p[0]][0] = 1;
    for(int X=0, j=1; j<p.length(); j++){
        for(int c=0; c<dfa->size(); c++)
            (*dfa)[c][j] = (*dfa)[c][X];    // copy for mismatch cases
        (*dfa)[p[j]][j] = j+1;         // for match case
        X = (*dfa)[p[j]][X];            // update restart X
    }
    return dfa;
}

void search(const string& text, const string& p)
{
    vector<vector<int> >* dfa = build_dfa(p);
    int j = 0;
    for(int i=0; i<text.length(); i++){
        j = (*dfa)[text[i]][j];
        if(j == p.length()){    // match
            cout<<"matched: "<<endl;
            cout<<text<<endl;
            cout<<string(i-p.length()+1, ' ') + text.substr(i-p.length()+1, p.length())<<endl;
            cout<<"at: "<<i-p.length()+1<<endl;
            j = 0;      // continue search next occurrence
        }
    }
}

int main()
{
    string text = "acadacaeacadacacadacaeacadacacy";
    string p = "acadacaeacadacacy";
    search(text, p);
    return 0;
}
