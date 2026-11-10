# 共通CMake設定

複数ターゲットで共有する依存物の接続、警告、コンパイル設定を置く。

現在のC++20設定と構成確認はルートの[CMakeLists.txt](../CMakeLists.txt)にある。共通処理が実際に生じた時点で、このディレクトリへ抽出する。旧WindowsのCMakeは[独立した旧作業ルート](../legacy/windows/CMakeLists.txt)にある。
