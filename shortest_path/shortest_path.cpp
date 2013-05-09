#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>

using namespace std;

struct edge{
	edge(int s, int d, int l): src(s), dest(d), length(l) {}
	int src;
	int dest;
	int length;
};

#define INFINITY	10000000

int id = 0;					// id generator
map<string, int> city_id;	// city name => array index mapping
map<int, string> id_city;	// array index => city name mapping
int* dist;					// adjacency matrix
int* next;					// for path construction 
int nvertices;				// number of vertices

void build_graph(const char* city_info_file)
{

	ifstream fin;

	fin.open (city_info_file, ifstream::in);
	if(!fin.good()){
		cerr<<"fail to open file: "<< city_info_file <<endl;
		exit(0);
	}

	cout<<"building graph...\t\t\t\t";
	vector<edge> edges;
	string src, dest;
	int length;
	//edge format: city1 city2 length
	while(fin>>src){
		fin>>dest>>length;
		if(length >= INFINITY){
			cerr<<"length of ( "<<src<<", "<<dest<<" ) is too long, must less than "<<INFINITY<<endl;
			exit(0);
		}
		if(city_id.find(src) == city_id.end()){
			id_city[id] = src;
			city_id[src] = id++;
		}
		if(city_id.find(dest) == city_id.end()){
			id_city[id] = dest;
			city_id[dest] = id++;
		}
		edges.push_back(edge(city_id[src], city_id[dest], length));
	}
	fin.close();
	nvertices = city_id.size();
	//alloction for adjacency matrix
	dist = new int[nvertices*nvertices];

	for(int i=0; i<nvertices; i++)
		for(int j=0; j<nvertices; j++)
			dist[i*nvertices+j] = INFINITY;	// INFINITY means no road between cities

	for(int i=0; i<edges.size(); i++){
		int s = edges[i].src;
		int d = edges[i].dest;
		int l = edges[i].length;
		dist[s*nvertices+d] = l;
	}

	next = new int[nvertices*nvertices];
	for(int i=0; i<nvertices; i++)
		for(int j=0; j<nvertices; j++)
			next[i*nvertices+j] = -1;	// -1 means no intermediate vertices
	/*
	map<string, int>::iterator it;
	for(it=city_id.begin(); it!=city_id.end(); it++){
		cout<<it->first<<", "<<it->second<<endl;
	}
	for(int i=0; i<nvertices; i++){
		for(int j=0; j<nvertices; j++)
			cout<<dist[i*nvertices+j]<<"\t";
		cout<<endl;
	}*/
	cout<<"finished."<<endl;
}

string path (int i, int j)
{
	int n = nvertices;
	if (dist[i*n+j] == INFINITY)
		return "no path";
	int interm = next[i*n+j];
	if (interm == -1){
		return " ";  // the direct edge from i to j gives the shortest path
	}
	else{
		return path(i, interm) + id_city[interm] + path(interm, j);
	}
}

void floyd()
{
	cout<<"computing shortest paths for all cities...\t";
	int n = nvertices;
	for(int k=0; k<n; k++)
		for(int i=0; i<n; i++)
			for(int j=0; j<n; j++)
				if(dist[i*n+k] + dist[k*n+j] < dist[i*n+j]){
					dist[i*n+j] = dist[i*n+k] + dist[k*n+j];
					next[i*n+j] = k;
				}
	cout<<"finished."<<endl<<endl;
	/*
	for(int i=0; i<n; i++){
		for(int j=0; j<n; j++){
			if(dist[i*n+j]>=INFINITY)
				cout<<"INFINITY"<<"\t";
			else
				cout<<dist[i*n+j]<<"\t";
		}
		cout<<endl;
	}
	for(int i=0; i<n; i++){
		for(int j=0; j<n; j++)
			cout<<path(i, j)<<"\t";
		cout<<endl;
	}
	*/

}

void handle_user_input()
{
	bool cont = true;
	string src, dest;
	cout<<"Now you get shortest path of any two cities."<<endl;
	while(cont){
		bool err_city = false;
		cout<<"Please input two cities:"<<endl;
		cin>>src>>dest;
		if(city_id.find(src) == city_id.end()){
			cout<<"no such city: "<<src<<endl;
			err_city = true;
		}
		if(city_id.find(dest) == city_id.end()){
			cout<<"no such city: "<<dest<<endl;
			err_city = true;
		}

		int i = city_id[src];
		int j = city_id[dest];
		if(!err_city){
			cout<<"the shortest path between '("<<src<<" "<<dest<<")' is: "
				<<id_city[i]<<path(i, j)<<id_city[j]<<endl;
			cout<<"length of the shortest path is: "<<dist[i*nvertices+j]<<endl;
		}
		cout<<"continue? [y/n]"<<endl;
		string s;
		cin>>s;
		cont = s == "y"? true : false;
	}
}

int main(int argc, char* argv[])
{
	if(argc != 2){
		cout<<"usage: shortest_path.exe <filename>"<<endl;
		exit(0);
	}

	build_graph(argv[1]);
	floyd();
	handle_user_input();

	return 0;
}
