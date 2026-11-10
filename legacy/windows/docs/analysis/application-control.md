# 本体の起動拒否と必要な対処

2026-10-02（日本時間）。設定変更・生成物の再起動をせず、レジストリ、Authenticode、CodeIntegrityイベントを読み取った。

証拠：`work/analysis/application-control/product-policy-20261002.json`。`VerifiedAndReputablePolicyState=1`（Smart App Controlのenforcement）。検査時の本体 `2b04679d0b5fe08ce500fdad0e20e737f9d40ca517cb7aaa8f4895eed1765d7c` はAuthenticode `NotSigned`。22:21:59 JSTのイベント3033/3077は最初の本体 `product-snapshot/20261002T132125316Z/install/bin/Producer.exe` の署名要件不適合を記録し、policy IDは `0283ac0f-fff1-49ae-ada1-8a933130cad6`。検査した本体と起動を拒否された本体は別hashである。

Windows標準CiToolのpolicy一覧読取りは `OperationResult=-2147024891`（0x80070005/アクセス拒否）で、権限を変えた読取りでも一覧を取得できなかった。ログ：`policies-20261002.json` / `policies-20261002-authorized-read.json`。追加の組織policyの有無は未確認であり、SACだけが有効と断定しない。同じ条件で読取りを繰り返さない。

[MicrosoftのFAQ](https://support.microsoft.com/en-us/windows/security/threat-malware-protection/smart-app-control-frequently-asked-questions)は、Smart App Controlで個別appを一時許可する方法はなく、開発者に有効なコード署名を案内している。[Microsoftの署名手順](https://learn.microsoft.com/en-us/windows/apps/develop/smart-app-control/code-signing-for-smart-app-control)では、信頼された発行元のRSA証明書を用いる。任意の自己署名を作るだけではSACの対応にならない。

ユーザーにログ取得を求める必要はない。Windowsセキュリティ→アプリとブラウザーコントロール→Smart App Controlで「オン」を確認できるが、現時点でこの値を変更する必要はない。署名証明書・署名サービスの利用には本人/組織の手続きや費用が関係する場合があるため、購入、外部送信、証明書導入・署名はまだ行わない。

この時点では個人管理PCか会社/学校管理PCかを確認中だった。後にユーザーが個人管理PCと回答し、自身でSACをオフにしたと明示した。下記の環境変更後の実行記録へ進んだ。署名手段の購入や他人への問い合わせ送信は行っていない。

エージェントは場所・実行ユーザー・loaderの変更で同じ拒否を再試行せず、SACの設定も変更しなかった。ユーザー本人の明示した設定変更後、read-onlyで新しい環境状態を確かめ、通常の本体起動を実施した。

署名を導入する場合は、署名前/後の両hash、署名者、timestamp、検証結果、使用サービス・手順をビルド記録と結び付ける。ビルド成功、署名適合、native実行、UI・音声を含む本体受入はそれぞれ別に判定する。署名だけで全体完成にはしない。実装・コンパイルなど依存しない作業は継続する。

## ユーザーによる環境変更後

ユーザー回答は「自分で管理する個人PC」、続いて「Smart App Controlをオフにしました」。2026-10-02T13:58:02Zにregistryの同じ値が0であることを読み取った。要約 `work/analysis/application-control/user-environment-change-20261002.json`。エージェントによる設定変更ではなく、現在の承認された環境を再確認した記録である。

新しい保存版135712877Zでは通常のProducer.exe --smokeとcore57件が実行でき、修正版142100453Zではcore58件とheadless module由来照合が成功した。証拠 `work/acceptance/product/20261002T135817271Z/run.json`、`20261002T142216083Z/run.json`。旧版の拒否は別hash/別環境条件の履歴として残す。現在、署名・許可環境の指定に関するユーザー回答待ちはない。追加policy全一覧は未取得のままで、全policyが無効という判断はしない。
