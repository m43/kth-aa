#ifndef KTH_AA_UTILITY_H
#define KTH_AA_UTILITY_H

#include <memory>
#include <iostream>
#include <vector>
#include <tuple>
#include <algorithm>
#include <cmath>
#include <random>
#include <fstream>
#include <string>
#include <iomanip>

using namespace std;

template<class T>
class Grid { // https://stackoverflow.com/questions/936687/how-do-i-declare-a-2d-array-in-c-using-new
    size_t _rows;
    size_t _columns;
    T *data;

public:
    Grid(size_t rows, size_t columns)
            : _rows{rows},
              _columns{columns},
              data{new T[rows * columns]} {}

    ~Grid() {
        delete[] data;
    }

    [[nodiscard]] size_t rows() const { return _rows; }

    [[nodiscard]] size_t columns() const { return _columns; }

    T *operator[](size_t row) { return row * _columns + data; }

    T &operator()(size_t row, size_t column) {
        return data[row * _columns + column];
    }

    void print() {
        for (int r = 0; r < rows(); r++) {
            for (int c = 0; c < columns(); c++) {
                cout << (*this)[r][c] << " ";
            }
            cout << endl;
        }
        cout << endl;
    }
};


class UnionFind {
public:
    explicit UnionFind(size_t n) : _rank(n, 0)
//    , _size(n, 1)
    {
        _parent.resize(n);
        while (n--) _parent[n] = n;
    }

//    int size(int x) {
//        return _size[x];
//    }

    int find(int x) {
        if (_parent[x] == x) return x;
        return _parent[x] = find(_parent[x]); // path compression
    }

    void merge(int x, int y) {
        x = find(x), y = find(y);
        if (_rank[x] > _rank[y]) {
            _parent[y] = x;
//            _size[x] = _size[x] + _size[y];
            return;
        }

        _parent[x] = y;
//        _size[y] = _size[x] + _size[y];
        if (_rank[x] == _rank[y]) {
            _rank[x]++;
        }
    }

    bool equiv(int x, int y) {
        return find(x) == find(y);
    }

private:
    vector<int> _parent;
    vector<int> _rank;
//    vector<int> _size;
};

inline double squared_distance(const pair<double, double> &t1, const pair<double, double> &t2) {
    double dx = t1.first - t2.first;
    double dy = t1.second - t2.second;
    return dx * dx + dy * dy;
}

template<class T=int, class U=int>
U tour_distance(Grid<U> &distances, vector<T> tour) {
    U distance = 0;
    for (T i = 0; i < (T) tour.size(); i++) {
        distance += distances[tour[i]][tour[(i + 1) % distances.rows()]];
    }
    return distance;
}

vector<pair<double, double>> create_n_cities(int n, int seed);

template<class U=int>
inline Grid<U> *distance_matrix(const vector<pair<double, double>> &cities) {
    auto n = cities.size();
    auto matrix = new Grid<U>(n, n);
    for (unsigned i = 0; i < n; i++) {
        (*matrix)[i][i] = 0;
    }
    if (is_integral<U>::value) {
        for (unsigned i = 0; i < n - 1; i++) {
            for (unsigned j = i + 1; j < n; j++) {
                (*matrix)[i][j] = (*matrix)[j][i] = round(sqrt(squared_distance(cities[i], cities[j])));
            }
        }
    } else {
        for (unsigned i = 0; i < n - 1; i++) {
            for (unsigned j = i + 1; j < n; j++) {
                (*matrix)[i][j] = (*matrix)[j][i] = sqrt(squared_distance(cities[i], cities[j]));
            }
        }
    }

    return matrix;
}

template<class T, class U>
inline Grid<T> *k_nearest_neighbors(Grid<U> &distances, int k) {
    auto n = distances.rows();
    auto knn = new Grid<T>(n, k);

    auto cmp = [](const pair<U, T> &a, const pair<U, T> &b) { return a.first > b.first; };
    auto heap = vector<pair<U, T>>();
    heap.reserve(n);

    for (T i = 0; i < n; i++) {
        heap.clear();
        for (T j = 0; j < n; j++) {
            if (i != j) { heap.emplace_back(distances[i][j], j); }
        }
        make_heap(heap.begin(), heap.end(), cmp);

        for (T neighbor = 0; neighbor < k; neighbor++) {
            (*knn)[i][neighbor] = heap.front().second;
            pop_heap(heap.begin(), heap.end(), cmp);
            heap.pop_back();
        }
    }
    return knn;
}

