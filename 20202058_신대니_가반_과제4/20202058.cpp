#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <cmath>
#include <random>

using namespace std;

#define S 10 // promising size
#define N 20

random_device rd;
mt19937 gen(rd());
uniform_int_distribution<int> dis(0, 999);
uniform_int_distribution<int> dis_(0, 5);
uniform_int_distribution<int> dis_2(0, N-1);


vector<vector<double>> vertex;
double W[1000][1000];

inline double dist(vector<double> &x, vector<double> &y){
	return sqrt(pow(x[0] - y[0], 2) + pow(x[1] - y[1], 2));
}

///////////////////////  Class Path  //////////////////////////

class Path{
	private :
	double tot_cost;
	int firstIndex;
	int lastIndex;
	double best_cost;
	double avg;


	public :
	vector<int> x;
	// vector<double> cost;
	Path();
	Path(vector<int> a);
	~Path();

	double get_cost();
	double get_bestcost();
	double get_avg();
	int get_firstIndex();
	int get_lastIndex();

	void repath(vector<int> a);

	void cal_cost();

	bool operator> (Path& pa);
	bool operator>= (Path& pa);
	bool operator== (Path& pa);
	bool operator!= (Path& pa);
};

Path::Path(){
	x.resize(1000);
	// cost.resize(100);
	firstIndex = -1;
	lastIndex = -1;
	tot_cost = 0;
	best_cost = 0;
	avg = 0;
}

Path::Path(vector<int> a){
	x.swap(a);
	// cost.resize(100);
	cal_cost();
}

Path::~Path(){
	// x.~vector();
}

double Path::get_cost(){return tot_cost;}

double Path::get_bestcost(){return best_cost;}

double Path::get_avg(){return avg;}

int Path::get_firstIndex(){return firstIndex;}

int Path::get_lastIndex(){return lastIndex;}

void Path::repath(vector<int> a){
	x.swap(a);
	cal_cost();
}

void Path::cal_cost(){
	tot_cost = 0;
	firstIndex = 0;
	best_cost = 0;
	avg = 0;
	double curcost = 0;

	int j = 0, k = 0;

	for (int i=0; i<S; i++){
		tot_cost += W[j][x[j]];
		j = x[j];
	}

	best_cost = curcost = tot_cost;
	lastIndex = j;

	int newfirst = 0;
	for (int i=S; i<1000; i++){
		double curdist = W[j][x[j]];
		tot_cost += curdist;

		curcost = curcost + curdist - W[newfirst][x[newfirst]];

		if (curcost<best_cost){
			best_cost = curcost;
			firstIndex = x[newfirst];
			lastIndex = x[j];
		}
		j = x[j];
		newfirst = x[newfirst];
	}
	avg = tot_cost/1000;
}

bool Path::operator> (Path& pa){
	if (this->tot_cost > pa.tot_cost) {return true;}
	else {return false;}
}

bool Path::operator>= (Path& pa){
	if (this->tot_cost < pa.tot_cost) {return false;}
	else {return true;}
}

bool Path::operator== (Path& pa){
	return tot_cost == pa.tot_cost;
}

bool Path::operator!= (Path& pa){
	if (tot_cost == pa.tot_cost) {return false;}
	else {return true;}
}


//////////////////////////////////////////////////////

bool comper(Path &a, Path &b){
	return b>a;
}

bool eq(Path &a, Path &b){
	return a==b;
}

double checkCost(Path &pa, int index){
	double costSum = 0.0;
	for (int i=0; i<10; i++){
		costSum += W[index][pa.x[index]];
		index = pa.x[index];
	}
	return costSum;
}

Path crossover(Path &a, Path &b){  // a < b
	vector<int> newX;
	newX.assign(1000, -1);
	
	int index = a.get_firstIndex();

	for (int i=0; i<S; i++){
		newX[index] = a.x[index];
		index = a.x[index];
	}

	for (int k=S; k<1000; k++){
		double a_cost = checkCost(a, index);
		double b_cost = checkCost(b, index);

		if (a_cost < b_cost){
			if (W[index][a.x[index]] > a.get_avg()*2){
				if (newX[b.x[index]] == -1){
					newX[index] = b.x[index];
					index = b.x[index];	
				} else{
					double minimum = 200;
					int minindex = a.get_firstIndex();
					for (int j=0; j<1000; j++){
						if ((W[index][j]<minimum)&&(index!=j)&&(newX[j]==-1)){
							minimum = W[index][j];
							minindex = j;
						}
					}
					newX[index] = minindex;
					index = minindex;
				}
			}else if (newX[a.x[index]] == -1){
				newX[index] = a.x[index];
				index = a.x[index];
			}else if (newX[b.x[index]] == -1){
				newX[index] = b.x[index];
				index = b.x[index];
			}else {
				double minimum = 200;
				int minindex = a.get_firstIndex();
				for (int j=0; j<1000; j++){
					if ((W[index][j]<minimum)&&(index!=j)&&(newX[j]==-1)){
						minimum = W[index][j];
						minindex = j;
					}
				}
				newX[index] = minindex;
				index = minindex;
			}
		}else {
			if (W[index][b.x[index]] > b.get_avg()*2){
				if (newX[a.x[index]] == -1){
					newX[index] = a.x[index];
					index = a.x[index];	
				} else{
					double minimum = 200;
					int minindex = a.get_firstIndex();
					for (int j=0; j<1000; j++){
						if ((W[index][j]<minimum)&&(index!=j)&&(newX[j]==-1)){
							minimum = W[index][j];
							minindex = j;
						}
					}
					newX[index] = minindex;
					index = minindex;
				}
			}else if (newX[b.x[index]] == -1){
				newX[index] = b.x[index];
				index = b.x[index];	
			}else if (newX[a.x[index]] == -1){
				newX[index] = a.x[index];
				index = a.x[index];
			}else {
				double minimum = 200;
				int minindex = a.get_firstIndex();
				for (int j=0; j<1000; j++){
					if ((W[index][j]<minimum)&&(index!=j)&&(newX[j]==-1)){
						minimum = W[index][j];
						minindex = j;
					}
				}
				newX[index] = minindex;
				index = minindex;
			}
		}
	}
	Path pa(newX);

	return pa;
}

