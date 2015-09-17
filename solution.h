#include <assert.h>
#include <vector>
#include <string>
#include <random>

// #include "pretty_printing.h"

using namespace std;


#define debug(x) cerr << #x " = " << (x) << endl;
#define debug2(x, y) cerr << #x " = " << (x) << ", " #y " = " << y << endl;


typedef vector<string> Slide;

bool has_infection(const Slide &slide) {
    for (const auto &row : slide)
        if (row.find('V') != string::npos)
            return true;
    return false;
}


class ViralInfection {
    int w, h;

public:
    int runSim(vector<string> slide,
               int med_strength, int kill_time, double spread_prob) {
        h = slide.size();
        w = slide[0].size();
        cerr << "# "; debug(w);
        cerr << "# "; debug(h);

        cerr << "# "; debug(med_strength);
        cerr << "# "; debug(kill_time);
        cerr << "# "; debug(spread_prob);

        for (int t = 0; ; t++) {
            debug(t);
            if (!has_infection(slide))
                break;

            if (t > 0 && t % kill_time == 0 && (kill_time > 1 || t % 3 == 0)) {
                slide = Research::observe();
                continue;
            }

            int x = -1;
            int y = -1;
            for (int s = 0; s < w + h - 1; s++) {
                for (int i = 0; i < h && i <= s; i++) {
                    int j = s - i;
                    assert(j >= 0);
                    if (j >= w)
                        continue;

                    if (slide[i][j] == 'V') {
                        x = j; y = i; break;
                    }
                }
                if (x != -1)
                    break;
            }

            assert(x != -1);
            assert(y != -1);
            slide[y][x] = 'C';
            Research::addMed(x, y);
        }

        return 0;
    }
};
