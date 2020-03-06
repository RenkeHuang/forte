#include <numeric>

#include "psi4/libpsi4util/process.h"
#include "psi4/libmints/molecule.h"
#include "psi4/libmints/dipole.h"

#include "helpers/printing.h"
#include "helpers/timer.h"

#include "sadsrg.h"

using namespace psi;

namespace forte {

void SADSRG::H1_T1_C0(BlockedTensor& H1, BlockedTensor& T1, const double& alpha, double& C0) {
    local_timer timer;

    double E = 0.0;
    E += 2.0 * H1["am"] * T1["ma"];

    auto temp = ambit::BlockedTensor::build(tensor_type_, "Temp110", {"aa"});
    temp["uv"] += H1["ev"] * T1["ue"];
    temp["uv"] -= H1["um"] * T1["mv"];

    E += L1_["vu"] * temp["uv"];

    E *= alpha;
    C0 += E;

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H1, T1] -> C0 : %12.3f", timer.get());
    }
    dsrg_time_.add("110", timer.get());
}

void SADSRG::H1_T2_C0(BlockedTensor& H1, BlockedTensor& T2, const double& alpha, double& C0) {
    local_timer timer;

    double E = 0.0;
    auto temp = ambit::BlockedTensor::build(tensor_type_, "Temp120", {"aaaa"});
    temp["uvxy"] += H1["ex"] * T2["uvey"];
    temp["uvxy"] -= H1["vm"] * T2["umxy"];

    E += L2_["xyuv"] * temp["uvxy"];

    E *= alpha;
    C0 += E;

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H1, T2] -> C0 : %12.3f", timer.get());
    }
    dsrg_time_.add("120", timer.get());
}

void SADSRG::H2_T1_C0(BlockedTensor& H2, BlockedTensor& T1, const double& alpha, double& C0) {
    local_timer timer;

    double E = 0.0;

    auto temp = ambit::BlockedTensor::build(tensor_type_, "Temp120", {"aaaa"});
    temp["uvxy"] += H2["evxy"] * T1["ue"];
    temp["uvxy"] -= H2["uvmy"] * T1["mx"];

    E += L2_["xyuv"] * temp["uvxy"];

    E *= alpha;
    C0 += E;

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H2, T1] -> C0 : %12.3f", timer.get());
    }
    dsrg_time_.add("210", timer.get());
}

void SADSRG::H2_T2_C0(BlockedTensor& H2, BlockedTensor& T2, BlockedTensor& S2, const double& alpha,
                      double& C0) {
    local_timer timer;

    double E = 0.0;

    // [H2, T2] (C_2)^4 from ccvv
    E += H2["efmn"] * S2["mnef"];

    // [H2, T2] (C_2)^4 L1 from cavv
    E += H2["efmu"] * S2["mvef"] * L1_["uv"];

    // [H2, T2] (C_2)^4 L1 from ccav
    E += H2["vemn"] * S2["mnue"] * Eta1_["uv"];

    // other terms involving T2 with at least two active indices
    H2_T2_C0_T2small(H2, T2, S2, E);

    // multiply prefactor and copy to C0
    E *= alpha;
    C0 += E;

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H2, T2] -> C0 : %12.3f", timer.get());
    }
    dsrg_time_.add("220", timer.get());
}

void SADSRG::H2_T2_C0_T2small(BlockedTensor& H2, BlockedTensor& T2, BlockedTensor& S2, double& C0) {
    /**
     * Note the following blocks should be available in memory.
     * H2: vvaa, aacc, avca, avac, vaaa, aaca
     * T2: aavv, ccaa, caav, acav, aava, caaa
     * S2: aavv, ccaa, caav, acav, aava, caaa
     */

    // [H2, T2] L1 from aavv
    C0 += 0.25 * H2["efxu"] * S2["yvef"] * L1_["uv"] * L1_["xy"];

    // [H2, T2] L1 from ccaa
    C0 += 0.25 * H2["vymn"] * S2["mnux"] * Eta1_["uv"] * Eta1_["xy"];

    // [H2, T2] L1 from caav
    auto temp = ambit::BlockedTensor::build(tensor_type_, "temp_caav", {"aaaa"});
    temp["uxyv"] += 0.5 * H2["vemx"] * S2["myue"];
    temp["uxyv"] += 0.5 * H2["vexm"] * S2["ymue"];
    C0 += temp["uxyv"] * Eta1_["uv"] * L1_["xy"];

    // [H2, T2] L1 from caaa and aaav
    temp.zero();
    temp.set_name("temp_aaav_caaa");
    temp["uxyv"] += 0.25 * H2["evwx"] * S2["zyeu"] * L1_["wz"];
    temp["uxyv"] += 0.25 * H2["vzmx"] * S2["myuw"] * Eta1_["wz"];
    C0 += temp["uxyv"] * Eta1_["uv"] * L1_["xy"];

    // <[Hbar2, T2]> C_4 (C_2)^2
    temp.zero();
    temp.set_name("temp_H2T2C0_L2");

    // HH
    temp["uvxy"] += 0.5 * H2["uvmn"] * T2["mnxy"];
    temp["uvxy"] += 0.5 * H2["uvmw"] * T2["mzxy"] * L1_["wz"];

    // PP
    temp["uvxy"] += 0.5 * H2["efxy"] * T2["uvef"];
    temp["uvxy"] += 0.5 * H2["ezxy"] * T2["uvew"] * Eta1_["wz"];

    // HP
    temp["uvxy"] += H2["uexm"] * S2["vmye"];
    temp["uvxy"] -= H2["uemx"] * T2["vmye"];
    temp["uvxy"] -= H2["vemx"] * T2["muye"];

    // HP with Gamma1
    temp["uvxy"] += 0.5 * H2["euwx"] * S2["zvey"] * L1_["wz"];
    temp["uvxy"] -= 0.5 * H2["euxw"] * T2["zvey"] * L1_["wz"];
    temp["uvxy"] -= 0.5 * H2["evxw"] * T2["uzey"] * L1_["wz"];

    // HP with Eta1
    temp["uvxy"] += 0.5 * H2["wumx"] * S2["mvzy"] * Eta1_["wz"];
    temp["uvxy"] -= 0.5 * H2["uwmx"] * T2["mvzy"] * Eta1_["wz"];
    temp["uvxy"] -= 0.5 * H2["vwmx"] * T2["muyz"] * Eta1_["wz"];

    C0 += temp["uvxy"] * L2_["uvxy"];

    // <[Hbar2, T2]> C_6 C_2
    if (foptions_->get_str("THREEPDC") != "ZERO") {
        C0 += H2.block("vaaa")("ewxy") * T2.block("aava")("uvez") * rdms_.SF_L3()("xyzuwv");
        C0 -= H2.block("aaca")("uvmz") * T2.block("caaa")("mwxy") * rdms_.SF_L3()("xyzuwv");
    }
}

