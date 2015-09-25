#include <assert.h>
#include <vector>
#include <string>
#include <random>
#include <array>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <chrono>
#include <functional>

#include "pretty_printing.h"

using namespace std;


#define debug(x) cerr << #x " = " << (x) << endl
#define debug2(x, y) cerr << #x " = " << (x) << ", " #y " = " << y << endl
#define debug3(x, y, z) \
    cerr << #x " = " << (x) << ", " #y " = " << y << ", " #z " = " << z << endl


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


const int M = 6;
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


int med_strength = -1;
int kill_time = -1;
double spread_prob = -1.0;
int w = -1;
int h = -1;
Diffusion diffusion;


struct PointSet {
    set<pair<int, int>> points;
    int min_idx;
    int max_idx;
    // TODO: add orthogonal index

    PointSet() : min_idx(100000), max_idx(-1) {}

    void add_point(int x, int y) {
        assert(x >= 0);
        assert(x < ::w);
        assert(y >= 0);
        assert(y < ::h);
        int idx = x + w * y;
        min_idx = min(min_idx, idx);
        max_idx = max(max_idx, idx);
        points.emplace(x, y);
    }

    bool dominates(const PointSet &other) const {
        if (points.size() < other.points.size())
            return false;

        if (min_idx > other.min_idx || max_idx < other.max_idx)
            return false;
        for (auto pt : other.points) {
            if (points.count(pt) == 0)
                return false;
        }
        return true;
    }
};


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

    void check(const Distr &reality) const {
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

    double dist(const Distr &other) const {
        return abs(clean_prob - other.clean_prob) +
               abs(inf_prob - other.inf_prob);
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
    int w = min<int>(model[0].size() - 2, 20);
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


struct CureFootprint {
    vector<PointSet> cured_sets;
    int x, y, t;

    bool empty() const {
        for (const auto &cs : cured_sets)
            if (!cs.points.empty())
                return false;
        return true;
    }

    int size() const {
        int result = 0;
        for (const auto &cs : cured_sets)
            result += cs.points.size();
        return result;
    }

    bool dominates(const CureFootprint &other) const {
        assert(cured_sets.size() == other.cured_sets.size());
        for (int i = 0; i < cured_sets.size(); i++) {
            if (!cured_sets[i].dominates(other.cured_sets[i]))
                return false;
        }
        return true;
    }
};


typedef map<pair<int, int>, double> Improvement;

double improvement_sum(const Improvement &imp) {
    double result = 0.0;
    for (const auto &kv : imp)
        result += kv.second;
    return result;
}

void improvement_merge_with(Improvement &imp, const Improvement &other) {
    for (const auto &kv : other) {
        double &q = imp[kv.first];
        q = max(q, kv.second);
    }
}


struct Modeller {
    vector<bool> phases;
    vector<vector<vector<double>>> med_prediction;
    vector<Model> model_prediction;

    Modeller(
        vector<vector<double>> med, Model model, vector<bool> phases)
        : phases(phases),
          med_prediction({med}),
          model_prediction({model}) {

        for (bool phase : phases) {
            // cure
            cure_model(med_prediction.back(), model_prediction.back());

            if (phase) {
                // spread
                model_prediction.push_back(model_prediction.back());
                update_model(
                    model_prediction[model_prediction.size() - 2],
                    model_prediction[model_prediction.size() - 1]);
            }

            // diffuse
            med_prediction.push_back(med_prediction.back());
            diffusion_step(
                med_prediction[med_prediction.size() - 2],
                med_prediction[med_prediction.size() - 1]);
        }

        // TODO: cure as well
    }

    CureFootprint make_cure_footprint(int x0, int y0, int t0) const {
        CureFootprint result;
        result.x = x0;
        result.y = y0;
        result.t = t0;
        result.cured_sets.resize(model_prediction.size());

        assert(t0 <= phases.size());
        int start_model_idx = count(phases.begin(), phases.begin() + t0, true);

        for (int y = max(0, y0 - M); y < ::h && y <= y0 + M; y++) {
            for (int x = max(0, x0 - M); x < ::w && x <= x0 + M; x++) {
                if (abs(x - x0) + abs(y - y0) > M)
                    continue;
                // TODO: precompute start value of model_idx,
                // and iterate from t0
                int model_idx = start_model_idx;
                for (int t = t0; t < phases.size() && t <= t0 + M; t++) {
                    // cure
                    const auto &model = model_prediction[model_idx];
                    if (model[y + 1][x + 1].inf_prob > 1e-6) {
                        if (::diffusion.reach(x, y, x0, y0, t - t0) +
                            0.99 * med_prediction[t][y][x] >= 1.0) {
                            result.cured_sets[model_idx].add_point(x, y);
                        }
                    }

                    // spread
                    if (phases[t])
                        model_idx++;

                    // diffuse (implicitly in loop increment)
                }
                // TODO: cure as well
            }
        }

        return result;
    }

    Improvement simulate(const vector<CureFootprint> &footprints) {
        Improvement improvement;

        set<pair<int, int>> changed;

        vector<pair<Distr*, Distr>> old_values;

        for (int q = 0; q < model_prediction.size(); q++) {
            bool last = q + 1 == model_prediction.size();

            set<pair<int, int>> all_cured_points;
            for (const auto &f : footprints) {
                for (const auto &pt : f.cured_sets[q].points) {
                    int x = pt.first + 1;
                    int y = pt.second + 1;
                    all_cured_points.emplace(x, y);
                }
            }

            set<pair<int, int>> neighbors_to_update;
            for (const auto &pt : changed) {
                int x = pt.first;
                int y = pt.second;
                if (x - 1 > 0)
                    neighbors_to_update.emplace(x - 1, y);
                if (x + 1 <= ::w)
                    neighbors_to_update.emplace(x + 1, y);
                if (y - 1 > 0)
                    neighbors_to_update.emplace(x, y - 1);
                if (y + 1 <= ::h)
                    neighbors_to_update.emplace(x, y + 1);
            }
            changed.clear();
            for (const auto &pt : neighbors_to_update) {
                int x = pt.first;
                int y = pt.second;

                const auto &prev_model = model_prediction[q - 1];
                auto &model = model_prediction[q];

                auto new_distr = all_cured_points.count(pt) > 0
                    ? Distr::clean()
                    : prev_model[y][x].step(
                        prev_model[y - 1][x],
                        prev_model[y + 1][x],
                        prev_model[y][x - 1],
                        prev_model[y][x + 1]);

                if (new_distr.dist(model[y][x]) > 1e-6) {
                    double delta = new_distr.clean_prob - model[y][x].clean_prob;

                    // TODO: this assertion fails, why?
                    // assert(delta >= -1e-6);

                    if (last && delta > 1e-3) {
                        assert(improvement.count(pt) == 0);
                        improvement[pt] = delta;
                    }

                    old_values.emplace_back(&model[y][x], model[y][x]);
                    model[y][x] = new_distr;
                    changed.insert(pt);
                }
            }

            // TODO: change should only be considered change after
            // spread step together with cure step.

            for (auto pt : all_cured_points) {
                int x = pt.first;
                int y = pt.second;
                auto &distr = model_prediction[q][y][x];
                // Some points can be visited second time, but then condition
                // will be false, so we won't update them twice.
                if (distr.inf_prob > 1e-6) {
                    if (last && distr.inf_prob > 1e-3) {
                        assert(improvement.count(pt) == 0);
                        improvement[pt] = distr.inf_prob;
                    }

                    old_values.emplace_back(&distr, distr);
                    distr.cure();
                    changed.insert(pt);
                }
            }
        }

        reverse(old_values.begin(), old_values.end());
        for (auto kv : old_values) {
            *kv.first = kv.second;
        }

        // TODO: assert that model_prediction did not change
        return improvement;
    }
};


class ViralInfection {
public:

    vector<pair<int, int>> make_plan(
        vector<vector<double>> med, Model model, int time_to_observation, int start_iteration) {

        vector<bool> phases;

        int T = time_to_observation + (::kill_time == 1 ? 3 : 5);

        for (int i = 0; i < T; i++)
            phases.push_back(start_iteration + i > 0 &&
                             (start_iteration + i + 1) % ::kill_time == 0);
        debug(phases);

        Modeller modeller(med, model, phases);

        vector<pair<CureFootprint, Improvement>> choices;

        for (int t = 0; t < time_to_observation; t++) {
            vector<CureFootprint> cure_footprints;
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    auto cfp = modeller.make_cure_footprint(x, y, t);
                    if (!cfp.empty())
                        cure_footprints.push_back(cfp);
                }
            }
            debug2(t, cure_footprints.size());

            sort(cure_footprints.begin(), cure_footprints.end(),
                [](const CureFootprint &a, const CureFootprint &b) {
                    return a.size() > b.size();
                });

            // TODO: among equivalent items, pick ones that go ahead of the
            // frontier as much as possible.
            vector<CureFootprint> frontier_cure_footprints;
            for (const auto &cfp : cure_footprints) {
                bool to_add = true;
                for (const auto &d : frontier_cure_footprints) {
                    if (d.dominates(cfp)) {
                        to_add = false;
                        break;
                    }
                }
                if (to_add)
                    frontier_cure_footprints.push_back(cfp);
            }
            debug(frontier_cure_footprints.size());

            double frontier_speed =
                sqrt(::med_strength) * ::kill_time / min(::w, ::h);

            for (const auto &fp : frontier_cure_footprints) {
                auto imp = modeller.simulate({fp});
                for (auto &kv : imp) {
                    int x = kv.first.first;
                    int y = kv.first.second;
                    kv.second *= exp(-(x + y) * 0.02 / frontier_speed);
                }
                choices.emplace_back(fp, imp);
            }
            // #for (auto fp : frontier_cure_footprints)
            // debug2(frontier_cure_footprints.front().cured_sets[0].points,
            //        frontier_cure_footprints.front().cured_sets[1].points);
            // debug(modeller.simulate({frontier_cure_footprints.front()}));
            // debug(modeller.simulate({frontier_cure_footprints.back()}));
        }

        debug(choices.size());

        // vector<bool> free_slots(time_to_observation, true);
        vector<pair<int, int>> sol(time_to_observation, {-1, -1});
        Improvement accum;
        while (true) {
            double best_improvement = improvement_sum(accum);
            Improvement new_accum;

            int best_t = -1;
            int best_x;
            int best_y;

            for (const auto &choice : choices) {
                const auto &fp = choice.first;
                const auto &extra_imp = choice.second;

                if (sol[fp.t].first != -1)
                    continue;

                Improvement tmp_imp = accum;
                improvement_merge_with(tmp_imp, extra_imp);
                double new_sum = improvement_sum(tmp_imp);
                if (new_sum > best_improvement) {
                    best_improvement = new_sum;
                    best_t = fp.t;
                    best_x = fp.x;
                    best_y = fp.y;

                    new_accum = tmp_imp;
                }
            }
            if (best_t == -1)
                break;
            sol[best_t] = {best_x, best_y};
            debug3(best_t, best_x, best_y);
            accum = new_accum;
        }
        debug(sol);
        debug(improvement_sum(accum));
        return sol;
    }

    int runSim(vector<string> slide,
               int med_strength, int kill_time, double spread_prob) {
        ::h = slide.size();
        ::w = slide[0].size();
        ::med_strength = med_strength;
        ::kill_time = kill_time;
        ::spread_prob = spread_prob;

        cerr << "# "; debug(w);
        cerr << "# "; debug(h);

        cerr << "# "; debug(med_strength);
        cerr << "# "; debug(kill_time);
        cerr << "# "; debug(spread_prob);

        ::diffusion = Diffusion(w, h, med_strength);

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
            // if (kill_time == 3)
            //     time_to_observation = 6;
            if (iteration)
                time_to_observation--;

            auto plan = make_plan(med, model, time_to_observation, iteration);
            assert(plan.size() == time_to_observation);
            for (int q = 0; q < time_to_observation; q++) {

                // if (all_of(plan.begin() + q, plan.end(),
                //            bind1st(equal_to<pair<int, int>>(), make_pair(-1, -1)))) {
                //     cerr << "early exit" << endl;
                //     debug(q);
                //     return 0;
                // }

                // drop
                auto pt = plan[q];
                if (pt.first == -1) {
                    Research::waitTime(1);
                    // this_thread::sleep_for(std::chrono::seconds(3));
                } else {
                    int x = pt.first;
                    int y = pt.second;
                    Research::addMed(x, y);
                    // this_thread::sleep_for(std::chrono::seconds(3));
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

                bool has_virus = false;
                for (int y = 0; y < ::h; y++)
                    for (int x = 0; x < ::w; x++)
                        if (model[y + 1][x + 1].inf_prob > 1e-8)
                            has_virus = true;
                if (!has_virus) {
                    return 0;
                }

                iteration++;
            }

            // observe and check
            slide = Research::observe();
            // this_thread::sleep_for(std::chrono::seconds(3));
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

            bool has_virus = false;
            for (int y = 0; y < ::h; y++)
                for (int x = 0; x < ::w; x++)
                    if (model[y + 1][x + 1].inf_prob > 1e-8)
                        has_virus = true;
            if (!has_virus) {
                return 0;
            }

            iteration++;
        }

        return 0;
    }
};
