#include <assert.h>
#include <vector>
#include <string>
#include <random>
#include <array>
#include <iomanip>

#include "pretty_printing.h"

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


template<typename Matrix>
void diffusion_step(const Matrix &cur, Matrix &next) {
    assert(&cur != &next);
    next = cur;
    for (int i = 0; i < cur.size(); i++) {
        for (int j = 0; j < cur.front().size(); j++) {
            if (i > 0) {
                double d = (cur[i][j] - cur[i - 1][j]) * 0.2;
                next[i - 1][j] += d;
                next[i][j] -= d;
            }
            if (j > 0) {
                double d = (cur[i][j] - cur[i][j - 1]) * 0.2;
                next[i][j - 1] += d;
                next[i][j] -= d;
            }
        }
    }
}


const int M = 8;
class Diffusion {
    int w, h;
    int med_strength;

    array<array<array<double, 2*M + 1>, 2*M + 1>, M + 1> i_prop;

public:
    Diffusion() {}

    Diffusion(int w, int h, int med_strength) : w(w), h(h) {
        for (int i = 0; i < 2*M + 1; i++)
            for (int j = 0; j < 2*M + 1; j++)
                i_prop[0][i][j] = 0.0;
        i_prop[0][M][M] = med_strength;
        for (int t = 1; t < i_prop.size(); t++) {
            auto &cur = i_prop[t - 1];
            auto &next = i_prop[t];
            diffusion_step(cur, next);

            // for (const auto &row : next) {
            //     for (double cell : row)
            //         cerr << setw(5) << (int)(cell * 1000);
            //     cerr << endl;
            // }
            // cerr << endl;
        }
    }

    double reach(int dx, int dy, int dt) const {
        assert(dt >= 0);
        if (dt >= i_prop.size())
            return 0.0;
        if (dx < -M || dx > M)
            return 0.0;
        if (dy < -M || dy > M)
            return 0.0;
        return i_prop[dt][dx + M][dy + M];
    }

    double reach(int x1, int y1, int x2, int y2, int dt) const {
        // TODO: reflections
        return reach(x1 - x2, y1 - y2, dt);
    }
};


double spread_prob = -1.0;


struct Distr {
    double clean_prob;
    double inf_prob;

    double dead_prob() const {
        assert(clean_prob + inf_prob <= 1.0);
        return 1.0 - clean_prob - inf_prob;
    }

    Distr(double clean_prob, double inf_prob)
        : clean_prob(clean_prob), inf_prob(inf_prob) {
        assert(clean_prob >= 0.0);
        assert(clean_prob <= 1.0);
        assert(inf_prob >= 0.0);
        assert(inf_prob <= 1.0);
    }

    static Distr clean() {
        return Distr(1.0, 0.0);
    }
    static Distr infected() {
        return Distr(0.0, 1.0);
    }
    static Distr dead() {
        return Distr(0.0, 0.0);
    }

    Distr step(
        const Distr &n1, const Distr &n2,
        const Distr &n3, const Distr &n4) const {
        double nip =
            (1.0 - n1.inf_prob * spread_prob) *
            (1.0 - n2.inf_prob * spread_prob) *
            (1.0 - n3.inf_prob * spread_prob) *
            (1.0 - n4.inf_prob * spread_prob);
        return Distr(clean_prob * nip, clean_prob * (1.0 - nip));
    }

    void cure() {
        clean_prob += inf_prob;
        inf_prob = 0.0;
    }

    void check(const Distr reality) const {
        const double eps = 1e-8;

        if (reality.clean_prob > 0.5)
            assert(clean_prob > eps);
        else
            assert(clean_prob < 1 - eps);

        if (reality.inf_prob > 0.5)
            assert(inf_prob > eps);
        else
            assert(inf_prob < 1 - eps);

        if (reality.dead_prob() > 0.5)
            assert(dead_prob() > eps);
        else
            assert(dead_prob() < 1 - eps);
    }
};


typedef vector<vector<Distr>> Model;


Model slide_to_model(const vector<string> &slide) {
    vector<vector<Distr>> slide_model(
        slide.size() + 2, vector<Distr>(slide[0].size() + 2, Distr::clean()));

    for (int i = 0; i < slide.size(); i++) {
        for (int j = 0; j < slide[0].size(); j++) {
            Distr &c = slide_model[i + 1][j + 1];
            switch (slide[i][j]) {
                case 'C': c = Distr::clean(); break;
                case 'X': c = Distr::dead(); break;
                case 'V': c = Distr::infected(); break;
                default: assert(false); break;
            }
        }
    }

    return slide_model;
}


void show_model(ostream &out, const Model &model) {
    int w = min<int>(model.size() - 2, 20);
    int h = min<int>(model.size() - 2, 20);
    for (int i = 1; i <= h; i++) {
        for (int j = 1; j <= w; j++) {
            int q = model[i][j].clean_prob * 8.0002 + 0.9999;
            if (q) out << q; else out << ' ';
            out << ',';
            q = model[i][j].inf_prob * 8.0002 + 0.9999;
            if (q) out << q; else out << ' ';
            out << "  ";
        }
        out << endl;
    }
}

void update_model(const Model &cur, Model &next) {
    assert(&cur != &next);
    assert(cur.size() == next.size());
    assert(cur[0].size() == next[0].size());
    for (int i = 1; i < cur.size() - 1; i++) {
        for (int j = 1; j < cur[0].size() - 1; j++) {
            next[i][j] = cur[i][j].step(
                cur[i][j - 1], cur[i][j + 1],
                cur[i - 1][j], cur[i + 1][j]);
        }
    }
}


