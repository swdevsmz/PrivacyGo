#!/bin/bash



echo "NASM をインストール中..."
sudo apt update
sudo apt install -y nasm
echo "NASM のインストールが完了しました。"

# cmakeがインストールされていない場合はインストール
if ! command -v cmake &> /dev/null; then
  echo "cmake が見つかりません。インストールします。"
  sudo apt install -y cmake
  echo "cmake のインストールが完了しました。"
else
  echo "cmake は既にインストールされています。"
fi



# このスクリプトが存在するディレクトリをワークスペースのルートとして設定
WORKSPACE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"


echo "IPCL のビルドとインストール..."
IPCL_DIR="$WORKSPACE_ROOT/ipcl"
IPCL_INSTALL_DIR="$WORKSPACE_ROOT/ipcl_install"

# IPCL ディレクトリが存在しない場合はクローン
if [ ! -d "$IPCL_DIR" ]; then
  git clone https://github.com/intel/pailliercryptolib.git "$IPCL_DIR"
fi

# IPCL のインストール先ディレクトリを作成
mkdir -p "$IPCL_INSTALL_DIR"

# IPCL のビルドとインストール
cmake -B "$IPCL_DIR"/build -S "$IPCL_DIR" -DCMAKE_INSTALL_PREFIX="$IPCL_INSTALL_DIR" -DCMAKE_BUILD_TYPE=Release -DIPCL_TEST=OFF -DIPCL_BENCHMARK=OFF
cmake --build "$IPCL_DIR"/build -j 4
cmake --build "$IPCL_DIR"/build --target install
echo "IPCL のビルドとインストールが完了しました。"



echo "JSON for Modern C++ のビルド..."
JSON_DIR="$WORKSPACE_ROOT/json"

# JSON ディレクトリが存在しない場合はクローン
if [ ! -d "$JSON_DIR" ]; then
  git clone https://github.com/nlohmann/json.git "$JSON_DIR"
fi

# JSON のビルド
cmake -B "$JSON_DIR"/build -S "$JSON_DIR"
cmake --build "$JSON_DIR"/build -j 4
echo "JSON for Modern C++ のビルドが完了しました。"

# DPCA-PSI
echo "DPCA-PSI のビルド..."
DPCA_PSI_DIR="$WORKSPACE_ROOT/dpca-psi"
cmake -B "$DPCA_PSI_DIR"/build -S "$DPCA_PSI_DIR" -DCMAKE_BUILD_TYPE=Release -DIPCL_DIR="$IPCL_INSTALL_DIR"/lib/cmake/ipcl-2.0.0  -Dnlohmann_json_DIR="$JSON_DIR"/build
cmake --build "$DPCA_PSI_DIR"/build -j 4
echo "DPCA-PSI のビルドが完了しました。"

# PPAM
echo "PPAM のビルド..."
PPAM_DIR="$WORKSPACE_ROOT/ppam"
cmake -B "$PPAM_DIR"/build -S "$PPAM_DIR" -DCMAKE_BUILD_TYPE=Release -DIPCL_DIR="$IPCL_INSTALL_DIR"/lib/cmake/ipcl-2.0.0  -Dnlohmann_json_DIR="$JSON_DIR"/build
cmake --build "$PPAM_DIR"/build -j 4
echo "PPAM のビルドが完了しました。"