void SADSRG::H1_T1_C1(BlockedTensor& H1, BlockedTensor& T1, const double& alpha,
                      BlockedTensor& C1) {
    local_timer timer;

    C1["ip"] += alpha * H1["ap"] * T1["ia"];
    C1["qa"] -= alpha * H1["qi"] * T1["ia"];

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H1, T1] -> C1 : %12.3f", timer.get());
    }
    dsrg_time_.add("111", timer.get());
}

void SADSRG::H1_T2_C1(BlockedTensor& H1, BlockedTensor& T2, const double& alpha,
                      BlockedTensor& C1) {
    local_timer timer;

    C1["ia"] += 2.0 * alpha * H1["bm"] * T2["imab"];
    C1["ia"] -= alpha * H1["bm"] * T2["miab"];

    C1["ia"] += alpha * H1["bu"] * T2["ivab"] * L1_["uv"];
    C1["ia"] -= 0.5 * alpha * H1["bu"] * T2["viab"] * L1_["uv"];

    C1["ia"] -= alpha * H1["vj"] * T2["ijau"] * L1_["uv"];
    C1["ia"] += 0.5 * alpha * H1["vj"] * T2["jiau"] * L1_["uv"];

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H1, T2] -> C1 : %12.3f", timer.get());
    }
    dsrg_time_.add("121", timer.get());
}

void SADSRG::H2_T1_C1(BlockedTensor& H2, BlockedTensor& T1, const double& alpha,
                      BlockedTensor& C1) {
    local_timer timer;

    C1["qp"] += 2.0 * alpha * T1["ma"] * H2["qapm"];
    C1["qp"] -= alpha * T1["ma"] * H2["aqpm"];

    C1["qp"] += alpha * T1["xe"] * L1_["yx"] * H2["qepy"];
    C1["qp"] -= 0.5 * alpha * T1["xe"] * L1_["yx"] * H2["eqpy"];

    C1["qp"] -= alpha * T1["mu"] * L1_["uv"] * H2["qvpm"];
    C1["qp"] += 0.5 * alpha * T1["mu"] * L1_["uv"] * H2["vqpm"];

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H2, T1] -> C1 : %12.3f", timer.get());
    }
    dsrg_time_.add("211", timer.get());
}

