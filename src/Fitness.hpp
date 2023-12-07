#ifndef __W_FITNESS_
#define __W_FITNESS_

#include <functional>
#include <iostream>
#include <mutex>
#include <semaphore.h>
#include <sstream>
#include <string>
#include <thread>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/lexical_cast.hpp>

#include <pagmo/algorithm.hpp>
#include <pagmo/algorithms/pso_gen.hpp>
#include <pagmo/detail/constants.hpp>
#include <pagmo/detail/visibility.hpp>
#include <pagmo/io.hpp>
#include <pagmo/population.hpp>
#include <pagmo/problem.hpp>
#include <pagmo/types.hpp>

#include "Machine.hpp"
#include "MiscUtils.hpp"

#define SESSION_FILE_ALG "fuzzing_alg.bin"
#define SESSION_FILE_POP "fuzzing_pop.bin"
#define SESSION_FILE "fuzzing_session.gzip"

using namespace std;
using namespace pagmo;
using namespace misc_utils;

// Enum that holds types for stop conditions
enum FuzzingStopCondition {
    TIME,
    ITERATIONS
};

typedef struct {

public:
    uint32_t iterations;
    uint32_t current_fitness;
    uint population_size;
    uint32_t random_seed;
    vector<vector_double> X; // Population for each generation
    vector<vector_double> Y; // Fitnesses for each fitness evaluation

private:
    friend class boost::serialization::access;
    template <typename Archive>
    void serialize(Archive &ar, unsigned)
    {
        // clang-format off
        ar &iterations
           &current_fitness
           &population_size
           &random_seed
           &X
           &Y;
        // clang-format on
    }
} fitness_session_t;

class Fitness {
    char *TAG = "[Optimizer] ";

private:
    // Main class vars
    bool _enable;
    uint _individual_size;
    uint _population_size;
    uint _generation_size;
    uint _iterations;
    double _opt_signal;
    double _current_fitness;
    string _session_dir = "";
    // Iteration callback vars
    stringstream _ss;
    thread *_thread_opt = nullptr;
    function<void(const vector_double &)> callback_iteration = nullptr;
    function<void(const FuzzingStopCondition stop_code)> callback_stop_condition = nullptr;
    // Optimizer vars
    algorithm *_algo = nullptr;
    problem *_p0 = nullptr;
    population _pop;
    fitness_session_t _session;
    uint32_t _rand_seed;
    bool _request_session_recovery;
    bool _request_init_population;
    bool _session_restored;
    sem_t _sem_thread_opt;
    sem_t _sem_iter;
    sem_t _iter_done;

    // Stop Condition vars
    std::shared_ptr<React::TimeoutWatcher> stop_cond_timeout = nullptr;

public:
    vector_double x; // Current Population

    struct problem_basic {
        Fitness *obj = nullptr;

        problem_basic() = default;

        problem_basic(Fitness *_obj)
            : obj(_obj)
        {
        }

        // Mandatory, computes the fitness
        vector_double fitness(const vector_double &x) const
        {

            if (obj->_request_session_recovery) {
                // Recovery last session if requested
                if (obj->_iterations < obj->_session.Y.size()) {
                    return {obj->_session.Y[obj->_iterations++]};
                }
                else {
                    obj->_request_session_recovery = false;
                    // Recover fitness from previous iteration
                    obj->_current_fitness = obj->_session.Y.back()[0];
                    GL1M(obj->TAG, "Starting after the last iteration below:");
                    obj->x = obj->_session.X.back();
                    obj->print_summary();
                }
            }

            obj->_iterations++;
            if (obj->_enable)
                obj->x = x; // Update X population on main object.

            if (obj->_request_init_population) {
                obj->_request_init_population = false;
                // Indicate that the initial population is ready to be used.
                sem_post(&obj->_sem_thread_opt);
            }
            sem_post(&obj->_iter_done);
            // Wait cost function insertion from fuzzer
            sem_wait(&obj->_sem_iter);

            obj->print_summary();
            if (obj->_enable) {
                obj->update_session();
            }

            return {obj->_current_fitness * obj->_opt_signal};
        }

