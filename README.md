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

PPAMのサンプルプログラムは、送信側（sender）と受信側（receiver）をそれぞれ別々のターミナルで実行する必要があります。

1. 1つ目のターミナルで以下を実行（送信側）

```bash
cd ./ppam/build/example/scripts && bash sender_test.sh
```

2. 2つ目のターミナルで以下を実行（受信側）

```bash
cd ./ppam/build/example/scripts && bash receiver_test.sh
```
