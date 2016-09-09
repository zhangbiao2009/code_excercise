#include <iostream>
#include <vector>
#include <stack>
#include <climits>

using namespace std;

struct Node{
    int val;
    int start;
    int end;
    Node* left;
    Node* right;
    Node(int s, int e, Node* l=NULL, Node* r=NULL)
        : start(s), end(e), left(l), right(r) {}
};

class SegmentTree{
    private:
        Node* root;

        void traverse(Node* np, int d)
        {
            if(np == NULL)
                return;
            traverse(np->left, d+1);
            for(int i=0; i<d; i++)
                cout<<"\t";
            cout<<np->val<<"["<<np->start<<", "<<np->end<<"]"<<endl;
            traverse(np->right, d+1);
        }

        Node* createNode(const vector<int>& nodes, int start, int end) // [start, end]
        {
            Node* np = new Node(start, end);
            if(start < end){
                int mid = (start+end)/2;
                np->left = createNode(nodes, start, mid);
                np->right = createNode(nodes, mid+1, end);
                np->val = np->left->val+np->right->val;         // val is the sum of children's val
            }
            else{
                np->val = nodes[start];
            }
            return np;
        }

        int update_internal(Node* np, int i, int val)
        {
            if(np->start == np->end && np->start == i){       // found
                int delta = val - np->val;
                np->val = val;
                return delta;
            }

            int delta;
            if(i<np->right->start) 
                delta = update_internal(np->left, i, val);
            else
                delta = update_internal(np->right, i, val);
            np->val += delta;
            return delta;
        }
    public:
        SegmentTree(const vector<int>& nodes)
        {
            root = createNode(nodes, 0, nodes.size()-1);
        }

        void update(int i, int val) {
            update_internal(root, i, val);
        } 

        int sumRange_internal(Node* np, int i, int j)
        {
            if(i > j) return 0;
            if(np->start == i && np->end == j)
                return np->val;
            if(j<np->right->start)
                return sumRange_internal(np->left, i, j);
            if(i > np->left->end)
                return sumRange_internal(np->right, i, j);

            return sumRange_internal(np->left, i, np->left->end) + sumRange_internal(np->right, np->right->start, j);
        }

        int sumRange(int i, int j)
        {
            return sumRange_internal(root, i, j);
        }
        
        void traverse()
        {
            traverse(root, 0);
        }

};


int main()
{
    vector<int> v{5,3,9,6,10,4,2,7, 12, 15};
    SegmentTree tree(v);
    tree.traverse();
    cout<<tree.sumRange(1, 8)<<endl;
    /*
    tree.update(5, 14);
    cout<<endl;
    tree.traverse();
    */
    return 0;
}