        // Mandatory, returns the box-bounds
        std::pair<vector_double, vector_double> get_bounds() const
        {
            std::pair<vector_double, vector_double> x_bounds;
            for (size_t i = 0; i < obj->_individual_size; i++) {
                // Set bounds here
                x_bounds.first.push_back(0.0);
                x_bounds.second.push_back(0.2);
            }
            return x_bounds;
        }

        // Optional, provides a name for the problem overrding the default name
        std::string get_name() const
        {
            return "WDissector";
        }

        // Optional, provides extra information that will be appended after
        // the default stream operator
        std::string get_extra_info() const
        {
            return "WDissector Fuzzing Engine.";
        }
    };

private:
    void problem_handler()
    {
        enable_idle_scheduler();

        // Init optimizer
        _p0 = new problem{problem_basic{this}};
        _algo = new algorithm{pso_gen(_generation_size)};
        _algo->extract<pso_gen>()->set_verbosity(1);
        // Set seeds
        _algo->extract<pso_gen>()->set_seed(_rand_seed);
        try {
            _p0->set_seed(_rand_seed);
        }
        catch (const std::exception &e) {
            // Ignore
        }

        if (StateMachine.config.fuzzing.restore_session_on_startup) {
            LoadSession();
        }

        // Check time stop condition and start timeout
        auto &stop_cond_cfg = StateMachine.config.fuzzing.stop_condition;
        if (stop_cond_cfg.stop_on_max_time_minutes &&
            stop_cond_cfg.stop_on_max_time_minutes) {
            stop_cond_timeout = loop.onTimeout(60 * stop_cond_cfg.max_time_minutes, [this] {
                if (stop_cond_cfg.stop_on_max_time_minutes) {
                    GL1M(TAG, "Timeout reached");
                    if (this->callback_stop_condition != nullptr) {
                        this->callback_stop_condition(FuzzingStopCondition::TIME);
                    }
                }
            });
        }

        // the constructor will eventualy trigger sem_post(&_sem_thread_opt)
        _pop = population{*_p0, _population_size, _rand_seed};

        while (true) {
            _pop = _algo->evolve(_pop);
            puts("pop = algo->evolve(pop);");
        }

        if (stop_cond_timeout != nullptr) {
            stop_cond_timeout->cancel();
        }
    }

    void print_summary()
    {

        uint x_size = x.size();
        bool limit_size = false;
        if (x_size >= 7) {
            limit_size = true;
        }
        // Print Params
        _ss.str("");
        _ss << "[";

        for (size_t i = 0; i < x_size; i++) {
            _ss << x[i];
            if (limit_size && i == 5) {
                _ss << ",...,";
                _ss << x[i + 1];
                break;
            }
            else if (i != x_size - 1)
                _ss << ",";
        }
        _ss << "]";

        GL1Y("--------------------------------------------------------");
        GL1Y(TAG, "Iter=", _iterations, "  Params=", _ss.str());
        GL1Y(TAG, "Fitness=", _current_fitness,
             "  Adj. Fitness=", _current_fitness * _opt_signal);

        if (_enable) {
            static uint last_logs_size = 0;
            // Print Algorithm log
            auto &full_logs = _algo->extract<pso_gen>()->get_log();
            if (full_logs.size() != last_logs_size) {
                last_logs_size = full_logs.size();

                auto &algo_log = full_logs.back();
                GL1Y(TAG, "Gen:", to_string(get<0>(algo_log)), ", ",
                     "Fevals:", to_string(get<1>(algo_log)), ", ",
                     "gbest:", to_string(get<2>(algo_log)), ", ",
                     "Mean Vel.:", to_string(get<3>(algo_log)), ", ",
                     "Mean lbest:", to_string(get<4>(algo_log)), ", ",
                     "Avg. Dist.:", to_string(get<5>(algo_log)), ", ");
            }
        }

        if (callback_iteration)
            callback_iteration(x);

        GL1Y("--------------------------------------------------------");
    }