void SADSRG::H2_T2_C1(BlockedTensor& H2, BlockedTensor& T2, BlockedTensor& S2, const double& alpha,
                      BlockedTensor& C1) {
    local_timer timer;

    // [Hbar2, T2] (C_2)^3 -> C1 particle contractions
    C1["ir"] += alpha * H2["abrm"] * S2["imab"];

    C1["ir"] += 0.5 * alpha * L1_["uv"] * S2["ivab"] * H2["abru"];

    C1["ir"] += 0.25 * alpha * S2["ijux"] * L1_["xy"] * L1_["uv"] * H2["vyrj"];

    C1["ir"] -= 0.5 * alpha * L1_["uv"] * S2["imub"] * H2["vbrm"];
    C1["ir"] -= 0.5 * alpha * L1_["uv"] * S2["miub"] * H2["bvrm"];

    C1["ir"] -= 0.25 * alpha * S2["iyub"] * L1_["uv"] * L1_["xy"] * H2["vbrx"];
    C1["ir"] -= 0.25 * alpha * S2["iybu"] * L1_["uv"] * L1_["xy"] * H2["bvrx"];

    // [Hbar2, T2] C_4 C_2 2:2 -> C1 ir
    C1["ir"] += 0.5 * alpha * T2["ijxy"] * L2_["xyuv"] * H2["uvrj"];

    C1["ir"] += 0.5 * alpha * H2["aurx"] * S2["ivay"] * L2_["xyuv"];
    C1["ir"] -= 0.5 * alpha * H2["uarx"] * T2["ivay"] * L2_["xyuv"];
    C1["ir"] -= 0.5 * alpha * H2["uarx"] * T2["ivya"] * L2_["xyvu"];

    // [Hbar2, T2] (C_2)^3 -> C1 hole contractions
    C1["pa"] -= alpha * H2["peij"] * S2["ijae"];

    C1["pa"] -= 0.5 * alpha * Eta1_["uv"] * S2["ijau"] * H2["pvij"];

    C1["pa"] -= 0.25 * alpha * S2["vyab"] * Eta1_["uv"] * Eta1_["xy"] * H2["pbux"];

    C1["pa"] += 0.5 * alpha * Eta1_["uv"] * S2["vjae"] * H2["peuj"];
    C1["pa"] += 0.5 * alpha * Eta1_["uv"] * S2["jvae"] * H2["peju"];

    C1["pa"] += 0.25 * alpha * S2["vjax"] * Eta1_["uv"] * Eta1_["xy"] * H2["pyuj"];
    C1["pa"] += 0.25 * alpha * S2["jvax"] * Eta1_["xy"] * Eta1_["uv"] * H2["pyju"];

    // [Hbar2, T2] C_4 C_2 2:2 -> C1 pa
    C1["pa"] -= 0.5 * alpha * L2_["xyuv"] * T2["uvab"] * H2["pbxy"];

    C1["pa"] -= 0.5 * alpha * H2["puix"] * S2["ivay"] * L2_["xyuv"];
    C1["pa"] += 0.5 * alpha * H2["puxi"] * T2["ivay"] * L2_["xyuv"];
    C1["pa"] += 0.5 * alpha * H2["puxi"] * T2["viay"] * L2_["xyvu"];

    // [Hbar2, T2] C_4 C_2 1:3 -> C1
    C1["jb"] += 0.5 * alpha * H2["avxy"] * S2["ujab"] * L2_["xyuv"];

    C1["jb"] -= 0.5 * alpha * H2["uviy"] * S2["ijxb"] * L2_["xyuv"];

    C1["qs"] += alpha * H2["eqxs"] * T2["uvey"] * L2_["xyuv"];
    C1["qs"] -= 0.5 * alpha * H2["eqsx"] * T2["uvey"] * L2_["xyuv"];

    C1["qs"] -= alpha * H2["uqms"] * T2["mvxy"] * L2_["xyuv"];
    C1["qs"] += 0.5 * alpha * H2["uqsm"] * T2["mvxy"] * L2_["xyuv"];

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H2, T2] -> C1 : %12.3f", timer.get());
    }
    dsrg_time_.add("221", timer.get());
}

void SADSRG::H1_T2_C2(BlockedTensor& H1, BlockedTensor& T2, const double& alpha,
                      BlockedTensor& C2) {
    local_timer timer;

    C2["ijpb"] += alpha * T2["ijab"] * H1["ap"];
    C2["jibp"] += alpha * T2["ijab"] * H1["ap"];

    C2["qjab"] -= alpha * T2["ijab"] * H1["qi"];
    C2["jqba"] -= alpha * T2["ijab"] * H1["qi"];

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H1, T2] -> C2 : %12.3f", timer.get());
    }
    dsrg_time_.add("122", timer.get());
}

void SADSRG::H2_T1_C2(BlockedTensor& H2, BlockedTensor& T1, const double& alpha,
                      BlockedTensor& C2) {
    local_timer timer;

    C2["irpq"] += alpha * T1["ia"] * H2["arpq"];
    C2["riqp"] += alpha * T1["ia"] * H2["arpq"];

    C2["rsaq"] -= alpha * T1["ia"] * H2["rsiq"];
    C2["srqa"] -= alpha * T1["ia"] * H2["rsiq"];

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H2, T1] -> C2 : %12.3f", timer.get());
    }
    dsrg_time_.add("212", timer.get());
}

