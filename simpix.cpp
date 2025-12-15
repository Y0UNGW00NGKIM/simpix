#include "TROOT.h"
#include "TASImage.h"

#include <getopt.h>
#include <assert.h>

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <cstring>

using namespace std;

static inline long long pixel_cost(UInt_t p, UInt_t q) {
    int pr = (p >> 16) & 0xff;
    int pg = (p >> 8) & 0xff;
    int pb = p & 0xff;

    int qr = (q >> 16) & 0xff;
    int qg = (q >> 8) & 0xff;
    int qb = q & 0xff;

    int dr = pr - qr;
    int dg = pg - qg;
    int db = pb - qb;

    return (long long)dr * dr + (long long)dg * dg + (long long)db * db;
}

static inline int clamp_int(int v, int lo, int hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static void print_usage() {
    cout << "Usage:\n";
    cout << "  ./simpix -s <source_image> -t <target_image> [-o out.png]\n";
    cout << "          [--moves N] [--melt M] [--t_start T0] [--t_end T1] [--alpha A]\n";
    cout << "          [--radius R] [--seed S] [--save_every K] [--log_every L]\n";
}

int main(int argc, char **argv) {
    TString fsrc;
    TString ftgt;
    TString fout = "out.png";

    long long n_moves = 20000000;
    long long n_melt = 0;
    double t_start = 50000.0;
    double t_end = 10.0;
    double alpha = -1.0;
    int radius = 50;
    unsigned long long seed = 0;
    long long save_every = 1000000;
    long long log_every = 1000000;

    static struct option long_options[] = {
        {"source", required_argument, 0, 's'},
        {"target", required_argument, 0, 't'},
        {"out", required_argument, 0, 'o'},
        {"moves", required_argument, 0, 'n'},
        {"melt", required_argument, 0, 'm'},
        {"t_start", required_argument, 0, 'T'},
        {"t_end", required_argument, 0, 'E'},
        {"alpha", required_argument, 0, 'a'},
        {"radius", required_argument, 0, 'r'},
        {"seed", required_argument, 0, 'S'},
        {"save_every", required_argument, 0, 'v'},
        {"log_every", required_argument, 0, 'l'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int long_index = 0;
    while ((opt = getopt_long(argc, argv, "s:t:o:n:m:T:E:a:r:S:v:l:h", long_options, &long_index)) != -1) {
        if (opt == 's') {
            fsrc = optarg;
        } else if (opt == 't') {
            ftgt = optarg;
        } else if (opt == 'o') {
            fout = optarg;
        } else if (opt == 'n') {
            n_moves = atoll(optarg);
        } else if (opt == 'm') {
            n_melt = atoll(optarg);
        } else if (opt == 'T') {
            t_start = atof(optarg);
        } else if (opt == 'E') {
            t_end = atof(optarg);
        } else if (opt == 'a') {
            alpha = atof(optarg);
        } else if (opt == 'r') {
            radius = atoi(optarg);
        } else if (opt == 'S') {
            seed = strtoull(optarg, nullptr, 10);
        } else if (opt == 'v') {
            save_every = atoll(optarg);
        } else if (opt == 'l') {
            log_every = atoll(optarg);
        } else if (opt == 'h') {
            print_usage();
            return 0;
        } else {
            print_usage();
            return 1;
        }
    }

    if (fsrc.Length() == 0 || ftgt.Length() == 0) {
        print_usage();
        return 1;
    }

    gROOT->SetBatch(kTRUE);

    cout << "Reading images: source= " << fsrc << " target= " << ftgt << endl;
    cout << "Output image= " << fout << endl;

    TASImage *src = new TASImage(fsrc.Data());
    TASImage *tgt = new TASImage(ftgt.Data());
    TASImage *out = new TASImage(*src);

    assert(src->GetWidth() == tgt->GetWidth() && src->GetHeight() == tgt->GetHeight());

    int w = (int)src->GetWidth();
    int h = (int)src->GetHeight();
    Long_t numPix = (Long_t)w * (Long_t)h;

    cout << "Pixel Geometry: " << w << " x " << h << endl;
    cout << "num_pixels " << (long long)numPix << endl;

    UInt_t *outPix = out->GetArgbArray();
    UInt_t *tgtPix = tgt->GetArgbArray();

    auto t0 = chrono::high_resolution_clock::now();

    long long energy = 0;
    for (Long_t i = 0; i < numPix; i++) {
        energy += pixel_cost(outPix[i], tgtPix[i]);
    }

    long long initial_energy = energy;
    vector<UInt_t> best_pix((size_t)numPix);
    memcpy(best_pix.data(), outPix, (size_t)numPix * sizeof(UInt_t));
    long long best_energy = energy;

    if (seed == 0) {
        seed = (unsigned long long)chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    mt19937_64 rng(seed);
    uniform_real_distribution<double> uni01(0.0, 1.0);
    uniform_int_distribution<Long_t> idx_dist(0, numPix - 1);
    uniform_int_distribution<int> off_dist(-radius, radius);

    if (alpha <= 0.0) {
        if (n_moves > 0 && t_start > 0.0 && t_end > 0.0) {
            alpha = pow(t_end / t_start, 1.0 / (double)n_moves);
        } else {
            alpha = 1.0;
        }
    }

    cout << "seed " << seed << endl;
    cout << "melt_moves " << n_melt << endl;
    cout << "moves " << n_moves << endl;
    cout << "t_start " << t_start << endl;
    cout << "t_end " << t_end << endl;
    cout << "alpha " << alpha << endl;
    cout << "radius " << radius << endl;

    long long accepted = 0;
    long long attempted = 0;
    double t = t_start;

    for (long long step = 0; step < n_melt; step++) {
        Long_t i = idx_dist(rng);
        Long_t j = idx_dist(rng);
        if (i == j) {
            continue;
        }

        long long old_cost = pixel_cost(outPix[i], tgtPix[i]) + pixel_cost(outPix[j], tgtPix[j]);
        long long new_cost = pixel_cost(outPix[j], tgtPix[i]) + pixel_cost(outPix[i], tgtPix[j]);
        long long dE = new_cost - old_cost;

        attempted += 1;

        bool accept = false;
        if (dE <= 0) {
            accept = true;
        } else {
            double r = uni01(rng);
            double p = exp(-(double)dE / t_start);
            if (r < p) {
                accept = true;
            }
        }

        if (accept) {
            UInt_t tmp = outPix[i];
            outPix[i] = outPix[j];
            outPix[j] = tmp;
            energy += dE;
            accepted += 1;
        }
    }

    attempted = 0;
    accepted = 0;
    t = t_start;

    for (long long step = 0; step < n_moves; step++) {
        Long_t i;
        Long_t j;

        if (radius > 0) {
            Long_t idx = idx_dist(rng);
            int x1 = (int)(idx % w);
            int y1 = (int)(idx / w);

            int dx = off_dist(rng);
            int dy = off_dist(rng);

            int x2 = clamp_int(x1 + dx, 0, w - 1);
            int y2 = clamp_int(y1 + dy, 0, h - 1);

            i = (Long_t)y1 * (Long_t)w + (Long_t)x1;
            j = (Long_t)y2 * (Long_t)w + (Long_t)x2;

            if (i == j) {
                continue;
            }
        } else {
            i = idx_dist(rng);
            j = idx_dist(rng);
            if (i == j) {
                continue;
            }
        }

        long long old_cost = pixel_cost(outPix[i], tgtPix[i]) + pixel_cost(outPix[j], tgtPix[j]);
        long long new_cost = pixel_cost(outPix[j], tgtPix[i]) + pixel_cost(outPix[i], tgtPix[j]);
        long long dE = new_cost - old_cost;

        attempted += 1;

        bool accept = false;
        if (dE <= 0) {
            accept = true;
        } else {
            double r = uni01(rng);
            double p = exp(-(double)dE / t);
            if (r < p) {
                accept = true;
            }
        }

        if (accept) {
            UInt_t tmp = outPix[i];
            outPix[i] = outPix[j];
            outPix[j] = tmp;
            energy += dE;
            accepted += 1;
        }

        t *= alpha;

        if (save_every > 0 && (step + 1) % save_every == 0) {
            if (energy < best_energy) {
                memcpy(best_pix.data(), outPix, (size_t)numPix * sizeof(UInt_t));
                best_energy = energy;
            }
        }

        if (log_every > 0 && (step + 1) % log_every == 0) {
            double acc = attempted > 0 ? (double)accepted / (double)attempted : 0.0;
            cout << "step " << (step + 1) << " T " << t << " energy " << energy << " best " << best_energy << " acc " << acc << endl;
            attempted = 0;
            accepted = 0;
        }
    }

    memcpy(outPix, best_pix.data(), (size_t)numPix * sizeof(UInt_t));

    out->WriteImage(fout.Data());

    auto t1 = chrono::high_resolution_clock::now();
    double dt = chrono::duration<double>(t1 - t0).count();

    cout << "initial_energy " << initial_energy << endl;
    cout << "best_energy " << best_energy << endl;
    cout << "time_s " << dt << endl;

    return 0;
}