    bool update_session()
    {
        if (!StateMachine.config.fuzzing.restore_session_on_startup)
            return false;
        // Update session vars
        _session.iterations = _iterations;
        _session.current_fitness = _current_fitness;
        _session.X.push_back(x);
        _session.Y.push_back({_current_fitness * _opt_signal});

        return false;
    }

public:
    int init(uint individual_size = 1,
             uint population_size = 5,
             uint generation_size = 200,
             bool maximization = true,
             const string session_save_dir = "")
    {
        _individual_size = individual_size;
        _population_size = population_size;
        _generation_size = generation_size;
        _iterations = 0;
        _current_fitness = 1e6;
        _enable = true;
        _request_session_recovery = false;
        _request_init_population = false;
        _session_restored = false;

        if (session_save_dir == "")
            _session_dir = "./logs/"s + StateMachine.config.name + "/";
        else
            _session_dir = session_save_dir;

        if (maximization)
            _opt_signal = -1.0;
        else
            _opt_signal = 1.0;

        _rand_seed = StateMachine.config.fuzzing.random_seed;
        _session.population_size = _population_size;
        _session.random_seed = _rand_seed;
        _request_init_population = true;

        sem_init(&_sem_thread_opt, 0, 0);
        sem_init(&_sem_iter, 0, 0);
        _thread_opt = new thread(&Fitness::problem_handler, this);
        pthread_setname_np(_thread_opt->native_handle(), "thread_optimizer");
        _thread_opt->detach();
        sem_wait(&_sem_thread_opt); // Wait first population

        _enable = StateMachine.config.fuzzing.enable_optimization;

        if (!_enable && !_session_restored) {
            SetDefaultX();
            GL1Y(TAG, "Optimization disabled. Using default population:");
            print_summary();
        }

        if (_enable) {
            auto algo_summary = boost::lexical_cast<std::string>(*_algo);
            GL1Y(TAG, algo_summary);
            stringstream ss;
            ss << *_p0 << endl;
            GL1Y("\n", ss.str());
        }

        GL1Y(TAG, "Initialized with X Size=", _individual_size, ", Population Size=", _population_size);

        return 0;
    }

    void SetEnable(bool val)
    {
        _enable = val;
    }

    void SetSessionFolder(const string &path)
    {
        _session_dir = path + "/";
    }

    bool SaveSession()
    {
        static mutex mutex_save_session;
        lock_guard<mutex> lock(mutex_save_session);

        if (x.size() == 0 || !_enable) {
            return false;
        }

        ofstream file_session(_session_dir + SESSION_FILE);

        if (!file_session.is_open())
            goto error;

        try {

            boost::iostreams::filtering_ostream filter;
            filter.push(boost::iostreams::gzip_compressor());
            filter.push(file_session);
            boost::archive::binary_oarchive oarchive(filter);
            oarchive << _session;
            // Set full rw permission
            system(("chmod 0666 " + _session_dir + SESSION_FILE).c_str());
        }
        catch (const std::exception &e) {
            GL1R(e.what());
            goto error;
        }

        GL1G(TAG, "Session file saved to ", _session_dir, SESSION_FILE);
        return true;

    error:
        GL1R(TAG, "Session files failed to save");
        return false;
    }

