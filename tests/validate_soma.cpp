#include <fstream>

#include "gtest.h"
#include "util.hpp"

#include <cell.hpp>
#include <fvm.hpp>

#include <json/src/json.hpp>

// compares results with those generated by nrn/soma.py
// single compartment model with HH channels
TEST(soma, neuron_baseline)
{
    using namespace nest::mc;
    using namespace nlohmann;

    nest::mc::cell cell;

    // setup global state for the mechanisms
    nest::mc::mechanisms::setup_mechanism_helpers();

    // Soma with diameter 18.8um and HH channel
    auto soma = cell.add_soma(18.8/2.0);
    soma->mechanism("membrane").set("r_L", 123); // no effect for single compartment cell
    soma->add_mechanism(hh_parameters());

    // add stimulus to the soma
    cell.add_stimulus({0,0.5}, {10., 100., 0.1});

    // make the lowered finite volume cell
    fvm::fvm_cell<double, int> model(cell);

    // load data from file
    auto cell_data = testing::load_spike_data("../nrn/soma.json");
    EXPECT_TRUE(cell_data.size()>0);
    if(cell_data.size()==0) return;

    json& nrn =
        *std::min_element(
            cell_data.begin(), cell_data.end(),
            [](json& lhs, json& rhs) {return lhs["dt"]<rhs["dt"];}
        );
    std::vector<double> nrn_spike_times = nrn["spikes"];

    std::cout << "baseline with time step size " << nrn["dt"] << " ms\n";
    std::cout << "baseline spikes : " << nrn["spikes"] << "\n";

    for(auto& run : cell_data) {
        std::vector<double> v;
        double dt = run["dt"];

        // set initial conditions
        using memory::all;
        model.voltage()(all) = -65.;
        model.initialize(); // have to do this _after_ initial conditions are set

        // run the simulation
        auto tfinal =   120.; // ms
        int nt = tfinal/dt;
        v.push_back(model.voltage()[0]);
        for(auto i=0; i<nt; ++i) {
            model.advance(dt);
            // save voltage at soma
            v.push_back(model.voltage()[0]);
        }

        // get the spike times from the NEST MC simulation
        auto nst_spike_times = testing::find_spikes(v, 0., dt);
        // compare NEST MC and baseline NEURON results
        auto comparison = testing::compare_spikes(nst_spike_times, nrn_spike_times);

        // Assert that relative error is less than 1%.
        // For a 100 ms simulation this asserts that the difference between
        // NEST and the most accurate NEURON simulation is less than 1 ms.
        std::cout << "MAX ERROR @ " << dt << " is " << comparison.max_relative_error()*100. << "\n";
        EXPECT_TRUE(comparison.max_relative_error()*100. < 1.);
    }
}

// check if solution converges
TEST(soma, convergence)
{
    using namespace nest::mc;

    nest::mc::cell cell;

    // setup global state for the mechanisms
    nest::mc::mechanisms::setup_mechanism_helpers();

    // Soma with diameter 18.8um and HH channel
    auto soma = cell.add_soma(18.8/2.0);
    soma->mechanism("membrane").set("r_L", 123); // no effect for single compartment cell
    soma->add_mechanism(hh_parameters());

    // add stimulus to the soma
    cell.add_stimulus({0,0.5}, {10., 100., 0.1});

    // make the lowered finite volume cell
    fvm::fvm_cell<double, int> model(cell);

    // generate baseline solution with small dt=0.0001
    std::vector<double> baseline_spike_times;
    {
        auto dt = 1e-4;
        std::vector<double> v;

        // set initial conditions
        using memory::all;
        model.voltage()(all) = -65.;
        model.initialize(); // have to do this _after_ initial conditions are set

        // run the simulation
        auto tfinal =   120.; // ms
        int nt = tfinal/dt;
        v.push_back(model.voltage()[0]);
        for(auto i=0; i<nt; ++i) {
            model.advance(dt);
            // save voltage at soma
            v.push_back(model.voltage()[0]);
        }

        baseline_spike_times = testing::find_spikes(v, 0., dt);
    }

    testing::spike_comparison previous;
    for(auto dt : {0.05, 0.02, 0.01, 0.005, 0.001}) {
        std::vector<double> v;

        // set initial conditions
        using memory::all;
        model.voltage()(all) = -65.;
        model.initialize(); // have to do this _after_ initial conditions are set

        // run the simulation
        auto tfinal =   120.; // ms
        int nt = tfinal/dt;
        v.push_back(model.voltage()[0]);
        for(auto i=0; i<nt; ++i) {
            model.advance(dt);
            // save voltage at soma
            v.push_back(model.voltage()[0]);
        }

        // get the spike times from the NEST MC simulation
        auto nst_spike_times = testing::find_spikes(v, 0., dt);

        // compare NEST MC and baseline NEURON results
        auto comparison = testing::compare_spikes(nst_spike_times, baseline_spike_times);
        std::cout << "dt " << std::setw(8) << dt << " : " << comparison << "\n";
        if(!previous.is_valid()) {
            previous = std::move(comparison);
        }
        else {
            // Assert that relative error is less than 1%.
            // For a 100 ms simulation this asserts that the difference between
            // NEST and the most accurate NEURON simulation is less than 1 ms.
            EXPECT_TRUE(comparison.max_relative_error() < previous.max_relative_error());
            EXPECT_TRUE(comparison.rms < previous.rms);
            EXPECT_TRUE(comparison.max < previous.max);
        }
    }
}
