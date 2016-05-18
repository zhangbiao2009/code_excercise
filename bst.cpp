#include <iostream>
#include <vector>
#include <stack>
#include <climits>

using namespace std;

struct Node{
    bool accessed;
    int data;
    Node* left;
    Node* right;
    Node(int v, Node* l=NULL, Node* r=NULL)
        : accessed(false), data(v), left(l), right(r) {}
};

class BST{
    private:
        Node* root;
        void insert(Node** np, int v)
        {
            if(*np == NULL){
                *np = new Node(v);
                return;
            }
            if(v < (*np)->data)
                insert(&(*np)->left, v);
            else
                insert(&(*np)->right, v);
        }

        void traverse(Node* np)
        {
            if(np == NULL)
                return;
            traverse(np->left);
            cout<<np->data<<endl;
            traverse(np->right);
        }

        void traverse2(Node* np)        // 非递归，这种方式的缺点是需要一个accessed标记
        {
            stack<Node*> s;
            s.push(np);
            while(!s.empty()){
                Node* tmp = s.top();
                s.pop();
                if(tmp){
                    if(!tmp->accessed){
                        s.push(tmp->right);
                        tmp->accessed = true;
                        s.push(tmp);
                        s.push(tmp->left);
                    }
                    else{
                        cout<<tmp->data<<endl;
                    }
                }
            }
        }

        void traverse3(Node* np)        // 非递归，无需标记
        {
            stack<Node*> s;

            Node* needp = np;
            while(1){
                for(auto p = needp; p != NULL; p = p->left)
                    s.push(p);

                if(!s.empty()){
                    Node* tmp = s.top();
                    s.pop();
                    cout<<tmp->data<<endl;
                    needp = tmp->right;
                }
                else break;
            }
        }

    public:
        BST(const vector<int> nodes)
            :root(NULL)
        {
            for(int i=0; i<nodes.size(); i++)
                insert(&root, nodes[i]);
        }

        void traverse()
        {
            traverse3(root);
        }

        bool isBST(Node* np, int* prevp)
        {
            if(np == NULL)
                return true;
            bool is = isBST(np->left, prevp);
            if(!is) return false;
            if(*prevp > np->data)
                return false;
            cout<<"prev: "<<*prevp<< ", current: "<<np->data<<endl;
            *prevp = np->data;
            return isBST(np->right, prevp);

        }

        bool checkBST()
        {
            int prev = INT_MIN;
            return isBST(root, &prev);
        }

};


int main()
{
    BST tree(vector<int>{5,3,9,6,10,4,2,7});
    cout<<tree.checkBST()<<endl;
    return 0;
}
