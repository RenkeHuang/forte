/*
 * @BEGIN LICENSE
 *
 * Forte: an open-source plugin to Psi4 (https://github.com/psi4/psi4)
 * that implements a variety of quantum chemistry methods for strongly
 * correlated electrons.
 *
 * Copyright (c) 2012-2017 by its authors (see COPYING, COPYING.LESSER, AUTHORS).
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

#include <algorithm>

#include "psi4/psi4-dec.h"
#include "psi4/libpsi4util/PsiOutStream.h"

#include "stl_bitset_determinant.h"

using namespace psi;

#define ALFA(n) bits_[n]
#define BETA(n) bits_[num_str_bits + n]

namespace psi {
namespace forte {

const STLBitsetDeterminant::bit_t STLBitsetDeterminant::alfa_mask =
    bit_t(0xFFFFFFFFFFFFFFFF) |
    (bit_t(0xFFFFFFFFFFFFFFFF) << STLBitsetDeterminant::num_str_bits / 2);
const STLBitsetDeterminant::bit_t STLBitsetDeterminant::beta_mask =
    alfa_mask << STLBitsetDeterminant::num_str_bits;

STLBitsetDeterminant::STLBitsetDeterminant(int nmo) : size_(nmo) {
    //    set_count_bits(nmo);
    //    if (nmo == 0) {
    //        bits_.flip();
    //    }
}

STLBitsetDeterminant::STLBitsetDeterminant(const std::vector<bool>& occupation) {
    int nmo = occupation.size() / 2;
    for (int p = 0; p < nmo; ++p)
        ALFA(p) = occupation[p];
    for (int p = 0; p < nmo; ++p)
        BETA(p) = occupation[nmo + p];
    set_count_bits(nmo);
}

STLBitsetDeterminant::STLBitsetDeterminant(const std::vector<bool>& occupation_a,
                                           const std::vector<bool>& occupation_b) {
    int nmo = occupation_a.size();
    for (int p = 0; p < nmo; ++p) {
        ALFA(p) = occupation_a[p];
        BETA(p) = occupation_b[p];
    }
    set_count_bits(nmo);
}

STLBitsetDeterminant::STLBitsetDeterminant(const bit_t& bits, int nmo) {
    bits_ = bits;
    set_count_bits(nmo);
}

const STLBitsetDeterminant::bit_t& STLBitsetDeterminant::bits() const { return bits_; }

void STLBitsetDeterminant::copy(const STLBitsetDeterminant& rhs) {
    bits_ = rhs.bits_;
    set_count_bits(rhs.find_nmo());
}

bool STLBitsetDeterminant::less_than(const STLBitsetDeterminant& rhs,
                                     const STLBitsetDeterminant& lhs) {
    // check beta first
    for (int i = num_str_bits + rhs.size_ - 1; i >= num_str_bits; i--) {
        if (rhs.bits_[i] ^ lhs.bits_[i])
            return lhs.bits_[i];
    }
    for (int i = rhs.size_ - 1; i >= 0; i--) {
        if (rhs.bits_[i] ^ lhs.bits_[i])
            return lhs.bits_[i];
    }
    //  return false;
    //    // check beta first
    //    for (int p = num_det_bits - 1; p >= 0; --p) {
    //        if ((rhs.bits_[p] == false) and (lhs.bits_[p] == true))
    //            return true;
    //        if ((rhs.bits_[p] == true) and (lhs.bits_[p] == false))
    //            return false;
    //    }
    return false;
}

bool STLBitsetDeterminant::reverse_less_then(const STLBitsetDeterminant& rhs,
                                             const STLBitsetDeterminant& lhs) {
    // check alpha first
    for (int i = rhs.size_ - 1; i >= 0; i--) {
        if (rhs.bits_[i] ^ lhs.bits_[i])
            return lhs.bits_[i];
    }
    for (int i = num_str_bits + rhs.size_ - 1; i >= num_str_bits; i--) {
        if (rhs.bits_[i] ^ lhs.bits_[i])
            return lhs.bits_[i];
    }
    //    for (int p = num_str_bits - 1; p >= 0; --p) {
    //        if ((i.get_alfa_bit(p) == false) and (j.get_alfa_bit(p) == true))
    //            return true;
    //        if ((i.get_alfa_bit(p) == true) and (j.get_alfa_bit(p) == false))
    //            return false;
    //    }
    //    for (int p = num_str_bits - 1; p >= 0; --p) {
    //        if ((i.get_beta_bit(p) == false) and (j.get_beta_bit(p) == true))
    //            return true;
    //        if ((i.get_beta_bit(p) == true) and (j.get_beta_bit(p) == false))
    //            return false;
    //    }
    return false;
}

bool STLBitsetDeterminant::operator==(const STLBitsetDeterminant& lhs) const {
    return (bits_ == lhs.bits_);
}

bool STLBitsetDeterminant::operator<(const STLBitsetDeterminant& lhs) const {
    // check beta first
    // check beta first
    for (int i = num_str_bits + size_ - 1; i >= num_str_bits; i--) {
        if (bits_[i] ^ lhs.bits_[i])
            return lhs.bits_[i];
    }
    for (int i = size_ - 1; i >= 0; i--) {
        if (bits_[i] ^ lhs.bits_[i])
            return lhs.bits_[i];
    }
    //    for (int p = num_det_bits - 1; p >= 0; --p) {
    //        if ((bits_[p] == false) and (lhs.bits_[p] == true))
    //            return true;
    //        if ((bits_[p] == true) and (lhs.bits_[p] == false))
    //            return false;
    //    }
    return false;
}

STLBitsetDeterminant STLBitsetDeterminant::operator^(const STLBitsetDeterminant& lhs) const {
    int nmo = lhs.find_nmo();
    return STLBitsetDeterminant(bits_ ^ lhs.bits_, nmo);
}

STLBitsetDeterminant& STLBitsetDeterminant::operator^=(const STLBitsetDeterminant& lhs) {
    int nmo = lhs.find_nmo();
    bits_ ^= lhs.bits_;
    set_count_bits(nmo);
    return *this;
}

STLBitsetDeterminant& STLBitsetDeterminant::operator&=(const STLBitsetDeterminant& lhs) {
    bits_ &= lhs.bits_;
    set_count_bits(lhs.find_nmo());
    return *this;
}

STLBitsetDeterminant& STLBitsetDeterminant::operator|=(const STLBitsetDeterminant& lhs) {
    bits_ |= lhs.bits_;
    set_count_bits(lhs.find_nmo());
    return *this;
}

STLBitsetDeterminant& STLBitsetDeterminant::flip() {
    int nmo = find_nmo();
    bits_.flip();
    set_count_bits(nmo);
    return *this;
}

int STLBitsetDeterminant::count_alfa() const {
    //  int count = 0;
    //  for (int i = 0; i < size_; ++i) {
    //      count += ALFA(i);
    //  }
    //  return count;
    return (bits_ & alfa_mask).count();
}

int STLBitsetDeterminant::count_beta() const {
    //    int count = 0;
    //    for (int i = 0; i < size_; ++i) {
    //        count += BETA(i);
    //    }
    //    return count;
    return (bits_ & beta_mask).count();
}

bool STLBitsetDeterminant::get_alfa_bit(int n) const { return ALFA(n); }

bool STLBitsetDeterminant::get_beta_bit(int n) const { return BETA(n); }

void STLBitsetDeterminant::set_alfa_bit(int n, bool value) { ALFA(n) = value; }

void STLBitsetDeterminant::set_beta_bit(int n, bool value) { BETA(n) = value; }

void STLBitsetDeterminant::set_count_bits(int nmo) {
    size_ = nmo;
    //    set_alfa_bit(nmo, false);
    //    set_beta_bit(nmo, false);
    //    for (int b = nmo + 1; b < num_str_bits; ++b) {
    //        set_alfa_bit(b, true);
    //        set_beta_bit(b, true);
    //    }
}

int STLBitsetDeterminant::find_nmo() const {
    return size_;
    //    for (int p = num_str_bits - 1; p >= 0; --p) {
    //        if (not ALFA(p))
    //            return p;
    //    }
    //    outfile->Printf("\n\n Using an uninitialized determinant");
    //    exit(1);
    //    return 0;
}

std::vector<int> STLBitsetDeterminant::get_alfa_occ() {
    int nmo = find_nmo();
    std::vector<int> occ;
    for (int p = 0; p < nmo; ++p) {
        if (ALFA(p))
            occ.push_back(p);
    }
    return occ;
}

std::vector<int> STLBitsetDeterminant::get_beta_occ() {
    int nmo = find_nmo();
    std::vector<int> occ;
    for (int p = 0; p < nmo; ++p) {
        if (BETA(p))
            occ.push_back(p);
    }
    return occ;
}

std::vector<int> STLBitsetDeterminant::get_alfa_vir() {
    int nmo = find_nmo();
    std::vector<int> vir;
    for (int p = 0; p < nmo; ++p) {
        if (not ALFA(p))
            vir.push_back(p);
    }
    return vir;
}

std::vector<int> STLBitsetDeterminant::get_beta_vir() {
    int nmo = find_nmo();
    std::vector<int> vir;
    for (int p = 0; p < nmo; ++p) {
        if (not BETA(p))
            vir.push_back(p);
    }
    return vir;
}

std::vector<int> STLBitsetDeterminant::get_alfa_occ() const {
    int nmo = find_nmo();
    std::vector<int> occ;
    for (int p = 0; p < nmo; ++p) {
        if (ALFA(p))
            occ.push_back(p);
    }
    return occ;
}

std::vector<int> STLBitsetDeterminant::get_beta_occ() const {
    int nmo = find_nmo();
    std::vector<int> occ;
    for (int p = 0; p < nmo; ++p) {
        if (BETA(p))
            occ.push_back(p);
    }
    return occ;
}

std::vector<int> STLBitsetDeterminant::get_alfa_vir() const {
    int nmo = find_nmo();
    std::vector<int> vir;
    for (int p = 0; p < nmo; ++p) {
        if (not ALFA(p))
            vir.push_back(p);
    }
    return vir;
}

std::vector<int> STLBitsetDeterminant::get_beta_vir() const {
    int nmo = find_nmo();
    std::vector<int> vir;
    for (int p = 0; p < nmo; ++p) {
        if (not BETA(p))
            vir.push_back(p);
    }
    return vir;
}

double STLBitsetDeterminant::create_alfa_bit(int n) {
    if (ALFA(n))
        return 0.0;
    ALFA(n) = true;
    return slater_sign_a(n);
}

double STLBitsetDeterminant::create_beta_bit(int n) {
    if (BETA(n))
        return 0.0;
    BETA(n) = true;
    return slater_sign_b(n);
}

double STLBitsetDeterminant::destroy_alfa_bit(int n) {
    if (not ALFA(n))
        return 0.0;
    ALFA(n) = false;
    return slater_sign_a(n);
}

/// Set the value of a beta bit
double STLBitsetDeterminant::destroy_beta_bit(int n) {
    if (not BETA(n))
        return 0.0;
    BETA(n) = false;
    return slater_sign_b(n);
}

/// Switch alfa and beta bits
void STLBitsetDeterminant::spin_flip() {
    bool temp;
    int nmo = find_nmo();
    for (int p = 0; p < nmo; ++p) {
        temp = ALFA(p);
        ALFA(p) = BETA(p);
        BETA(p) = temp;
    }
}

/// Return determinant with one spin annihilated, 0 == alpha
void STLBitsetDeterminant::zero_spin(STLBitsetDeterminant::SpinType spin_type) {
    int nmo = find_nmo();
    if (spin_type == SpinType::AlphaSpin) {
        for (int p = 0; p < nmo; ++p) {
            ALFA(p) = false;
        }
    } else {
        for (int p = 0; p < nmo; ++p) {
            BETA(p) = false;
        }
    }
}

void STLBitsetDeterminant::print() const {
    int nmo = find_nmo();
    outfile->Printf("\n  |");
    for (int p = 0; p < nmo; ++p) {
        if (ALFA(p) and BETA(p)) {
            outfile->Printf("%d", 2);
        } else if (ALFA(p) and not BETA(p)) {
            outfile->Printf("+");
        } else if (not ALFA(p) and BETA(p)) {
            outfile->Printf("-");
        } else {
            outfile->Printf("0");
        }
    }
    outfile->Printf(">");
}

std::string STLBitsetDeterminant::str() const {
    int nmo = find_nmo();
    std::string s;
    s += "|";
    for (int p = 0; p < nmo; ++p) {
        if (ALFA(p) and BETA(p)) {
            s += "2";
        } else if (ALFA(p) and not BETA(p)) {
            s += "+";
        } else if (not ALFA(p) and BETA(p)) {
            s += "-";
        } else {
            s += "0";
        }
    }
    s += ">";
    return s;
}

std::string STLBitsetDeterminant::str2() const {
    int nmo = find_nmo();
    std::string s;
    s += "|";
    for (int p = 0; p < nmo; ++p) {
        s += ALFA(p) ? "1" : "0";
    }
    s += "|";
    for (int p = 0; p < nmo; ++p) {
        s += BETA(p) ? "1" : "0";
    }
    s += ">";
    return s;
}

double STLBitsetDeterminant::slater_sign_a(int n) const {
    double sign = 1.0;
    for (int i = 0; i < n; ++i) { // This runs up to the operator before n
        if (ALFA(i))
            sign *= -1.0;
    }
    return (sign);
}

double STLBitsetDeterminant::slater_sign_aa(int n, int m) const {
    double sign = 1.0;
    for (int i = m + 1; i < n; ++i) { // This runs up to the operator before n
        if (ALFA(i))
            sign *= -1.0;
    }
    for (int i = n + 1; i < m; ++i) {
        if (ALFA(i)) {
            sign *= -1.0;
        }
    }
    return (sign);
}

double STLBitsetDeterminant::slater_sign_b(int n) const {
    double sign = 1.0;
    for (int i = 0; i < n; ++i) { // This runs up to the operator before n
        if (BETA(i))
            sign *= -1.0;
    }
    return (sign);
}

double STLBitsetDeterminant::slater_sign_bb(int n, int m) const {
    double sign = 1.0;
    for (int i = m + 1; i < n; ++i) { // This runs up to the operator before n
        if (BETA(i))
            sign *= -1.0;
    }
    for (int i = n + 1; i < m; ++i) {
        if (BETA(i)) {
            sign *= -1.0;
        }
    }
    return (sign);
}

double STLBitsetDeterminant::slater_sign_aaaa(int i, int j, int a, int b) const {
    if ((((i < a) && (j < a) && (i < b) && (j < b)) == true) ||
        (((i < a) || (j < a) || (i < b) || (j < b)) == false)) {
        if ((i < j) ^ (a < b)) {
            return (-1.0 * slater_sign_aa(i, j) * slater_sign_aa(a, b));
        } else {
            return (slater_sign_aa(i, j) * slater_sign_aa(a, b));
        }
    } else {
        if ((i < j) ^ (a < b)) {
            return (-1.0 * slater_sign_aa(i, b) * slater_sign_aa(j, a));
        } else {
            return (slater_sign_aa(i, a) * slater_sign_aa(j, b));
        }
    }
}

double STLBitsetDeterminant::slater_sign_bbbb(int i, int j, int a, int b) const {
    if ((((i < a) && (j < a) && (i < b) && (j < b)) == true) ||
        (((i < a) || (j < a) || (i < b) || (j < b)) == false)) {
        if ((i < j) ^ (a < b)) {
            return (-1.0 * slater_sign_bb(i, j) * slater_sign_bb(a, b));
        } else {
            return (slater_sign_bb(i, j) * slater_sign_bb(a, b));
        }
    } else {
        if ((i < j) ^ (a < b)) {
            return (-1.0 * slater_sign_bb(i, b) * slater_sign_bb(j, a));
        } else {
            return (slater_sign_bb(i, a) * slater_sign_bb(j, b));
        }
    }
}

double STLBitsetDeterminant::single_excitation_a(int i, int a) {
    ALFA(i) = false;
    ALFA(a) = true;
    return slater_sign_aa(i, a);
}

double STLBitsetDeterminant::single_excitation_b(int i, int a) {
    BETA(i) = false;
    BETA(a) = true;
    return slater_sign_bb(i, a);
}

double STLBitsetDeterminant::double_excitation_aa(int i, int j, int a, int b) {
    ALFA(i) = false;
    ALFA(j) = false;
    ALFA(b) = true;
    ALFA(a) = true;
    return slater_sign_aaaa(i, j, a, b);
}

double STLBitsetDeterminant::double_excitation_ab(int i, int j, int a, int b) {
    ALFA(i) = false;
    BETA(j) = false;
    BETA(b) = true;
    ALFA(a) = true;
    return slater_sign_aa(i, a) * slater_sign_bb(j, b);
}

double STLBitsetDeterminant::double_excitation_bb(int i, int j, int a, int b) {
    BETA(i) = false;
    BETA(j) = false;
    BETA(b) = true;
    BETA(a) = true;
    return slater_sign_bbbb(i, j, a, b);
}

std::vector<std::pair<STLBitsetDeterminant, double>> STLBitsetDeterminant::spin_plus() const {
    int nmo = find_nmo();

    std::vector<std::pair<STLBitsetDeterminant, double>> res;
    for (int i = 0; i < nmo; ++i) {
        if ((not ALFA(i)) and BETA(i)) {
            double sign = slater_sign_a(i) * slater_sign_b(i);
            STLBitsetDeterminant new_det(*this);
            new_det.set_alfa_bit(i, true);
            new_det.set_beta_bit(i, false);
            res.push_back(std::make_pair(new_det, sign));
        }
    }
    return res;
}

std::vector<std::pair<STLBitsetDeterminant, double>> STLBitsetDeterminant::spin_minus() const {
    int nmo = find_nmo();
    std::vector<std::pair<STLBitsetDeterminant, double>> res;
    for (int i = 0; i < nmo; ++i) {
        if (ALFA(i) and (not BETA(i))) {
            double sign = slater_sign_a(i) * slater_sign_b(i);
            STLBitsetDeterminant new_det(*this);
            new_det.set_alfa_bit(i, false);
            new_det.set_beta_bit(i, true);
            res.push_back(std::make_pair(new_det, sign));
        }
    }
    return res;
}

double STLBitsetDeterminant::spin_z() const {
    int nmo = find_nmo();

    int n = 0;
    for (int i = 0; i < nmo; ++i) {
        if (ALFA(i))
            n++;
        if (BETA(i))
            n--;
    }
    return 0.5 * static_cast<double>(n);
}

int STLBitsetDeterminant::npair() {
    int nmo = find_nmo();
    int npair = 0;
    for (int n = 0; n < nmo; ++n) {
        if (ALFA(n) and BETA(n)) {
            npair++;
        }
    }
    return npair;
}

double STLBitsetDeterminant::spin2(const STLBitsetDeterminant& rhs) const {
    int nmo = find_nmo();
    const bit_t& I = bits_;
    const bit_t& J = rhs.bits_;

    // Compute the matrix elements of the operator S^2
    // S^2 = S- S+ + Sz (Sz + 1)
    //     = Sz (Sz + 1) + Nbeta + Npairs - sum_pq' a+(qa) a+(pb) a-(qb) a-(pa)
    double matrix_element = 0.0;

    int nadiff = 0;
    int nbdiff = 0;
    int na = 0;
    int nb = 0;
    int npair = 0;
    // Count how many differences in mos are there and the number of alpha/beta
    // electrons
    for (int n = 0; n < nmo; ++n) {
        if (I[n] != J[n])
            nadiff++;
        if (I[num_str_bits + n] != J[num_str_bits + n])
            nbdiff++;
        if (I[n])
            na++;
        if (I[num_str_bits + n])
            nb++;
        if ((I[n] and I[num_str_bits + n]))
            npair += 1;
    }
    nadiff /= 2;
    nbdiff /= 2;

    double Ms = 0.5 * static_cast<double>(na - nb);

    // PhiI = PhiJ -> S^2 = Sz (Sz + 1) + Nbeta - Npairs
    if ((nadiff == 0) and (nbdiff == 0)) {
        matrix_element += Ms * (Ms + 1.0) + double(nb) - double(npair);
    }

    // PhiI = a+(qa) a+(pb) a-(qb) a-(pa) PhiJ
    if ((nadiff == 1) and (nbdiff == 1)) {
        // Find a pair of spin coupled electrons
        int i = -1;
        int j = -1;
        // The logic here is a bit complex
        for (int p = 0; p < nmo; ++p) {
            if (J[p] and I[num_str_bits + p] and (not J[num_str_bits + p]) and (not I[p]))
                i = p; //(p)
            if (J[num_str_bits + p] and I[p] and (not J[p]) and (not I[num_str_bits + p]))
                j = p; //(q)
        }
        if (i != j and i >= 0 and j >= 0) {
            // double sign = SlaterSign(J, i) * SlaterSign(J, nmo_ + j) * SlaterSign(I, nmo_ + i) *
            //              SlaterSign(I, j);
            double sign =
                rhs.slater_sign_a(i) * rhs.slater_sign_b(j) * slater_sign_a(j) * slater_sign_b(i);
            matrix_element -= sign;
        }
    }
    return (matrix_element);
}

void STLBitsetDeterminant::enforce_spin_completeness(std::vector<STLBitsetDeterminant>& det_space,
                                                     int nmo) {
    det_hash<bool> det_map;

    // Add all determinants to the map, assume set is mostly spin complete
    for (auto& I : det_space) {
        det_map[I] = true;
    }
    // Loop over determinants
    size_t ndet_added = 0;
    std::vector<size_t> closed(nmo, 0);
    std::vector<size_t> open(nmo, 0);
    std::vector<size_t> open_bits(nmo, 0);
    for (size_t I = 0, det_size = det_space.size(); I < det_size; ++I) {
        const STLBitsetDeterminant& det = det_space[I];
        //        outfile->Printf("\n  Original determinant: %s",
        //        det.str().c_str());
        for (int i = 0; i < nmo; ++i) {
            closed[i] = open[i] = 0;
            open_bits[i] = false;
        }
        int naopen = 0;
        int nbopen = 0;
        int nclosed = 0;
        for (int i = 0; i < nmo; ++i) {
            if (det.get_alfa_bit(i) and (not det.get_beta_bit(i))) {
                open[naopen + nbopen] = i;
                naopen += 1;
            } else if ((not det.get_alfa_bit(i)) and det.get_beta_bit(i)) {
                open[naopen + nbopen] = i;
                nbopen += 1;
            } else if (det.get_alfa_bit(i) and det.get_beta_bit(i)) {
                closed[nclosed] = i;
                nclosed += 1;
            }
        }

        if (naopen + nbopen == 0)
            continue;

        // Generate the strings 1111100000
        //                      {nao}{nbo}
        for (int i = 0; i < nbopen; ++i)
            open_bits[i] = false; // 0
        for (int i = nbopen; i < naopen + nbopen; ++i)
            open_bits[i] = true; // 1
        do {
            STLBitsetDeterminant new_det(nmo);
            for (int c = 0; c < nclosed; ++c) {
                new_det.set_alfa_bit(closed[c], true);
                new_det.set_beta_bit(closed[c], true);
            }
            for (int o = 0; o < naopen + nbopen; ++o) {
                if (open_bits[o]) { //? not
                    new_det.set_alfa_bit(open[o], true);
                } else {
                    new_det.set_beta_bit(open[o], true);
                }
            }
            if (det_map.count(new_det) == 0) {
                det_space.push_back(new_det);
                det_map[new_det] = true;
                //                outfile->Printf("\n  added determinant:
                //                %s", new_det.str().c_str());
                ndet_added++;
            }
        } while (std::next_permutation(open_bits.begin(), open_bits.begin() + naopen + nbopen));
    }
    // if( ndet_added > 0 ){
    //    outfile->Printf("\n\n  Determinant space is spin incomplete!");
    //    outfile->Printf("\n  %zu more determinants were needed.", ndet_added);
    //}else{
    //    outfile->Printf("\n\n  Determinant space is spin complete.");
    //}
}
}
} // end namespace