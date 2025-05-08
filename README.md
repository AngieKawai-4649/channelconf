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

### chtool [option]
-c : channel.cnfを読み込み共有メモリを割り付けデータを展開する  
　　既に割り付けられていてchannel.cnfの内容とサイズが異なる場合  
　　削除して再割り当てする  
-d : 共有メモリを削除する  
-m : 共有メモリからMirakurun channels.yml フォーマットで出力する  
-p : 共有メモリからVLC(SMPLAYER)プレイリストフォーマットで出力する  
-v : 共有メモリ内容を表示する  
-h : 使い方を表示する  

### semtool [option]  
-c : セマフォを割り当てる  
-d : セマフォを削除する  
-u : セマフォUNLOCKする  
-h : 使い方を表示する  

## 環境変数
CHANNELCONFPATH : channel.cnfの場所をフルパスで定義する  未定義時はchtoolと同じディレクトリにchannel.cnfを置く  
CHANNELCONFKEY  : IPC 共有メモリ、セマフォのキーを指定する 未定義時は 0x4649  
例  
export CHANNELCONFPATH=/opt/TV_app/config  
export CHANNELCONFKEY=0x777777  

## Docker
Dockerで運用時、ゲストOS（コンテナ）の共有メモリ、セマフォを使用するとコンテナにログインしてchtoolを実行することになり不便なので  
ホストOSの共有メモリ、セマフォを使用するようにコンテナを作成することを推奨  

## docker-compose.yml 例
    services:
        mirakurun:
            image: mirakurun:3.9.0-rc.4.mod
            container_name: Mirakurun
            cap_add:
                - SYS_ADMIN
                - SYS_NICE
            ports:
                - "40772:40772"
                - "9229:9229"
            volumes:
                - ../Mirakurun/bind/run/:/var/run/
                - ../Mirakurun/bind/opt/:/opt/
                - ../Mirakurun/bind/config/:/app-config/
                - ../Mirakurun/bind/data/:/app-data/
            environment:
                TZ: "Asia/Tokyo"
            devices:
                - /dev/bus:/dev/bus
                - /dev/FSUSB2N_1:/dev/FSUSB2N_1
                - /dev/FSUSB2N_2:/dev/FSUSB2N_2
                - /dev/SANPAKUN_1:/dev/SANPAKUN_1
                - /dev/SANPAKUN_2:/dev/SANPAKUN_2
            restart: always
            logging:
                driver: json-file
                options:
                    max-file: "1"
                    max-size: 10m
            ipc: host       <--------これを入れる
## ビルド
$ git clone https://github.com/AngieKawai-4649/channelconf.git  
$ cd channelconf  
$ make  
$ sudo make install  

その後、recfsusb2n recsanpakun recdvb recpt1をリビルドする  

