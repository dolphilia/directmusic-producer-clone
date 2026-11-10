# 外部依存

採用スタックのJUCEは、実装開始時に`third_party/JUCE/`へGit submoduleとして追加し、commitを固定する。現在はJUCEを取得しておらず、`.gitmodules`も未作成。

他の依存も採用版・取得方法・利用条件を記録する。同じ依存を複数の方法で取得したり、各モジュールへ別コピーを置いたりしない。ライセンスの方針と技術選定は[採用記録](../docs/decisions/0001-technology-stack.md)を参照する。