void SADSRG::H2_T2_C2(BlockedTensor& H2, BlockedTensor& T2, BlockedTensor& S2, const double& alpha,
                      BlockedTensor& C2) {
    local_timer timer;

    // particle-particle contractions
    C2["ijrs"] += alpha * H2["abrs"] * T2["ijab"];

    C2["ijrs"] -= 0.5 * alpha * L1_["xy"] * T2["ijxb"] * H2["ybrs"];
    C2["jisr"] -= 0.5 * alpha * L1_["xy"] * T2["ijxb"] * H2["ybrs"];

    // hole-hole contractions
    C2["pqab"] += alpha * H2["pqij"] * T2["ijab"];

    C2["pqab"] -= 0.5 * alpha * Eta1_["xy"] * T2["yjab"] * H2["pqxj"];
    C2["qpba"] -= 0.5 * alpha * Eta1_["xy"] * T2["yjab"] * H2["pqxj"];

    // hole-particle contractions
    std::vector<std::string> blocks;
    for (const std::string& block : C2.block_labels()) {
        if (block.substr(1, 1) == virt_label_ or block.substr(3, 1) == core_label_)
            continue;
        else
            blocks.push_back(block);
    }

    auto temp = ambit::BlockedTensor::build(tensor_type_, "temp", blocks);
    temp["qjsb"] += alpha * H2["aqms"] * S2["mjab"];
    temp["qjsb"] -= alpha * H2["aqsm"] * T2["mjab"];
    temp["qjsb"] += 0.5 * alpha * L1_["xy"] * S2["yjab"] * H2["aqxs"];
    temp["qjsb"] -= 0.5 * alpha * L1_["xy"] * T2["yjab"] * H2["aqsx"];
    temp["qjsb"] -= 0.5 * alpha * L1_["xy"] * S2["ijxb"] * H2["yqis"];
    temp["qjsb"] += 0.5 * alpha * L1_["xy"] * T2["ijxb"] * H2["yqsi"];

    C2["qjsb"] += temp["qjsb"];
    C2["jqbs"] += temp["qjsb"];

    blocks.clear();
    for (const std::string& block : C2.block_labels()) {
        if (block.substr(0, 1) == virt_label_ or block.substr(3, 1) == core_label_)
            continue;
        else
            blocks.push_back(block);
    }

    temp = ambit::BlockedTensor::build(tensor_type_, "temp", blocks);
    temp["jqsb"] -= alpha * H2["aqsm"] * T2["mjba"];
    temp["jqsb"] -= 0.5 * alpha * L1_["xy"] * T2["yjba"] * H2["aqsx"];
    temp["jqsb"] += 0.5 * alpha * L1_["xy"] * T2["ijbx"] * H2["yqsi"];

    C2["jqsb"] += temp["jqsb"];
    C2["qjbs"] += temp["jqsb"];

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H2, T2] -> C2 : %12.3f", timer.get());
    }
    dsrg_time_.add("222", timer.get());
}

void SADSRG::V_T1_C0_DF(BlockedTensor& B, BlockedTensor& T1, const double& alpha, double& C0) {
    local_timer timer;

    double E = 0.0;

    auto temp = ambit::BlockedTensor::build(tensor_type_, "DFtemp120", {"Laa"});
    temp["gux"] += B["gex"] * T1["ue"];
    temp["gux"] -= B["gum"] * T1["mx"];

    E += L2_["xyuv"] * temp["gux"] * B["gvy"];

    E *= alpha;
    C0 += E;

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H2, T1] -> C0 : %12.3f", timer.get());
    }
    dsrg_time_.add("210", timer.get());
}

void SADSRG::V_T2_C0_DF(BlockedTensor& B, BlockedTensor& T2, BlockedTensor& S2, const double& alpha,
                        double& C0) {
    local_timer timer;

    double E = 0.0;

    // [H2, T2] (C_2)^4 from ccvv, cavv, and ccav
    auto temp = ambit::BlockedTensor::build(tensor_type_, "temp_220", {"Lvc"});
    temp["gem"] += B["gfn"] * S2["mnef"];
    temp["gem"] += B["gfu"] * S2["mvef"] * L1_["uv"];
    temp["gem"] += B["gvn"] * S2["nmue"] * Eta1_["uv"];
    E += temp["gem"] * B["gem"];

    // form H2 for other blocks that fits memory
    std::vector<std::string> blocks{"aacc", "aaca", "vvaa", "vaaa", "avac", "avca"};
    auto H2 = ambit::BlockedTensor::build(tensor_type_, "temp_H2", blocks);
    H2["abij"] = B["gai"] * B["gbj"];

    H2_T2_C0_T2small(H2, T2, S2, E);

    // multiply prefactor and copy to C0
    E *= alpha;
    C0 += E;

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H2, T2] -> C0 : %12.3f", timer.get());
    }
    dsrg_time_.add("220", timer.get());
}

void SADSRG::V_T1_C1_DF(BlockedTensor& B, BlockedTensor& T1, const double& alpha,
                        BlockedTensor& C1) {
    local_timer timer;

    auto temp = ambit::BlockedTensor::build(tensor_type_, "DFtemp211", {"L"});
    temp["g"] += 2.0 * alpha * T1["ma"] * B["gam"];
    temp["g"] += alpha * T1["xe"] * L1_["yx"] * B["gey"];
    temp["g"] -= alpha * T1["mu"] * L1_["uv"] * B["gvm"];
    C1["qp"] += temp["g"] * B["gqp"];

    temp = ambit::BlockedTensor::build(tensor_type_, "DFtemp211", {"Lgc"});
    temp["gpm"] -= alpha * T1["ma"] * B["gap"];
    temp["gpm"] += 0.5 * alpha * T1["mu"] * L1_["uv"] * B["gvp"];
    C1["qp"] += temp["gpm"] * B["gqm"];

    C1["qp"] -= 0.5 * alpha * T1["xe"] * L1_["yx"] * B["gep"] * B["gqy"];

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H2, T1] -> C1 : %12.3f", timer.get());
    }
    dsrg_time_.add("211", timer.get());
}

