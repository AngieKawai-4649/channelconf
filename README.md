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
