# microvpn

シンプルなネットワーク学習用VPNです。もちろん実用向けではありません。

- 認証、暗号化なし
- クライアントはひとつだけ
- IPv4

# 準備

## サーバ

* ルーティング

```sh
# (1) 仮想デバイスにIPアドレスを割り当てる
ip addr add 11.8.0.1 dev tunserver
# (2) 仮想デバイスをリンクアップさせる
ip link set tunserver up
# (3) VPN内宛のパケットを仮想デバイスに向ける
ip route add 11.8.0.0/24 dev tunserver
```

* カーネルパラメータでIPフォワードを許可

```sh
# /etc/sysctl.conf
# ルーティングの(3)に対応
net.ipv4.ip_forward=1
```

* ファイアウォール(iptables)

```sh
# VPNから出ていく通信についてIPマスカレードを適用する
sudo iptables -t nat -A POSTROUTING -s 11.8.0.0/24 -o eth0 -j MASQUERADE
# VPNサーバへ入るUDPパケットを許可する
sudo iptables -A INPUT -p udp --dport 1195 -j ACCEPT
# インターネットからVPNに入る通信が仮想デバイスに行くのを許可する
# ルーティングの(3)に対応
sudo iptables -A FORWARD -i eth0 -o tunserver -d 11.8.0.0/24 -j ACCEPT
```


## クライアント

* ルーティング

```sh
# (1) 仮想デバイスにIPアドレスを割り当てる
sudo ip addr add 11.8.0.2 dev tunclient
# (2) 仮想デバイスをリンクアップさせる
sudo ip link set tunclient up
# (3) VPNサーバへは直接接続するようにする
sudo ip route add 7.256.22.2 via 192.168.0.1 dev eno1
# (4) すべての宛先への通信を仮想デバイスに向ける
sudo ip route add 0.0.0.0/0 dev tunclient
```

* カーネルパラメータでIPフォワードを許可

```sh
# /etc/sysctl.conf
net.ipv4.ip_forward=1
```

* iptablesはデフォルトのまま

# 使い方

環境変数にIPアドレスを設定

```sh
export CLIENT_IPADDR="3.256.8.1"
export SERVER_IPADDR="7.256.22.2"
```

## サーバ

```
gcc -g util.c server.c -o server.out
sudo -E ./server.out
```

## クライアント

```
gcc -g util.c client.c -o client.out
sudo -E ./client.out
```

## 接続テスト

```sh
sudo traceroute -i tunclient 1.1.1.1 -4 -I -n
```

sudo ip route add ::/0 dev tunclient