void SADSRG::V_T2_C1_DF(BlockedTensor& B, BlockedTensor& T2, BlockedTensor& S2, const double& alpha,
                        BlockedTensor& C1) {
    local_timer timer;

    // [Hbar2, T2] (C_2)^3 -> C1 particle contractions
    auto temp = ambit::BlockedTensor::build(tensor_type_, "DFtemp221", {"Lhp"});

    temp["gia"] += alpha * B["gbm"] * S2["imab"];

    temp["gia"] += 0.5 * alpha * L1_["uv"] * S2["ivab"] * B["gbu"];

    temp["giv"] += 0.25 * alpha * S2["ijux"] * L1_["xy"] * L1_["uv"] * B["gyj"];

    temp["giv"] -= 0.5 * alpha * L1_["uv"] * S2["imub"] * B["gbm"];
    temp["gia"] -= 0.5 * alpha * L1_["uv"] * S2["miua"] * B["gvm"];

    temp["giv"] -= 0.25 * alpha * S2["iyub"] * L1_["uv"] * L1_["xy"] * B["gbx"];
    temp["gia"] -= 0.25 * alpha * S2["iyau"] * L1_["uv"] * L1_["xy"] * B["gvx"];

    // [Hbar2, T2] C_4 C_2 2:2 -> C1 ir
    temp["giu"] += 0.5 * alpha * T2["ijxy"] * L2_["xyuv"] * B["gvj"];

    temp["gia"] += 0.5 * alpha * B["gux"] * S2["ivay"] * L2_["xyuv"];
    temp["giu"] -= 0.5 * alpha * B["gax"] * T2["ivay"] * L2_["xyuv"];
    temp["giu"] -= 0.5 * alpha * B["gax"] * T2["ivya"] * L2_["xyvu"];

    C1["ir"] += temp["gia"] * B["gar"];

    // [Hbar2, T2] (C_2)^3 -> C1 hole contractions
    temp.zero();

    temp["gia"] -= alpha * B["gej"] * S2["ijae"];

    temp["gia"] -= 0.5 * alpha * Eta1_["uv"] * S2["ijau"] * B["gvj"];

    temp["gua"] -= 0.25 * alpha * S2["vyab"] * Eta1_["uv"] * Eta1_["xy"] * B["gbx"];

    temp["gua"] += 0.5 * alpha * Eta1_["uv"] * S2["vjae"] * B["gej"];
    temp["gia"] += 0.5 * alpha * Eta1_["uv"] * S2["ivae"] * B["geu"];

    temp["gua"] += 0.25 * alpha * S2["vjax"] * Eta1_["uv"] * Eta1_["xy"] * B["gyj"];
    temp["gia"] += 0.25 * alpha * S2["ivax"] * Eta1_["xy"] * Eta1_["uv"] * B["gyu"];

    // [Hbar2, T2] C_4 C_2 2:2 -> C1 pa
    temp["gxa"] -= 0.5 * alpha * L2_["xyuv"] * T2["uvab"] * B["gby"];

    temp["gia"] -= 0.5 * alpha * B["gux"] * S2["ivay"] * L2_["xyuv"];
    temp["gxa"] += 0.5 * alpha * B["gui"] * T2["ivay"] * L2_["xyuv"];
    temp["gxa"] += 0.5 * alpha * B["gui"] * T2["viay"] * L2_["xyvu"];

    C1["pa"] += temp["gia"] * B["gpi"];

    // [Hbar2, T2] C_4 C_2 1:3 -> C1
    temp = ambit::BlockedTensor::build(tensor_type_, "DFtemp221", {"Laa"});
    temp["gxu"] = B["gvy"] * L2_["xyuv"];
    C1["jb"] += 0.5 * alpha * B["gax"] * S2["ujab"] * temp["gxu"];
    C1["jb"] -= 0.5 * alpha * B["gui"] * S2["ijxb"] * temp["gxu"];

    temp = ambit::BlockedTensor::build(tensor_type_, "DFtemp221", {"L"});
    temp["g"] += alpha * B["gex"] * T2["uvey"] * L2_["xyuv"];
    temp["g"] -= alpha * B["gum"] * T2["mvxy"] * L2_["xyuv"];
    C1["qs"] += temp["g"] * B["gqs"];

    C1["qs"] -= 0.5 * alpha * B["ges"] * B["gqx"] * T2["uvey"] * L2_["xyuv"];

    C1["qs"] += 0.5 * alpha * B["gus"] * B["gqm"] * T2["mvxy"] * L2_["xyuv"];

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H2, T2] -> C1 : %12.3f", timer.get());
    }
    dsrg_time_.add("221", timer.get());
}

void SADSRG::V_T1_C2_DF(BlockedTensor& B, BlockedTensor& T1, const double& alpha,
                        BlockedTensor& C2) {
    local_timer timer;

    C2["irpq"] += alpha * T1["ia"] * B["gap"] * B["grq"];
    C2["riqp"] += alpha * T1["ia"] * B["gap"] * B["grq"];
    C2["rsaq"] -= alpha * T1["ia"] * B["gri"] * B["gsq"];
    C2["srqa"] -= alpha * T1["ia"] * B["gri"] * B["gsq"];

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H2, T1] -> C2 : %12.3f", timer.get());
    }
    dsrg_time_.add("212", timer.get());
}

