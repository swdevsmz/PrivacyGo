// Copyright 2023 TikTok Pte. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <random>
#include <string>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "nlohmann/json.hpp"

#include "dpca-psi/common/dummy_data_utils.h"
#include "dpca-psi/common/utils.h"
#include "dpca-psi/crypto/prng.h"
#include "dpca-psi/dp_cardinality_psi.h"
#include "dpca-psi/network/two_channel_net_io.h"
#include "ppam/ppam.h"

// 指定した範囲でランダムな特徴量ベクトルを生成する関数
// n: 要素数, min/max: 範囲, is_zero: trueなら全要素0
std::vector<double> random_features(std::size_t n, std::size_t min, std::size_t max, bool is_zero) {
    std::vector<double> result(n, 0.0);
    std::random_device rd("/dev/urandom");
    std::default_random_engine eng(rd());
    std::uniform_real_distribution<double> distr(static_cast<double>(min), static_cast<double>(max));
    if (!is_zero) {
        for (std::size_t i = 0; i < n; ++i) {
            result[i] = distr(eng) - (static_cast<double>(min) - static_cast<double>(max)) / 2.0;
        }
    }
    return result;
}

// PPAMプロトコルのサンプル実行関数
// config_path: 設定ファイルパス
// log_path: ログ出力ディレクトリ
// intersection_size: 両者の共通データ数
// intersection_ratio: 片方の全体データ数/共通データ数
// feature_size: 特徴量の数
// use_default_tau: 差分プライバシー用パラメータを強制指定するか
// default_tau: 差分プライバシー用パラメータ値
void ppam_example(const std::string& config_path, const std::string& log_path, std::size_t intersection_size,
        std::size_t intersection_ratio, std::size_t feature_size, bool use_default_tau, std::size_t default_tau) {
    auto start = privacy_go::dpca_psi::clock_start();

    // 1. 設定ファイル(JSON)を読み込む
    std::ifstream in(config_path);
    nlohmann::json params = nlohmann::json::parse(in, nullptr, true);
    in.close();

    // 送信者/受信者フラグや差分プライバシー有無を取得
    bool is_sender = params["common"]["is_sender"];
    bool input_dp = params["dp_params"]["input_dp"];

    // ログ出力設定
    FLAGS_alsologtostderr = 1;
    FLAGS_log_dir = log_path;
    std::string log_file_name;
    log_file_name = std::string("ppam_") + (is_sender ? "sender_" : "receiver_") +
                    (input_dp ? "with_dp_" : "without_dp_") + "intersection_size_" + std::to_string(intersection_size);
    google::InitGoogleLogging(log_file_name.c_str());

    // 2. 通信チャネルを確立（TCPソケット）
    std::string address = params["common"]["address"];
    std::uint16_t remote_port = params["common"]["remote_port"];
    std::uint16_t local_port = params["common"]["local_port"];
    auto net = std::make_shared<privacy_go::dpca_psi::TwoChannelNetIO>(address, remote_port, local_port);

    // 3. 入力データ（ID・特徴量）を生成
    //   - 本来はファイルから読むが、ここでは乱数で生成
    std::vector<std::vector<std::string>> keys;  // 各カラムのIDリスト
    std::vector<std::vector<double>> features;   // 各カラムの特徴量リスト

    double expected_sum = 0.0;  // 差分プライバシー無しの理論値
    double actual_sum = 0.0;    // 実際の計算値

    std::size_t data_size = intersection_ratio * intersection_size;  // 片方の全体データ数
    std::size_t key_size = params["common"]["ids_num"];              // IDカラム数

    // 乱数生成器の初期化（共通シードは通信で同期）
    privacy_go::dpca_psi::block common_seed = privacy_go::dpca_psi::kZeroBlock;
    if (is_sender) {
        // 送信者がシードを生成し、受信者に送信
        common_seed = privacy_go::dpca_psi::read_block_from_dev_urandom();
        net->send_value<privacy_go::dpca_psi::block>(common_seed);
    } else {
        // 受信者はシードを受信
        common_seed = net->recv_value<privacy_go::dpca_psi::block>();
    }

    privacy_go::dpca_psi::PRNG common_prng;
    common_prng.set_seed(common_seed);  // 共通部分用
    privacy_go::dpca_psi::PRNG unique_prng;
    unique_prng.set_seed(privacy_go::dpca_psi::read_block_from_dev_urandom());  // ユニーク部分用

    // IDリスト生成（カラム数ごとに）
    keys.reserve(key_size);
    if (key_size == 3) {
        // 3カラムの場合は交差割合を個別に設定
        std::vector<double> column_intersection_ratio = {0.85, 0.1, 0.05};
        std::size_t accumulated_intersection_size = 0;
        for (std::size_t key_idx = 0; key_idx < key_size; ++key_idx) {
            // 各カラムの交差サイズを計算
            std::size_t cur_intersection_size = std::min(
                    std::size_t(std::ceil(static_cast<double>(intersection_size) * column_intersection_ratio[key_idx])),
                    intersection_size - accumulated_intersection_size);
            // 共通部分のID生成
            auto common_keys = privacy_go::dpca_psi::random_keys(
                    common_prng, cur_intersection_size, "bench" + std::to_string(key_idx));
            // ユニーク部分のID生成
            std::string unique_suffix = (is_sender ? "sender" : "receiver") + std::to_string(key_idx);
            auto unique_keys =
                    privacy_go::dpca_psi::random_keys(unique_prng, data_size - cur_intersection_size, unique_suffix);
            // 共通部分をユニーク部分に挿入
            unique_keys.insert(
                    unique_keys.begin() + accumulated_intersection_size, common_keys.begin(), common_keys.end());
            accumulated_intersection_size += cur_intersection_size;
            keys.emplace_back(unique_keys);
        }
    } else {
        // それ以外のカラム数の場合
        std::size_t column_intersection_size = (intersection_size + key_size - 1) / key_size;
        for (std::size_t key_idx = 0; key_idx < key_size; ++key_idx) {
            std::size_t cur_intersection_size =
                    std::min(column_intersection_size, intersection_size - key_idx * column_intersection_size);
            auto common_keys = privacy_go::dpca_psi::random_keys(
                    common_prng, cur_intersection_size, "bench" + std::to_string(key_idx));
            std::string unique_suffix = (is_sender ? "sender" : "receiver") + std::to_string(key_idx);
            auto unique_keys =
                    privacy_go::dpca_psi::random_keys(unique_prng, data_size - cur_intersection_size, unique_suffix);
            unique_keys.insert(
                    unique_keys.begin() + key_idx * column_intersection_size, common_keys.begin(), common_keys.end());
            keys.emplace_back(unique_keys);
        }
    }

    // 特徴量ベクトルを生成
    features.reserve(feature_size);
    for (std::size_t i = 0; i < feature_size; ++i) {
        features.emplace_back(random_features(data_size, 5, 10, false));
    }

    // 差分プライバシー用パラメータを上書き
    if (use_default_tau) {
        params["dp_params"]["precomputed_tau"] = default_tau;
    }

    std::size_t communication = 0;

    // 4. PPAMプロトコル本体を実行
    //    - initialize: パラメータ・通信初期化
    //    - measurement: プライバシー保護付きで集計値を計算
    privacy_go::ppam::PrivacyMeasurement ads_measure;
    ads_measure.initialize(params, net);
    actual_sum = ads_measure.measurement(15.0, keys, features);

    // 5. 統計情報の計算・出力
    auto duration = static_cast<double>(privacy_go::dpca_psi::time_from(start)) * 1.0 / 1000000.0;  // 秒
    communication = net->get_bytes_sent();                                                          // 送信バイト数
    std::size_t remote_communication = 0;
    if (is_sender) {
        // 送信者は自分の通信量を先に送信
        net->send_value<std::uint64_t>(communication);
        remote_communication = net->recv_value<std::uint64_t>();
    } else {
        // 受信者は先に受信
        remote_communication = net->recv_value<std::uint64_t>();
        net->send_value<std::uint64_t>(communication);
    }

    double self_comm = static_cast<double>(communication) * 1.0 / (1024 * 1024);  // MB換算
    double remote_comm = static_cast<double>(remote_communication) * 1.0 / (1024 * 1024);
    double total_comm = static_cast<double>(communication + remote_communication) * 1.0 / (1024 * 1024);

    // 差分プライバシー無しの理論値も計算
    expected_sum = ads_measure.plain_measurement(15.0, keys, features);

    // 結果をログ出力
    LOG(INFO) << "-------------------------------";
    LOG(INFO) << (is_sender ? "Sender" : "Receiver");
    LOG(INFO) << "Apply dp: " << params["dp_params"]["input_dp"];
    LOG(INFO) << "Total Communication is " << total_comm << "(" << self_comm << " + " << remote_comm << ")"
              << "MB." << std::endl;
    LOG(INFO) << "Total time is " << duration << " s.";
    LOG(INFO) << "Expected / Actual sum is " << expected_sum << " / " << actual_sum;

    // 差分が大きすぎる場合はエラー
    assert((expected_sum - actual_sum < 0.01));

    google::ShutdownGoogleLogging();
}

DEFINE_string(
        config_path, "./json/sender_with_precomputed_tau.json", "the path where the sender's config file located");
DEFINE_string(log_path, "./logs/", "the directory where log file located");
DEFINE_uint64(intersection_size, 10, "the intersection size of both party.");
DEFINE_uint64(intersection_ratio, 100, "the ratio of sender/receiver data size to intersection size.");
DEFINE_uint64(feature_size, 1, "the feature size of sender/receiver data.");
DEFINE_bool(use_default_tau, false, "whether or not use default tau. just for testing.");
DEFINE_uint64(default_tau, 1440, "default tau. just for testing.");

// メイン関数
// コマンドライン引数をパースし、ppam_exampleを実行
int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    ppam_example(FLAGS_config_path, FLAGS_log_path, FLAGS_intersection_size, FLAGS_intersection_ratio,
            FLAGS_feature_size, FLAGS_use_default_tau, FLAGS_default_tau);
    return 0;
}
