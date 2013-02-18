#include <iostream>
#include <map>
#include <string>

using namespace std;

typedef struct Node{
	int key;
	int val;
	struct Node* prev;
	struct Node* next;
}Node;

const int MAX_CACHE_SIZE = 1;

//circular doubly linked list
static Node* head = NULL;
static map<int, Node*> cache;

Node* new_node(int key, int val)
{
	Node* np = new Node;
	np->key = key;
	np->val = val;
	np->prev = np->next = np; //point to itself
	return np;
}

Node* find(int key)
{
	map<int, Node*>::iterator iter= cache.find(key);
	if(iter == cache.end())
		return NULL;
	return iter->second;
}

void remove_from_list(Node* node)
{
	if(node == head){ //max cache size is only 1, remove the head from the list
		head = NULL;
	}
	node->prev->next = node->next;
	node->next->prev = node->prev;
}

//put it to the front
void promote(Node* node)
{
	if(!head){ //first node
		head=node;
		return;
	}

	if(node == head)
		return; //do nothing

	if(node == head->prev){
		head = head->prev;
		return;
	}

	if(node->prev != node && node->next != node){
		//not a new node, already in the list, remove it from the list
		remove_from_list(node);
	}

	//insert node in the front
	node->next = head;
	node->prev = head->prev;
	head->prev->next = node;
	head->prev = node;
	head = node;
}

void set(int key, int val)
{
	Node* node = find(key);
	if(!node){
		node = new_node(key, val);
		if(cache.size() >= MAX_CACHE_SIZE){
			//evict an entry according to LRU policy (from list tail)
			Node* tmp = head->prev;
			remove_from_list(tmp);
			cache.erase(tmp->key);
			delete tmp;
		} 
		//store the address in the cache
		cache[key] = node;
	}else
		node->val = val; //set the value

	//put it in list front
	promote(node);
}

Node* get(int key)
{
	Node* node = find(key);
	if(node){
		//put it in list front
		promote(node);
	}
	return node;
}

int main()
{
	string cmd;
	int key, val;
	while(1){
		cout<<"please input command:"<<endl;
		cin>>cmd;
		if(cmd == "get"){
			cin>>key;
			Node* np = get(key);
			if(np)
				cout<<"("<<np->key<<", "<<np->val<<")"<<endl;
			else
				cout<<"null"<<endl;
		}else if(cmd == "set"){
			cin>>key>>val;
			set(key, val);
		}else{
			return 0;
		}
		cout<<"current cache status:"<<endl;
		if(!head)
			continue;
		cout<<"("<<head->key<<", "<<head->val<<") ";
		for(Node* np=head->next; np!=head; np=np->next){
			cout<<"("<<np->key<<", "<<np->val<<") ";
		}
		cout<<endl;
		/*
		cout<<"current cache status from map:"<<endl;
		for(map<int, Node*>::iterator iter= cache.begin(); iter!=cache.end(); iter++){
			cout<<"("<<iter->first<<", "<<iter->second->key<<", "<<iter->second->val<<") ";
		}
		cout<<endl;
		*/
	}
	return 0;
}
