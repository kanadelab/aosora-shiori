<div style="display:flex">
    <img src="assets/aosora-icon.svg" width="64">
    <div style="margin-left: 10px">
        <h2 style="display:inline">蒼空 / aosora-shiori</h2><br>
        しおりをつくってみようプロジェクト
    </div>
</div>

## これはなに？
SHIORIを作ってみるプロジェクトです。  
デモゴーストが起動できるくらいには、動きます。

[伺か・伺的 Advent Calendar 2024](https://adventar.org/calendars/10049) のネタとして作ってみました。  
* [わたしのかんがえたさいきょうのしおり ](https://note.com/kanade_lab/n/n7925c86d94eb)

## ためしてみる
ReleaseのところからデモゴーストやDLLをダウンロードできます。
VSCode向けのちょっとした拡張機能もあります。

仕様を書き出したガイドなどもありますので、興味があれば見てみてください。  
* チュートリアル: [蒼空ゴースト開発入門](https://github.com/kanadelab/aosora-shiori/wiki/%E8%92%BC%E7%A9%BA%E3%82%B4%E3%83%BC%E3%82%B9%E3%83%88%E9%96%8B%E7%99%BA%E5%85%A5%E9%96%80)
* [プログラミングガイド](https://github.com/kanadelab/aosora-shiori/wiki/%E3%83%97%E3%83%AD%E3%82%B0%E3%83%A9%E3%83%9F%E3%83%B3%E3%82%B0%E3%82%AC%E3%82%A4%E3%83%89)

## 利用規約など
配布している aosora.dll は伺かゴースト向けのSHIORI・SAORIとして自由に使用・配布いただけます。
ソースファイルのライセンスはまだ定めていないので、個別に許可した場合を除いては個人利用や本プロジェクト参加のための使用にとどめてください。
このプログラムは無保証です。このプログラムによって発生した現象について作者は責任を負いません。

## ビルドにかんして
* リポジトリ内のゴースト(/ssp/*)を起動する場合
  * aosora.dll のビルドが必要
* vscode拡張(/vscode-extension/*)をビルドする場合
  * aosora-sstp.exe のビルドが必要

先にaosora-shiori.slnをビルドしてください。  
ビルド生成物として、それぞれに必要なファイルが配置されます。

## そのほか
* 月波 清火 (@tukinami_seika@fedibird.com) 様がアイコンを作ってくださいました。

* jsoncpp (https://github.com/open-source-parsers/jsoncpp) をお借りしました。パブリックドメインのjsonシリアライザです。