void SADSRG::V_T2_C2_DF(BlockedTensor& B, BlockedTensor& T2, BlockedTensor& S2, const double& alpha,
                        BlockedTensor& C2) {
    local_timer timer;

    // particle-particle contractions
    C2["ijrs"] += batched("r", alpha * B["gar"] * B["gbs"] * T2["ijab"]);

    C2["ijrs"] -= 0.5 * alpha * L1_["xy"] * T2["ijxb"] * B["gyr"] * B["gbs"];
    C2["jisr"] -= 0.5 * alpha * L1_["xy"] * T2["ijxb"] * B["gyr"] * B["gbs"];

    // hole-hole contractions
    C2["pqab"] += alpha * B["gpi"] * B["gqj"] * T2["ijab"];

    C2["pqab"] -= 0.5 * alpha * Eta1_["xy"] * T2["yjab"] * B["gpx"] * B["gqj"];
    C2["qpba"] -= 0.5 * alpha * Eta1_["xy"] * T2["yjab"] * B["gpx"] * B["gqj"];

    // hole-particle contractions
    auto temp = ambit::BlockedTensor::build(tensor_type_, "DFtemp222", {"Lhp"});
    temp["gjb"] += alpha * B["gam"] * S2["mjab"];
    temp["gjb"] += 0.5 * alpha * L1_["xy"] * S2["yjab"] * B["gax"];
    temp["gjb"] -= 0.5 * alpha * L1_["xy"] * S2["ijxb"] * B["gyi"];

    C2["qjsb"] += temp["gjb"] * B["gqs"];
    C2["jqbs"] += temp["gjb"] * B["gqs"];

    // exchange like terms
    V_T2_C2_DF_PH_X(B, T2, alpha, C2);

    if (print_ > 2) {
        outfile->Printf("\n    Time for [H2, T2] -> C2 : %12.3f", timer.get());
    }
    dsrg_time_.add("222", timer.get());
}

void SADSRG::V_T2_C2_DF_PH_X(BlockedTensor& B, BlockedTensor& T2, const double& alpha,
                             BlockedTensor& C2) {

    std::vector<std::string> qjsb_small, qjsb_large, jqsb_small, jqsb_large;

    for (const std::string& block : C2.block_labels()) {
        auto j = block.substr(1, 1);
        auto b = block.substr(3, 1);

        if (j != virt_label_ and b != core_label_) {
            if (std::count(block.begin(), block.end(), 'v') > 2) {
                qjsb_large.push_back(block);
            } else {
                qjsb_small.push_back(block);
            }
        }

        j = block.substr(0, 1);
        if (j != virt_label_ and b != core_label_) {
            if (std::count(block.begin(), block.end(), 'v') > 2) {
                jqsb_large.push_back(block);
            } else {
                jqsb_small.push_back(block);
            }
        }
    }

    auto temp = ambit::BlockedTensor::build(tensor_type_, "DFtemp222PHX", qjsb_small);
    temp["qjsb"] -= alpha * B["gas"] * B["gqm"] * T2["mjab"];
    temp["qjsb"] -= 0.5 * alpha * L1_["xy"] * T2["yjab"] * B["gas"] * B["gqx"];
    temp["qjsb"] += 0.5 * alpha * L1_["xy"] * T2["ijxb"] * B["gys"] * B["gqi"];

    C2["qjsb"] += temp["qjsb"];
    C2["jqbs"] += temp["qjsb"];

    temp = ambit::BlockedTensor::build(tensor_type_, "DFtemp222PHX", jqsb_small);
    temp["jqsb"] -= alpha * B["gas"] * B["gqm"] * T2["mjba"];
    temp["jqsb"] -= 0.5 * alpha * L1_["xy"] * T2["yjba"] * B["gas"] * B["gqx"];
    temp["jqsb"] += 0.5 * alpha * L1_["xy"] * T2["ijbx"] * B["gys"] * B["gqi"];

    C2["jqsb"] += temp["jqsb"];
    C2["qjbs"] += temp["jqsb"];

    if (qjsb_large.size() != 0) {
        C2["e,j,f,v0"] -= batched("e", alpha * B["g,a,f"] * B["g,e,m"] * T2["m,j,a,v0"]);
        C2["j,e,v0,f"] -= batched("e", alpha * B["g,a,f"] * B["g,e,m"] * T2["m,j,a,v0"]);

        temp = ambit::BlockedTensor::build(tensor_type_, "DFtemp222PHX", {"ahpv"});
        temp["xjae"] = L1_["xy"] * T2["yjae"];
        C2["e,j,f,v0"] -= batched("e", 0.5 * alpha * temp["x,j,a,v0"] * B["g,a,f"] * B["g,e,x"]);
        C2["j,e,v0,f"] -= batched("e", 0.5 * alpha * temp["x,j,a,v0"] * B["g,a,f"] * B["g,e,x"]);

        temp = ambit::BlockedTensor::build(tensor_type_, "DFtemp222PHX", {"hhav"});
        temp["ijye"] = L1_["xy"] * T2["ijxe"];
        C2["e,j,f,v0"] += batched("e", 0.5 * alpha * temp["i,j,y,v0"] * B["g,y,f"] * B["g,e,i"]);
        C2["j,e,v0,f"] += batched("e", 0.5 * alpha * temp["i,j,y,v0"] * B["g,y,f"] * B["g,e,i"]);
    }

    if (jqsb_large.size() != 0) {
        C2["j,e,f,v0"] -= batched("e", alpha * B["g,a,f"] * B["g,e,m"] * T2["m,j,v0,a"]);
        C2["e,j,v0,f"] -= batched("e", alpha * B["g,a,f"] * B["g,e,m"] * T2["m,j,v0,a"]);

        temp = ambit::BlockedTensor::build(tensor_type_, "DFtemp222PHX", {"ahvp"});
        temp["xjea"] = L1_["xy"] * T2["yjea"];
        C2["j,e,f,v0"] -= batched("e", 0.5 * alpha * temp["x,j,v0,a"] * B["g,a,f"] * B["g,e,x"]);
        C2["e,j,v0,f"] -= batched("e", 0.5 * alpha * temp["x,j,v0,a"] * B["g,a,f"] * B["g,e,x"]);

        temp = ambit::BlockedTensor::build(tensor_type_, "DFtemp222PHX", {"hhva"});
        temp["ijey"] = L1_["xy"] * T2["ijex"];
        C2["j,e,f,v0"] += batched("e", 0.5 * alpha * temp["i,j,v0,y"] * B["g,y,f"] * B["g,e,i"]);
        C2["e,j,v0,f"] += batched("e", 0.5 * alpha * temp["i,j,v0,y"] * B["g,y,f"] * B["g,e,i"]);
    }
}