void mutation(Path &a){
	for (int i=0; i<100; i++){
		int first = dis(gen);
		int index = first;
		double firstCost = 0.0;
		int newX[7];
		for (int i=0; i<7; i++){
			firstCost += W[index][a.x[index]];
			index = a.x[index];
			newX[i] = index;
		}
		for (int i=0; i<20; i++){
			int a = dis_(gen); 
			int b = dis_(gen);
			int tmp = newX[a];
			newX[a] = newX[b];
			newX[b] = tmp;
		}
		double curCost = 0.0;
		index = first;
		for (int i=0; i<7; i++){
			curCost += W[index][newX[i]];
			index = newX[i];
		}
		if (curCost < firstCost){
			// printf("change X 			-- strat --\n");
			index = first;
			for (int i=0; i<7; i++){
				a.x[index] = newX[i];
				index = newX[i];
			}
			a.cal_cost();
		}
	}
}




int main(){
	ifstream data("TSP.csv");
	fstream fs;
    string line;

    while(getline(data,line)){
        stringstream lineStream(line);
        string cell;
        vector<double> row;
        while(getline(lineStream,cell,',')){
            row.push_back(stod(cell));
        }
        vertex.push_back(row);
    }
	data.close();

	for (int i=0; i<1000; i++){
		for (int j=i+1; j<1000; j++){
			W[i][j] = W[j][i] = dist(vertex[i], vertex[j]);
		}
	}

	list<Path> p;
	list<Path>::iterator itor = p.begin();
	list<Path>::iterator itor2;



	vector<int> aa(1000);

	Path pa;
	for (int k=0; k<N; k++){
		aa.assign(1000, -1);
		int index = dis(gen);
		int firstI = index;
		for (int i=0; i<1000; i++){
			double minimum = 200;
			int minindex = firstI;
			for (int j=0; j<1000; j++){
				if ((W[index][j]<minimum)&&(index!=j)&&(aa[j]==-1)){
					minimum = W[index][j];
					minindex = j;
				}
			}
			aa[index] = minindex;
			index = minindex;
		}
		pa.repath(aa);
		p.push_back(pa);
	}
	for (int k=0; k<2; k++){
		aa.assign(1000, -1);
		int index = dis(gen);
		int firstI = index;
	}

	p.sort(comper);
	// fs.open("cost.csv", ios_base::out);
	for (int c=0; c<2000; c++){
		for (int i=0; i<N/2; i++){
			itor = itor2 = p.begin();
			int a = dis_2(gen);
			int b = dis_2(gen);

			for (int j=0; j<a; j++){itor++;}
			for (int j=0; j<b; j++){itor2++;}

			if (itor->get_bestcost() < itor2->get_bestcost()){
				p.push_back(crossover(*itor, *itor2));
			} else {
				p.push_back(crossover(*itor2, *itor));
			}
			// printf("Crossover : %16.f\n", p.back().get_cost());
		}

		p.sort(comper);

		for (itor=p.begin(); itor!=p.end(); itor++){
			mutation(*itor);
		}
		p.sort(comper);
		p.unique(eq);
		// for (itor=p.begin(); itor!=p.end(); itor++){
		// 	for (itor2=itor; itor2!=p.end();){
		// 		itor2++;
		// 		if (*itor == *itor2){
		// 			itor2 = p.erase(itor2);
		// 		}
		// 	}
		// }
		while(p.size()>N) {p.pop_back();}

		// printf("GEN %d : %.16f\n", c, p.front().get_cost());
		// fs << "GEN" << c << ", " << p.front().get_cost() << endl;
	}
	// fs.close();
///////////////////////////////////////////////////////////


	
	fs.open("20202058.csv", ios_base::out);
	int index = 0;
	for(int i=0; i<1000; i++){
		int j = p.front().x[index];
		fs << j << endl;
		index = j;
	}
	fs.close();
	
	return 0;
}