void check_model(const Model &prediction, const Model &reality) {
    assert(prediction.size() == reality.size());
    assert(prediction[0].size() == reality[0].size());
    for (int i = 0; i < prediction.size(); i++) {
        for (int j = 0; j < prediction[0].size(); j++) {
            prediction[i][j].check(reality[i][j]);
        }
    }
}


void cure_model(const vector<vector<double>> &med, Model &model) {
    assert(med.size() == model.size() - 2);
    assert(med[0].size() == model[0].size() - 2);
    for (int i = 0; i < med.size(); i++)
        for (int j = 0; j < med[0].size(); j++)
            if (med[i][j] >= 1.0) {
                if (model[i + 1][j + 1].inf_prob >= 0.5) {
                    // cerr << "CURED!!!!!!!!";
                    // debug2(j, i);
                }
                model[i + 1][j + 1].cure();
            }
}


class ViralInfection {
    int w, h;
    Diffusion diffusion;

public:
    int runSim(vector<string> slide,
               int med_strength, int kill_time, double spread_prob) {
        h = slide.size();
        w = slide[0].size();
        ::spread_prob = spread_prob;

        cerr << "# "; debug(w);
        cerr << "# "; debug(h);

        cerr << "# "; debug(med_strength);
        cerr << "# "; debug(kill_time);
        cerr << "# "; debug(spread_prob);

        diffusion = Diffusion(w, h, med_strength);

        vector<vector<double>> med(h, vector<double>(w, 0.0));

        auto model = slide_to_model(slide);
        show_model(cerr, model);
        cerr << endl;

        int iteration = 0;
        while (true) {
            int time_to_observation = kill_time;
            if (kill_time == 1)
                time_to_observation = 3;
            if (kill_time == 2)
                time_to_observation = 4;
            if (iteration)
                time_to_observation--;

            auto plan = make_plan(med, model, time_to_observation);
            for (int q = 0; q < time_to_observation; q++) {
                // drop
                auto pt = plan[q];
                if (pt.first == -1) {
                    Research::waitTime(1);
                } else {
                    int x = pt.first;
                    int y = pt.second;
                    Research::addMed(x, y);
                    med[y][x] += med_strength;
                }

                // cure
                cure_model(med, model);

                // spread
                if ((iteration + 1) % kill_time == 0) {
                    // cerr << "spread during plan execution" << endl;
                    auto new_model = model;
                    update_model(model, new_model);
                    model = new_model;
                }

                // diffuse
                auto new_med = med;
                diffusion_step(med, new_med);
                med = new_med;

                iteration++;
            }

            // observe and check
            slide = Research::observe();
            auto reality = slide_to_model(slide);
            check_model(model, reality);

            // cerr << "model:" << endl;
            // show_model(cerr, model);
            // cerr << endl;

            // cerr << "reality:" << endl;
            // show_model(cerr, reality);
            // cerr << endl;

            bool has_infection = false;
            for (const auto &row : slide)
                for (char c : row)
                    if (c == 'V')
                        has_infection = true;
            if (!has_infection) {
                break;
            }

            model = reality;

            // cure
            // cerr << "obs cure {" << endl;
            cure_model(med, model);
            // cerr << "}" << endl;

            // spread
            if ((iteration + 1) % kill_time == 0) {
                // cerr << "spread during observation" << endl;
                auto new_model = model;
                update_model(model, new_model);
                model = new_model;
            }

            // diffuse
            auto new_med = med;
            diffusion_step(med, new_med);
            med = new_med;

            iteration++;
        }

        return 0;
    }

    vector<pair<int, int>> make_plan(
        vector<vector<double>> med, Model model, int time_to_observation) {
        // cerr << "begin plan" << endl;
        vector<pair<int, int>> result;

        vector<vector<vector<double>>> med_prediction = {med};
        for (int i = 1; i < time_to_observation; i++) {
            med_prediction.push_back(med_prediction.back());
            assert(med_prediction.size() == i + 1);
            diffusion_step(med_prediction[i - 1], med_prediction[i]);
        }

        static default_random_engine gen;

        /*for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                double p = model[y + 1][x + 1].inf_prob;
                if (p > 0.01) {
                    debug2(x, y);
                    debug(p);
                }
            }
        }*/

        auto affected_by = [&](int x, int y, int t) {
            vector<pair<int, int>> result;
            for (int i = max(0, y - M); i < h && i <= y + M; i++) {
                for (int j = max(0, x - M); j < w && j <= x + M; j++) {
                    if (model[i + 1][j + 1].inf_prob < 0.3)
                        continue;
                    for (int k = t; k < time_to_observation; k++) {
                        int dt = time_to_observation - 1 - k;
                        if (diffusion.reach(x, y, j, i, dt) + 0.99 * med_prediction[k][i][j] >= 1.0) {
                            result.emplace_back(j, i);
                            break;
                        }
                    }
                }
            }
            return result;
        };

        for (int index = 0; index < time_to_observation; index++) {

            int best_x = -1;
            int best_y = -1;
            double best = 0;
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    double q = 0.0;
                    for (auto pt : affected_by(x, y, index)) {
                        // TODO: things eliminated at the last (spread) step
                        // are less certain and should be discounted.
                        q += exp(-(x + y) * 0.1) *
                            (model[pt.second + 1][pt.first + 1].inf_prob + 0.00001 * (x + y));
                    }
                    if (q > best) {
                        best_x = x;
                        best_y = y;
                        best = q;
                    }
                }
            }

            // debug(best);
            // debug2(best_x, best_y);

            result.emplace_back(best_x, best_y);
            if (best_x != -1) {
                // debug(affected_by(best_x, best_y, index));
                for (auto pt : affected_by(best_x, best_y, index)) {
                    model[pt.second + 1][pt.first + 1].cure();
                }
            }
        }

        // cerr << "end plan" << endl;
        assert(result.size() == time_to_observation);
        return result;
    }

};
