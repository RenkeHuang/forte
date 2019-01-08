/*
 * @BEGIN LICENSE
 *
 * Forte: an open-source plugin to Psi4 (https://github.com/psi4/psi4)
 * that implements a variety of quantum chemistry methods for strongly
 * correlated electrons.
 *
 * Copyright (c) 2012-2019 by its authors (see COPYING, COPYING.LESSER, AUTHORS).
 *
 * The copyrights for code used from other parties are included in
 * the corresponding files.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 *
 * @END LICENSE
 */

#include <numeric>
#include <iomanip>

#include "psi4/psi4-dec.h"
#include "psi4/libpsi4util/PsiOutStream.h"
#include "psi4/libmints/molecule.h"
#include "psi4/libmints/pointgrp.h"
#include "psi4/libmints/wavefunction.h"
#include "psi4/libpsi4util/process.h"

#include "base_classes/forte_options.h"
#include "helpers/helpers.h"
#include "integrals/active_space_integrals.h"
#include "active_space_solver.h"

#include "ms_active_space_solver.h"

namespace forte {

MSActiveSpaceSolver::MSActiveSpaceSolver(
    const std::string& type,
    std::vector<std::pair<StateInfo, std::vector<double>>>& state_weights_list,
    std::shared_ptr<SCFInfo> scf_info, std::shared_ptr<MOSpaceInfo> mo_space_info,
    std::shared_ptr<ActiveSpaceIntegrals> as_ints, std::shared_ptr<ForteOptions> options)
    : type_(type), state_weights_list_(state_weights_list), scf_info_(scf_info),
      mo_space_info_(mo_space_info), as_ints_(as_ints), options_(options) {
    print_options();
}

double MSActiveSpaceSolver::compute_energy() {
    double energy = 0.0;
    for (const auto& [state, weights] : state_weights_list_) {
        // compute the energy of state and save it
        size_t nroot = weights.size();
        std::shared_ptr<ActiveSpaceSolver> solver = make_active_space_solver2(
            type_, state, nroot, scf_info_, mo_space_info_, as_ints_, options_);
        // TODO: need to pass information on how many states are computed
        solver->compute_energy();
        auto evals = solver->evals();
        for (size_t r = 0; r < nroot; r++) {
            energy += evals->get(r) * weights[r];
        }
        solvers_.push_back(solver);
    }
    return energy + as_ints_->ints()->nuclear_repulsion_energy();
}

void MSActiveSpaceSolver::print_options() {
    print_h2("Summary of Active Space Solver Input");

    //    std::vector<std::pair<std::string, size_t>> info;
    //    info.push_back({"No. a electrons in active", nalfa_ - ncore_ - nfrzc_});
    //    info.push_back({"No. b electrons in active", nbeta_ - ncore_ - nfrzc_});
    //    info.push_back({"multiplicity", multi_});
    //    info.push_back({"spin ms (2 * Sz)", twice_ms_});

    //    for (auto& str_dim : info) {
    //        outfile->Printf("\n    %-30s = %5zu", str_dim.first.c_str(), str_dim.second);
    //    }

    //    print_h2("Orbital Spaces");
    //    auto print_irrep = [&](const string& str, const psi::Dimension& array) {
    //        outfile->Printf("\n    %-30s", str.c_str());
    //        outfile->Printf("[");
    //        for (int h = 0; h < nirrep_; ++h) {
    //            outfile->Printf(" %4d ", array[h]);
    //        }
    //        outfile->Printf("]");
    //    };
    //    print_irrep("TOTAL MO", nmopi_);
    //    print_irrep("FROZEN CORE", frzc_dim_);
    //    print_irrep("FROZEN VIRTUAL", frzv_dim_);
    //    print_irrep("CORRELATED MO", ncmopi_);
    //    print_irrep("CORE", core_dim_);
    //    print_irrep("ACTIVE", actv_dim_);
    //    print_irrep("VIRTUAL", virt_dim_);

    print_h2("State Averaging Summary");

    psi::CharacterTable ct = psi::Process::environment.molecule()->point_group()->char_table();

    std::vector<std::string> irrep_symbol = psi::Process::environment.molecule()->irrep_labels();

    int nroots_max = 0;
    int nstates = 0;
    for (const auto& [state, weights] : state_weights_list_) {
        int nroots = weights.size();
        nstates += nroots;
        nroots_max = std::max(nroots_max, nroots);
    }

    if (nroots_max == 1) {
        nroots_max = 7;
    } else {
        nroots_max *= 6;
        nroots_max -= 1;
    }

    int ltotal = 6 + 2 + 6 + 2 + 7 + 2 + nroots_max;
    std::string blank(nroots_max - 7, ' ');
    std::string dash(ltotal, '-');
    psi::outfile->Printf("\n    Irrep.  Multi.  Nstates  %sWeights", blank.c_str());
    psi::outfile->Printf("\n    %s", dash.c_str());
    for (const auto& [state, weights] : state_weights_list_) {
        //            int irrep, multi, nroots;
        //            std::vector<double> weights;
        //            std::tie(irrep, multi, nroots, weights) = sa_info_[i];
        int irrep = state.irrep();
        int multiplicity = state.multiplicity();
        int nroots = weights.size();
        std::string w_str;
        for (const double& w : weights) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(3) << w;
            w_str += ss.str() + " ";
        }
        w_str.pop_back(); // delete the last space character

        std::stringstream ss;
        ss << std::setw(4) << std::right << irrep_symbol[irrep] << "    " << std::setw(4)
           << std::right << multiplicity << "    " << std::setw(5) << std::right << nroots << "    "
           << std::setw(nroots_max) << w_str;
        psi::outfile->Printf("\n    %s", ss.str().c_str());
    }
    psi::outfile->Printf("\n    %s", dash.c_str());
    psi::outfile->Printf("\n    Total number of states: %d", nstates);
    psi::outfile->Printf("\n    %s\n", dash.c_str());
}

