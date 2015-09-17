#include <iostream>
#include <vector>
#include <string>
#include <cassert>

using namespace std;


class Research {
public:
    static int addMed(int x, int y) {
        cout << "ADDMED" << endl;
        cout << x << " " << y << endl;
        cout.flush();
        int reply;
        cin >> reply;
        assert(reply == 0);
        return reply;
    }

    static vector<string> observe() {
        cout << "OBSERVE" << endl;
        cout.flush();
        int H;
        cin >> H;
        vector<string> slide(H);
        for (auto &row : slide)
            cin >> row;
        return slide;
    }

    static int waitTime(int t) {
        cout << "WAITTIME" << endl;
        cout << t << endl;
        cout.flush();
        int reply;
        cin >> reply;
        assert(reply == 0);
        return reply;
    }
};


#include "solution.h"


int main(int argc, char **argv) {
    debug2(argc, argv);

    int H;
    cin >> H;
    debug(H);

    vector<string> slide(H);
    for (auto &row : slide)
        cin >> row;

    int med_strength;
    cin >> med_strength;

    int kill_time;
    cin >> kill_time;

    double spread_prob;
    cin >> spread_prob;

    ViralInfection().runSim(slide, med_strength, kill_time, spread_prob);

    cout << "END" << endl;
    cout.flush();
    return 0;
}
