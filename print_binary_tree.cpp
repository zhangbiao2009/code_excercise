#include <iostream>
#include <vector>
#include <stack>
#include <climits>
#include <queue>

using namespace std;

struct Node{
    bool accessed;
    int data;
	int level;
	int idx;
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

		/*
		void traverse(Node* np, int level)
        {
			static int idx = 0;
            if(np == NULL)
                return;
            traverse(np->left, level+1);
			np->level = level;
			np->idx = idx;
			idx++;
			//cout<<np->data<<endl;
            traverse(np->right, level+1);
        }
		*/
		/*
		void traverse(Node* np, int& idx, int level)
        {
            if(np == NULL)
                return;
            traverse(np->left, idx, level+1);
			np->level = level;
			np->idx = idx;
			idx++;
			//cout<<np->data<<endl;
            traverse(np->right, idx, level+1);
        }
		*/
        int traverse(Node* np, int idx, int level)
        {
            if(np == NULL)
                return idx;
            idx = traverse(np->left, idx, level+1);
			np->level = level;
			np->idx = idx;
			idx++;
			//cout<<np->data<<endl;
            idx = traverse(np->right, idx, level+1);
			return idx;
        }

		void level_traverse(Node* root)
		{
			queue<Node*> q;
			if(root)
				q.push(root);
			Node* prev_np = NULL;
			while(!q.empty()) {
				Node* np = q.front();
				q.pop();
				if(np->left) q.push(np->left);
				if(np->right) q.push(np->right);
				int nspace = np->idx;
				if(prev_np){
					if(np->level != prev_np->level){
						cout<<endl;
					}
					else{
						nspace = np->idx - prev_np->idx - 1;
					}
				}

				for(int i=0; i<nspace; i++)
					cout<< ' ';
				cout<< np->data;

				prev_np = np;
			}
		}

    public:
        BST(const vector<int> nodes)
            :root(NULL)
        {
            for(int i=0; i<nodes.size(); i++)
                insert(&root, nodes[i]);
        }

		void print()
		{
			//int idx = 0;
			traverse(root, 0, 0);
			level_traverse(root);
		}

};


int main()
{
    BST tree(vector<int>{5,3,9,6,10,4,2,7});
	tree.print();
    return 0;
}
