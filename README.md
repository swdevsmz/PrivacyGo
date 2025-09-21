# PrivacyGo-with-build-script

このリポジトリは PrivacyGo のフォークであり、ビルド手順を簡略化するためのシェルスクリプト（build.sh）を追加・整備しています。元のリポジトリに比べて、依存ライブラリのビルドやセットアップを自動化し、より簡単に環境構築できることが特徴です。

- 元のREADMEはこちら: [README_upstream.md](./README_upstream.md)

## 動作確認環境
- Windows 11 + WSL2 (Ubuntu 24.04)

## 変更点について

本リポジトリでは、元のリポジトリから一部ソースファイル（例: `csv_file_o.h` など）に `#include` の追加等の修正を行っています。

これらはビルド時にエラーが発生したため修正したものです。エラーが出た理由としては「必要なヘッダファイルがインクルードされておらず、型や関数が未定義となった」から「該当するヘッダファイルを明示的にインクルードする」ことで対応しています。

## ビルド方法

リポジトリのルートディレクトリで以下のコマンドを実行してください。

```bash
./build.sh
```

## DPCA-PSI（Differentially Private Cardinality Approximation - Private Set Intersection）サンプルプログラム実行方法

DPCA-PSIのサンプルプログラムは、送信側（sender）と受信側（receiver）をそれぞれ別々のターミナルで実行する必要があります。

1. 1つ目のターミナルで以下を実行（送信側）

```bash
cd ./dpca-psi/build/example/scripts && bash sender_test.sh
```

2. 2つ目のターミナルで以下を実行（受信側）

```bash
cd ./dpca-psi/build/example/scripts && bash receiver_test.sh
```

## PPAM（Privacy-Preserving Audience Measurement）サンプルプログラム実行方法

このサンプルプログラムは、送信側（sender）と受信側（receiver）でそれぞれ異なるデータセット（例：ユーザーIDや視聴履歴などのCSVファイル）をインプットとして使用します。

- 送信側・受信側ともに、`./ppam/build/example/scripts` ディレクトリ内に用意されたサンプルデータファイル[sender_input_file.csv](dpca-psi/example/data/sender_input_file.csv), [receiver_input_file.csv](dpca-psi/example/data/receiver_input_file.csv) など）を自動的に読み込みます。
- プログラムは、プライバシーを保護しながら両者のデータセットの共通部分（オーディエンスの重複数など）を安全に計算します。
- 計算処理には暗号化技術が用いられており、相手に生データを開示せずに集計が可能です。
- 最終的なアウトプットとして、両者のデータセットに共通する要素数（重複ユーザー数など）が各ターミナルに表示されます。

### サンプルデータ例

- `sender_input_file.csv`（送信側のデータ例）:
    ```csv
    id_0,sender_id_1_0,sender_id_2_0,1680433923637
    id_1,sender_id_1_1,sender_id_2_1,1680433923637
    sender_id_0_2,id_2,sender_id_2_2,1680433923637
    sender_id_0_3,id_3,sender_id_2_3,1680433923637
    sender_id_0_4,sender_id_1_4,id_4,1680433923637
    sender_id_0_5,sender_id_1_5,sender_id_2_5,1680433923637
    sender_id_0_6,sender_id_1_6,sender_id_2_6,1680433923637
    sender_id_0_7,sender_id_1_7,sender_id_2_7,1680433923637
    sender_id_0_8,sender_id_1_8,sender_id_2_8,1680433923637
    sender_id_0_9,sender_id_1_9,sender_id_2_9,1680433923637
```
- `receiver_input_file.csv`（受信側のデータ例）:
    ```csv
    id_0,receiver_id_1_0,receiver_id_2_0,1680433923638
    id_1,receiver_id_1_1,receiver_id_2_1,1680433923640
    receiver_id_0_2,id_2,receiver_id_2_2,1680433923641
    receiver_id_0_3,id_3,receiver_id_2_3,1680433923643
    receiver_id_0_4,receiver_id_1_4,id_4,1680433923644
    receiver_id_0_5,receiver_id_1_5,receiver_id_2_5,1680433923645
    receiver_id_0_6,receiver_id_1_6,receiver_id_2_6,1680433923647
    receiver_id_0_7,receiver_id_1_7,receiver_id_2_7,1680433923649
    receiver_id_0_8,receiver_id_1_8,receiver_id_2_8,1680433923650
    receiver_id_0_9,receiver_id_1_9,receiver_id_2_9,1680433923652
```

### 実行手順

1. 1つ目のターミナルで以下を実行（送信側）

```bash
cd ./ppam/build/example/scripts && bash sender_test.sh
```

2. 2つ目のターミナルで以下を実行（受信側）

```bash
cd ./ppam/build/example/scripts && bash receiver_test.sh
```