// reverse tour segment if it makes the tour shorter given three vertices (each vertex is used for its left edge)
template<class T, class U>
inline int reverse_segment_3opt_seq(vector<T> *tour, int i, int j, int k, Grid<U> &distances, bool) {
    int a = (*tour)[(i - 1 + tour->size()) % tour->size()];
    int b = (*tour)[i];
    int c = (*tour)[j - 1];
    int d = (*tour)[j];
    int e = (*tour)[k - 1];
    int f = (*tour)[k % (*tour).size()];

    // original distance
    int d0 = distances[a][b] + distances[c][d] + distances[e][f];


    int d1 = distances[a][c] + distances[b][d] + distances[e][f];
    if (d0 > d1) {
        reverse(tour->begin() + i, tour->begin() + j);
        return d0 - d1;
    }

    int d2 = distances[a][b] + distances[c][e] + distances[d][f];
    if (d0 > d2) {
        reverse(tour->begin() + j, tour->begin() + k);
        return d0 - d2;
    }

    int d4 = distances[f][b] + distances[c][d] + distances[e][a];
    if (d0 > d4) {
        reverse(tour->begin() + i, tour->begin() + k);
        return d0 - d2;
    }

    int d3 = distances[a][d] + distances[e][b] + distances[c][f];
    if (d0 > d3) {
        vector<int> temp_tour = vector<int>{};
        temp_tour.insert(temp_tour.end(), tour->begin() + j, tour->begin() + k);
        temp_tour.insert(temp_tour.end(), tour->begin() + i, tour->begin() + j);
        copy_n(temp_tour.begin(), temp_tour.size(), &(*tour)[i]);
        return d0 - d3;
    }

    int d5 = distances[a][e] + distances[d][b] + distances[c][f];
    if (d0 > d5) {
        // get ac bd ef like in d1
        reverse(tour->begin() + i, tour->begin() + j);
        // get ae db cf
        reverse(tour->begin() + i, tour->begin() + k);
        return d0 - d5;
    }

    int d6 = distances[a][c] + distances[b][e] + distances[d][f];
    if (d0 > d6) {
        // get ac bd ef like with d1
        reverse(tour->begin() + i, tour->begin() + j);
        // now reverse from d to e
        reverse(tour->begin() + j, tour->begin() + k);
        return d0 - d6;
    }

    int d7 = distances[a][d] + distances[e][c] + distances[b][f];
    if (d0 > d7) {
        // get ab ce df like with d2
        reverse(tour->begin() + j, tour->begin() + k);
        // now reverse from d to b
        reverse(tour->begin() + i, tour->begin() + k);
        return d0 - d7;
    }

    return 0;
}

//template<class T, class U>
//inline int reverse_segment_3opt(vector<T> *tour, int i, int j, int k, Grid<U> &distances, bool apply) {
//    if (i < 0 or i >= j - 1 or j >= k - 1 or k > tour->size()) { // TODO remove
//        throw logic_error("Inconsistent state! Me no likey.");
//    }
//
//    int a = (*tour)[(i - 1 + tour->size()) % tour->size()];
//    int b = (*tour)[i];
//    int c = (*tour)[j - 1];
//    int d = (*tour)[j];
//    int e = (*tour)[k - 1];
//    int f = (*tour)[k % (*tour).size()];
//
//    // original distance
//    int d0 = distances[a][b] + distances[c][d] + distances[e][f];
//    int d1 = distances[a][c] + distances[b][d] + distances[e][f];
//    int d2 = distances[a][b] + distances[c][e] + distances[d][f];
//    int d3 = distances[a][d] + distances[e][b] + distances[c][f];
//    int d4 = distances[f][b] + distances[c][d] + distances[e][a];
//    int d5 = distances[a][e] + distances[d][b] + distances[c][f];
//    int d6 = distances[a][c] + distances[b][e] + distances[d][f];
//    int d7 = distances[a][d] + distances[e][c] + distances[b][f];
//
//    int best = min(min(min(min(min(min(d1, d2), d3), d4), d5), d6), d7);
//
//    if (best >= d0) return 0;
//    if (not apply) return d0 - best;
//
//    if (best == d1) {
//        reverse(tour->begin() + i, tour->begin() + j);
//    } else if (best == d2) {
//        reverse(tour->begin() + j, tour->begin() + k);
//        reverse(tour->begin() + j, tour->begin() + k);
//    } else if (best == d3) {
//        vector<int> temp_tour = vector<int>{};
//        temp_tour.insert(temp_tour.end(), tour->begin() + j, tour->begin() + k);
//        temp_tour.insert(temp_tour.end(), tour->begin() + i, tour->begin() + j);
//        copy_n(temp_tour.begin(), temp_tour.size(), &(*tour)[i]);
//    } else if (best == d4) {
//        reverse(tour->begin() + i, tour->begin() + k);
//    } else if (best == d5) {
//        // get ac bd ef like in d1
//        reverse(tour->begin() + i, tour->begin() + j);
//        // get ae db cf
//        reverse(tour->begin() + i, tour->begin() + k);
//    } else if (best == d6) {
//        // get ac bd ef like with d1
//        reverse(tour->begin() + i, tour->begin() + j);
//        // now reverse from d to e
//        reverse(tour->begin() + j, tour->begin() + k);
//    } else if (best == d7) {
//        // get ab ce df like with d2
//        reverse(tour->begin() + j, tour->begin() + k);
//        // now reverse from d to b
//        reverse(tour->begin() + i, tour->begin() + k);
//    } else {
//        throw runtime_error("inconsistent state");
//    }
//
//
//    return d0 - best;
//}

void log_cities(const vector<pair<double, double>> &cities, const string &path, const string &name);

template<class T>
void log_tour(vector<T> tour, const string &path, const string &name) {
    ofstream outfile(path, ios_base::app);
    outfile << "tour_name=" << name << "\n";
    outfile << "tour=[";
    for (auto c: tour) {
        outfile << c << ",";
    }
    outfile << "]\n";
    outfile.close();
}

#endif //KTH_AA_UTILITY_H