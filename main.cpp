#include<iostream>
#include<vector>
#include<thread>
#include<chrono>
#include <iomanip>

void copy(int start, int end, double* a, double* c) {
    for (int j = start; j < end; j++) c[j] = a[j];
}

void scale(int start, int end, double* b, double* c, double s) {
    for (int j = start; j < end; j++) {b[j] = s * c[j];}
}

void add(int start, int end, double* a, double* b, double* c) {
    for (int j = start; j < end; j++) {c[j] = a[j] + b[j];}
}

void triad(int start, int end, double* a, double* b, double* c, double s) {
    for (int j = start; j < end; j++) {a[j] = b[j] + s * c[j];}
}

double mysecond() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

int main(int argc, char** argv) {    
    int THREADS = (argc > 1) ? std::stoi(argv[1]) : 1;
    int ArraySize = (argc > 2) ? std::stoi(argv[2]) : 1e8; // it's bigger than 4x the chace 
    int LOOP = (argc > 3) ? std::stoi(argv[3]) : 10;


    double *a = new double[ArraySize];
    double *b = new double[ArraySize]; 
    double *c = new double[ArraySize];

    for (int j = 0; j < ArraySize; j++) { // does this effects the cpu branching?
        a[j] = 1.0;
        b[j] = 2.0;
        c[j] = 0.0;
    }

    double **times = new double*[4]; // one for very function
    for (int i = 0; i < 4; i++) times[i] = new double[LOOP]; // one for every iation

    int chunk = ArraySize / THREADS, scalar = 3;

    for (int i = 0; i < LOOP; i++) {

        times[0][i] = mysecond();
        {
            std::vector<std::thread> pool;
            for (int t = 0; t < THREADS; t++) {
                int start = t * chunk;
                int end = (t == THREADS - 1) ? ArraySize : start + chunk;
                pool.push_back(std::thread(copy, start, end, a, c));
            }
            for (auto &th : pool) th.join(); // time issue ???????? i have no idea
        }
        times[0][i] = mysecond() - times[0][i];

        times[1][i] = mysecond();
        {
            std::vector<std::thread> pool;
            for (int t = 0; t < THREADS; t++) {
                int start = t * chunk;
                int end = (t == THREADS - 1) ? ArraySize : start + chunk;
                pool.push_back(std::thread(scale, start, end, b, c, scalar));
            }
            for (auto &th : pool) th.join();
        }
        times[1][i] = mysecond() - times[1][i];

        times[2][i] = mysecond();
        {
            std::vector<std::thread> pool;
            for (int t = 0; t < THREADS; t++) {
                int start = t * chunk;
                int end = (t == THREADS - 1) ? ArraySize : start + chunk;
                pool.push_back(std::thread(add, start, end, a, b, c));
            }
            for (auto &th : pool) th.join(); 
        }
        times[2][i] = mysecond() - times[2][i];

        times[3][i] = mysecond();
        {
            std::vector<std::thread> pool;
            for (int t = 0; t < THREADS; t++) {
                int start = t * chunk;
                int end = (t == THREADS - 1) ? ArraySize : start + chunk;
                pool.push_back(std::thread(triad, start, end, a, b, c, scalar));
            }
            for (auto &th : pool) th.join();
        }
        times[3][i] = mysecond() - times[3][i];
    }

    double avgtime[4] = {0}, maxtime[4] = {0};
    double mintime[4] = {1ll << 62, 1ll << 62, 1ll << 62, 1ll << 62};
    const char* label[4] = {"Copy:      ", "Scale:     ", "Add:       ", "Triad:     "};
    double bytes[4] = {
        2.0 * sizeof(double) * ArraySize, // copy
        2.0 * sizeof(double) * ArraySize, // scale
        3.0 * sizeof(double) * ArraySize, // add
        3.0 * sizeof(double) * ArraySize // triad
    };

    int skip = 2; // this handels the cold start
    for (int i = skip; i < LOOP; i++) {
        for (int j = 0; j < 4; j++) {
            avgtime[j] += times[j][i];
            mintime[j] = std::min(mintime[j], times[j][i]);
            maxtime[j] = std::max(maxtime[j], times[j][i]);
        }
    }

    std::cout << "Function    Best Rate MB/s  Avg time     Min time     Max time\n";
    for (int i = 0; i < 4; i++) {
        avgtime[i] = avgtime[i] / (double)(LOOP - skip);
        std::cout << label[i] << std::fixed << std::setprecision(1) << 1.0E-06 * bytes[i] / mintime[i] 
        << "  " << std::setprecision(6) << avgtime[i] << "  " << mintime[i] << "  " << maxtime[i] << "\n";
    }
    std::cout << "-------------------------------------------------------------\n";

    delete[] a; delete[] b; delete[] c;
    for (int i = 0; i < 4; i++) delete[] times[i];
    delete[] times;
    return 0;
}