    bool LoadSession()
    {
        ifstream file_session(_session_dir + SESSION_FILE);

        if (!file_session.is_open())
            goto error;

        try {

            boost::iostreams::filtering_istream filter;
            filter.push(boost::iostreams::gzip_decompressor());
            filter.push(file_session);
            boost::archive::binary_iarchive iarchive(filter);
            iarchive >> _session;

            // Sanity checks on the population
            if (!_session.X.size()) {
                GL1R(TAG, "Empty population");
                goto error;
            }
            else if (_session.population_size != _population_size) {
                GL1R(TAG, "Wrong population size. Expected ", _population_size,
                     ", got ", _session.X.size());
                goto error;
            }
            else if (_session.X[0].size() != _individual_size) {
                GL1R(TAG, "Wrong individual size. Expected ", _individual_size,
                     ", got ", _session.X[0].size());
                goto error;
            }
            else if (_rand_seed != _session.random_seed) {
                GL1Y(TAG, "Wrong random seed. Expected ", _rand_seed,
                     ", got ", _session.random_seed);
                GL1Y("Using seed from session file: ", _session.random_seed);
            }

            _iterations = 0; // Start as 0 to later increment
            _current_fitness = _session.current_fitness;
            _request_session_recovery = true; // Request session recovery
            _rand_seed = _session.random_seed;
            try {
                _algo->extract<pso_gen>()->set_seed(_rand_seed);
                _p0->set_seed(_rand_seed);
            }
            catch (const std::exception &e) {
                GL1Y(TAG, "set_seed not fully supported by current algorithm");
            }

            _session_restored = true;

            GL1Y(TAG, "Recovering session with X Size=",
                 _session.X[0].size(),
                 ", Population Size=",
                 _session.population_size,
                 ", Iterations=", _session.iterations,
                 ", Current Fitness=", _session.current_fitness);
        }
        catch (const std::exception &e) {
            GL1R(TAG, "Exception: ", e.what());
            goto error;
        }

        GL1G(TAG, "Session recovered:");
        GL1G(TAG, _session_dir, SESSION_FILE);

        return true;

    error:
        GL1Y(TAG, "Session files failed to load, starting new session now...");
        return false;
    }

    vector_double GetIndividual()
    {
        return x;
    }

    uint32_t GetIndividualSize()
    {
        return _individual_size;
    }

    uint GetIterations()
    {
        return _iterations;
    }

    vector_double &Iteration(double fitness_value)
    {
        _current_fitness = fitness_value;
        sem_post(&_sem_iter);
        sem_wait(&_iter_done);
        // Save number of iterations
        if (StateMachine.config.fuzzing.save_session_on_exit) {
            // Save session assynchronously
            thread *t = new thread([&]() {
                enable_idle_scheduler();
                SaveSession();
            });
        }

        auto &stop_cond_cfg = StateMachine.config.fuzzing.stop_condition;
        if (stop_cond_cfg.stop_on_max_iterations &&
            (this->_iterations > stop_cond_cfg.max_iterations)) {
            GL1M(TAG, "Max iterations reached.");
            GL1M(TAG, "Max Iterations:", stop_cond_cfg.max_iterations);
            if (this->callback_stop_condition != nullptr) {
                this->callback_stop_condition(FuzzingStopCondition::ITERATIONS);
            }
        }

        return x;
    }

    void SetIterationCallback(function<void(const vector_double &)> fcn)
    {
        callback_iteration = fcn;
    }

    void SetStopConditionCallback(function<void(const FuzzingStopCondition stop_code)> fcn)
    {
        callback_stop_condition = fcn;
    }

    template <class T>
    void SetDefaultCallbacks(T &LogSink, int ExitSignal = SIGUSR1)
    {
        Fuzzing &fuzzing = StateMachine.config.fuzzing;
        SetIterationCallback([&](const vector_double x) {
            StateMachine.ResetStats();
            LogSink.writeLog(format("Iteration: {}", GetIterations()));
            if (fuzzing.enable_optimization)
                return;

            if (fuzzing.enable_mutation)
                GL1Y("Mutation Probability: ", fuzzing.default_mutation_probability);

            if (fuzzing.enable_duplication) {
                GL1Y("Duplication Probability: ", fuzzing.default_duplication_probability);
                GL1Y("Max Duplication Time: ", fuzzing.max_duplication_time_ms);
            }
        });
        SetStopConditionCallback([&](const FuzzingStopCondition stop_code) {
            GL1Y("Fuzzing session stopped");
            kill(getpid(), SIGUSR1);
        });
    }

    void SetDefaultX()
    {
        for (size_t i = 0; i < x.size(); i++) {
            x[i] = StateMachine.config.fuzzing.default_mutation_probability;
        }
    }
};

PAGMO_S11N_PROBLEM_EXPORT_KEY(Fitness::problem_basic);
#endif