std::vector<std::pair<StateInfo, std::vector<double>>>
make_state_weights_list(std::shared_ptr<ForteOptions> options,
                        std::shared_ptr<psi::Wavefunction> wfn) {
    std::vector<std::pair<StateInfo, std::vector<double>>> state_weight_list;
    auto state = make_state_info_from_psi_wfn(wfn);
    if ((options->psi_options())["AVG_STATE"].size() == 0) {

        int nroot = options->get_int("NROOT");
        int root = options->get_int("ROOT");

        std::vector<double> weights(nroot, 0.0);
        weights[root] = 1.0;
        state_weight_list.push_back(std::make_pair(state, weights));
    } else {
        double sum_of_weights = 0.0;
        size_t nstates = 0;
        size_t nentry = (options->psi_options())["AVG_STATE"].size();
        for (size_t i = 0; i < nentry; ++i) {
            if ((options->psi_options())["AVG_STATE"][i].size() != 3) {
                psi::outfile->Printf("\n  Error: invalid input of AVG_STATE. Each "
                                     "entry should take an array of three numbers.");
                throw psi::PSIEXCEPTION("Invalid input of AVG_STATE");
            }

            // read data
            // irreducible representation
            int irrep = (options->psi_options())["AVG_STATE"][i][0].to_integer();
            // multiplicity (2S + 1)
            int multi = (options->psi_options())["AVG_STATE"][i][1].to_integer();
            // number of states with this irrep and multiplicity
            int nstates_this = (options->psi_options())["AVG_STATE"][i][2].to_integer();

            // check for errors
            int nirrep = wfn->nirrep();
            if (irrep >= nirrep || irrep < 0) {
                psi::outfile->Printf("\n  Error: invalid irrep in AVG_STATE. Please "
                                     "check the input irrep (start from 0) not to "
                                     "exceed %d",
                                     nirrep - 1);
                throw psi::PSIEXCEPTION("Invalid irrep in AVG_STATE");
            }
            if (multi < 1) {
                psi::outfile->Printf("\n  Error: invalid multiplicity in AVG_STATE.");
                throw psi::PSIEXCEPTION("Invaid multiplicity in AVG_STATE");
            }

            if (nstates_this < 1) {
                psi::outfile->Printf("\n  Error: invalid nstates in AVG_STATE. "
                                     "nstates of a certain irrep and multiplicity "
                                     "should greater than 0.");
                throw psi::PSIEXCEPTION("Invalid nstates in AVG_STATE.");
            }

            std::vector<double> weights;
            if ((options->psi_options())["AVG_WEIGHT"].has_changed()) {
                if ((options->psi_options())["AVG_WEIGHT"].size() != nentry) {
                    psi::outfile->Printf("\n  Error: mismatched number of entries in "
                                         "AVG_STATE (%d) and AVG_WEIGHT (%d).",
                                         nentry, (options->psi_options())["AVG_WEIGHT"].size());
                    throw psi::PSIEXCEPTION("Mismatched number of entries in AVG_STATE "
                                            "and AVG_WEIGHT.");
                }
                int nweights = (options->psi_options())["AVG_WEIGHT"][i].size();
                if (nweights != nstates_this) {
                    psi::outfile->Printf("\n  Error: mismatched number of weights "
                                         "in entry %d of AVG_WEIGHT. Asked for %d "
                                         "states but only %d weights.",
                                         i, nstates_this, nweights);
                    throw psi::PSIEXCEPTION("Mismatched number of weights in AVG_WEIGHT.");
                }
                for (int n = 0; n < nstates_this; ++n) {
                    double w = (options->psi_options())["AVG_WEIGHT"][i][n].to_double();
                    if (w < 0.0) {
                        psi::outfile->Printf("\n  Error: negative weights in AVG_WEIGHT.");
                        throw psi::PSIEXCEPTION("Negative weights in AVG_WEIGHT.");
                    }
                    weights.push_back(w);
                }
            } else {
                // use equal weights
                weights = std::vector<double>(nstates, 1.0);
            }
            sum_of_weights = std::accumulate(std::begin(weights), std::end(weights), 0.0);
            state_weight_list.push_back(std::make_pair(state, weights));
            nstates += nstates_this;
        }

        // normalize weights
        for (auto& [state, weights] : state_weight_list) {
            for (auto& w : weights) {
                w /= sum_of_weights;
            }
        }
    }
    return state_weight_list;
}

} // namespace forte