void SADSRG::H_A_Ca(BlockedTensor& H1, BlockedTensor& H2, BlockedTensor& T1, BlockedTensor& T2,
                    const double& alpha, BlockedTensor& C1, BlockedTensor& C2) {
    // set up S2["ijab"] = 2 * T2["ijab"] - T2["ijba"]
    auto S2 = ambit::BlockedTensor::build(tensor_type_, "S2T", {"hhpp"});
    S2["ijab"] = 2.0 * T2["ijab"] - T2["ijba"];

    // set up G2["pqrs"] = 2 * H2["pqrs"] - H2["pqsr"]
    auto G2 = ambit::BlockedTensor::build(tensor_type_, "G2H", {"avac", "aaac", "avaa"});
    G2["pqrs"] = 2.0 * H2["pqrs"] - H2["pqsr"];

    H_A_Ca_small(H1, H2, G2, T1, T2, S2, alpha, C1, C2);

    auto temp = ambit::BlockedTensor::build(ambit::CoreTensor, "tempHACa", {"aa"});
    temp["wz"] += H2["abzm"] * S2["wmab"];
    temp["wz"] -= H2["weij"] * S2["ijze"];

    C1["uv"] += alpha * temp["uv"];
    C1["vu"] += alpha * temp["uv"];
}

void SADSRG::H_A_Ca_small(BlockedTensor& H1, BlockedTensor& H2, BlockedTensor& G2,
                          BlockedTensor& T1, BlockedTensor& T2, BlockedTensor& S2,
                          const double& alpha, BlockedTensor& C1, BlockedTensor& C2) {
    /**
     * The following blocks should be available in memory:
     * G2: avac, aaac, avaa
     * H2, T2, S2: all blocks with at least two active indices
     */

    auto temp = ambit::BlockedTensor::build(ambit::CoreTensor, "tempHACa", {"aa"});

    temp["uv"] += H1["ev"] * T1["ue"];
    temp["uv"] -= H1["um"] * T1["mv"];

    H_T_C1a_smallG(G2, T1, T2, temp);

    H_T_C1a_smallS(H1, H2, T2, S2, temp);

    C1["uv"] += alpha * temp["uv"];
    C1["vu"] += alpha * temp["uv"];

    temp = ambit::BlockedTensor::build(ambit::CoreTensor, "temp", {"aaaa"});

    H_T_C2a_smallS(H1, H2, T1, T2, S2, temp);

    C2["uvxy"] += alpha * temp["uvxy"];
    C2["xyuv"] += alpha * temp["uvxy"];
}

void SADSRG::H_T_C1a_smallG(BlockedTensor& G2, BlockedTensor& T1, BlockedTensor& T2,
                            BlockedTensor& C1) {
    /**
     * The following blocks should be available in memory:
     * G2: avac, aaac, avaa
     * T2: aava, caaa
     */

    C1["uv"] += T1["ma"] * G2["uavm"];
    C1["uv"] += 0.5 * T1["xe"] * L1_["yx"] * G2["uevy"];
    C1["uv"] -= 0.5 * T1["mx"] * L1_["xy"] * G2["uyvm"];

    C1["wz"] += 0.5 * G2["wezx"] * T2["uvey"] * L2_["xyuv"];
    C1["wz"] -= 0.5 * G2["wuzm"] * T2["mvxy"] * L2_["xyuv"];
}

