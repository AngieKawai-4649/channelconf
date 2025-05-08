# Linux TVチャンネル管理アプリケーション
チャンネル定義ファイルを作成し最新状態に維持管理することで  
当ファイルからMirakurun channels.yml、vlcプレイリストを作成することが可能  
また、ここで配布しているLinux用チューナーアプリケーション  
(recfsusb2n recsanpakun recdvb recpt1)のチャンネル情報として使用  

※トランスポンダ移動等があった時に当ファイルをメンテナンスするだけで  
Linux用チューナーアプリケーションプログラムを直すことなく運用できる  

## 構成要素
- channel.cnf       : TVチャンネル定義ファイル
- libchannelcnf.so  : Linux用チューナーアプリケーションにリンクする共有ライブラリ
- chtool            : チャンネル定義ファイルを管理するツール
- semtool           : Linux TVチャンネル管理アプリケーションで使用するsemaphoreを管理するツール

## 使用方法
1. channel.cnfを定義編集する
2. chtoolでチャンネル情報を共有メモリに割り付ける  
   (chtoolで割り付けなくても(recfsusb2n recsanpakun recdvb recpt1)の初期起動で自動的に割り付けるが  
   パフォーマンスを考慮すればchtoolで予め割り付けた方が良い)  
4. チャンネル情報変更時はchtoolを使用しMirakurun channels.ymlを生成し所定のディレクトリに配置する

chtool [option]  
-c : channel.cnfを読み込み共有メモリを割り付けデータを展開する  
      既に割り付けられていてchannel.cnfの内容とサイズが異なる場合  
      削除して再割り当てする  
-d : 共有メモリを削除する  
-m : 共有メモリからMirakurun channels.yml フォーマットで出力する  
-p : 共有メモリからVLC(SMPLAYER)プレイリストフォーマットで出力する  
-v : 共有メモリ内容を表示する  
-h : 使い方を表示する  

semtool [option]  
-c : セマフォを割り当てる  
-d : セマフォを削除する  
-u : セマフォUNLOCKする  
-h : 使い方を表示する  