void SADSRG::H_T_C1a_smallS(BlockedTensor& H1, BlockedTensor& H2, BlockedTensor& T2,
                            BlockedTensor& S2, BlockedTensor& C1) {
    /// H2, T2, and S2 should contain all blocks with at least two active indices.

    C1["uv"] += H1["bm"] * S2["umvb"];
    C1["uv"] += 0.5 * H1["bx"] * S2["uyvb"] * L1_["xy"];
    C1["uv"] -= 0.5 * H1["yj"] * S2["ujvx"] * L1_["xy"];

    auto temp = ambit::BlockedTensor::build(ambit::CoreTensor, "temp", {"aaaa"});
    temp["wzuv"] += 0.5 * S2["wvab"] * H2["abzu"];
    temp["wzuv"] -= 0.5 * S2["wmub"] * H2["vbzm"];
    temp["wzuv"] -= 0.5 * S2["mwub"] * H2["bvzm"];
    temp["wzuv"] += 0.25 * S2["wjux"] * L1_["xy"] * H2["vyzj"];
    temp["wzuv"] -= 0.25 * S2["wyub"] * L1_["xy"] * H2["vbzx"];
    temp["wzuv"] -= 0.25 * S2["wybu"] * L1_["xy"] * H2["bvzx"];
    C1["wz"] += temp["wzuv"] * L1_["uv"];

    temp.zero();
    temp["wzuv"] -= 0.5 * S2["ijzu"] * H2["wvij"];
    temp["wzuv"] += 0.5 * S2["vjze"] * H2["weuj"];
    temp["wzuv"] += 0.5 * S2["jvze"] * H2["weju"];
    temp["wzuv"] -= 0.25 * S2["vyzb"] * Eta1_["xy"] * H2["wbux"];
    temp["wzuv"] += 0.25 * S2["vjzx"] * Eta1_["xy"] * H2["wyuj"];
    temp["wzuv"] += 0.25 * S2["jvzx"] * Eta1_["xy"] * H2["wyju"];
    C1["wz"] += temp["wzuv"] * Eta1_["uv"];

    C1["wz"] += 0.5 * H2["uvzj"] * T2["jwyx"] * L2_["xyuv"];
    C1["wz"] += 0.5 * H2["auzx"] * S2["wvay"] * L2_["xyuv"];
    C1["wz"] -= 0.5 * H2["uazx"] * T2["wvay"] * L2_["xyuv"];
    C1["wz"] -= 0.5 * H2["uazx"] * T2["wvya"] * L2_["xyvu"];

    C1["wz"] -= 0.5 * H2["wbxy"] * T2["uvzb"] * L2_["xyuv"];
    C1["wz"] -= 0.5 * H2["wuix"] * S2["ivzy"] * L2_["xyuv"];
    C1["wz"] += 0.5 * H2["wuxi"] * T2["ivzy"] * L2_["xyuv"];
    C1["wz"] += 0.5 * H2["wuxi"] * T2["ivyz"] * L2_["xyvu"];

    C1["wz"] += 0.5 * H2["avxy"] * S2["uwaz"] * L2_["xyuv"];
    C1["wz"] -= 0.5 * H2["uviy"] * S2["iwxz"] * L2_["xyuv"];
}

void SADSRG::H_T_C2a_smallS(BlockedTensor& H1, BlockedTensor& H2, BlockedTensor& T1,
                            BlockedTensor& T2, BlockedTensor& S2, BlockedTensor& C2) {
    /// H2, T2, and S2 should contain all blocks with at least two active indices.

    C2["uvxy"] += H2["abxy"] * T2["uvab"];
    C2["uvxy"] += H2["uvij"] * T2["ijxy"];

    auto temp = ambit::BlockedTensor::build(ambit::CoreTensor, "temp", {"aaaa"});
    temp["uvxy"] += H1["ax"] * T2["uvay"];
    temp["uvxy"] -= H1["ui"] * T2["ivxy"];
    temp["uvxy"] += T1["ua"] * H2["avxy"];
    temp["uvxy"] -= T1["ix"] * H2["uviy"];

    temp["uvxy"] -= 0.5 * L1_["wz"] * T2["uvwa"] * H2["zaxy"];
    temp["uvxy"] -= 0.5 * Eta1_["wz"] * T2["zixy"] * H2["uvwi"];

    temp["uvxy"] += H2["aumx"] * S2["mvay"];
    temp["uvxy"] += 0.5 * L1_["wz"] * S2["zvay"] * H2["auwx"];
    temp["uvxy"] -= 0.5 * L1_["wz"] * S2["ivwy"] * H2["zuix"];

    temp["uvxy"] -= H2["auxm"] * T2["mvay"];
    temp["uvxy"] -= 0.5 * L1_["wz"] * T2["zvay"] * H2["auxw"];
    temp["uvxy"] += 0.5 * L1_["wz"] * T2["ivwy"] * H2["zuxi"];

    temp["uvxy"] -= H2["avxm"] * T2["muya"];
    temp["uvxy"] -= 0.5 * L1_["wz"] * T2["zuya"] * H2["avxw"];
    temp["uvxy"] += 0.5 * L1_["wz"] * T2["iuyw"] * H2["zvxi"];

    C2["uvxy"] += temp["uvxy"];
    C2["vuyx"] += temp["uvxy"];
}
} // namespace forte
