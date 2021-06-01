

- https://lightning.readthedocs.io/TOR.html
- https://2019.www.torproject.org/docs/tor-onion-service.html.en
- https://github.com/satindergrewal/lightning/blob/grewal/doc/TOR.md


### Install tor with

```bash
sudo apt update
sudo apt install tor
```

### Edit Tor's config file

```bash
sudo nano /etc/tor/torrc

# Uncomment following lines
ExitPolicy reject *:* # no exits allowed

ControlPort 9051
CookieAuthentication 1

# Also add this
CookieAuthFileGroupReadable 1
```

### Add user which will be running lightingd, to tor service's user group

```bash
# Make environment variables to temporarily store the user groups info

export TORGROUP=$(stat -c '%G' /run/tor/control.authcookie)
export LIGHTNINGUSER=$(whoami)

# Add tor service's user group to lightingd running user's group (i.e debian-tor).
# In our case the same user logged in currently (i.e. satinder) will be running lightningd.
# So, adding adding satinder to debian-tor group.
sudo usermod -a -G $TORGROUP $LIGHTNINGUSER

# Or you can also use this command directly
sudo usermod -a -G debian-tor $(whoami)
```

### Logout/Login, or Restart
`groups` command should return torgroup in result after restart

```bash
satinder@ubuntu:~$ groups
satinder adm cdrom sudo dip plugdev lpadmin sambashare debian-tor

# Following command should just return nothing. If error, setup is not done correct
satinder@ubuntu:~$ cat /run/tor/control.authcookie > /dev/null
satinder@ubuntu:~$
```

### After editing the /etc/tor/torrc file looks like this

```bash
ControlPort 9051
CookieAuthentication 1
CookieAuthFileGroupReadable 1

# This is the lightningd service setup with tor hidden service
HiddenServiceDir /var/lib/tor/lightningd-service_v3/
HiddenServiceVersion 3
HiddenServicePort 9735 127.0.0.1:9735
```

### Restart Tor service and check the onion v3 address

```bash
# Restart Tor service
satinder@ubuntu:~$ sudo systemctl restart tor

# Check if Tor service is running fine
satinder@ubuntu:~$ sudo systemctl status tor
● tor.service - Anonymizing overlay network for TCP (multi-instance-master)
   Loaded: loaded (/lib/systemd/system/tor.service; enabled; vendor preset: enabled)
   Active: active (exited) since Sun 2021-05-30 07:54:51 PDT; 1h 20min ago
  Process: 6880 ExecStart=/bin/true (code=exited, status=0/SUCCESS)
 Main PID: 6880 (code=exited, status=0/SUCCESS)

May 30 07:54:51 ubuntu systemd[1]: Stopped Anonymizing overlay network for TCP (multi-instance-master).
May 30 07:54:51 ubuntu systemd[1]: Stopping Anonymizing overlay network for TCP (multi-instance-master)...
May 30 07:54:51 ubuntu systemd[1]: Starting Anonymizing overlay network for TCP (multi-instance-master)...
May 30 07:54:51 ubuntu systemd[1]: Started Anonymizing overlay network for TCP (multi-instance-master).

# Check the newly generated onion v3 address for lightningd service
satinder@ubuntu:~$ sudo cat /var/lib/tor/lightningd-service_v3/hostname
4rn3inojrpfnre7ewaizrcdeqopy6dcgjyso5nmqu76r72fa2xmchvqd.onion 
```

### Add following to config
Get your `.onion` address as shown earlier and use that in the lightning config file.

```bash
nano ~/.lightning/config

proxy=127.0.0.1:9050
bind-addr=127.0.0.1:9735
addr=4rn3inojrpfnre7ewaizrcdeqopy6dcgjyso5nmqu76r72fa2xmchvqd.onion:9735
always-use-proxy=true
```

### At the end just start lightningd server like before

```bash
satinder@ubuntu:~/lightning$ ./lightningd/lightningd --log-level debug --conf=$HOME/.lightning/config
2021-06-01T19:39:41.276Z DEBUG   plugin-manager: started(36316) /home/satinder/lightning/lightningd/../plugins/autoclean
2021-06-01T19:39:41.278Z DEBUG   plugin-manager: started(36317) /home/satinder/lightning/lightningd/../plugins/bcli
2021-06-01T19:39:41.279Z DEBUG   plugin-manager: started(36318) /home/satinder/lightning/lightningd/../plugins/fetchinvoice
2021-06-01T19:39:41.280Z DEBUG   plugin-manager: started(36319) /home/satinder/lightning/lightningd/../plugins/keysend
2021-06-01T19:39:41.290Z DEBUG   plugin-manager: started(36320) /home/satinder/lightning/lightningd/../plugins/offers
2021-06-01T19:39:41.293Z DEBUG   plugin-manager: started(36321) /home/satinder/lightning/lightningd/../plugins/pay
2021-06-01T19:39:41.294Z DEBUG   plugin-manager: started(36322) /home/satinder/lightning/lightningd/../plugins/txprepare
2021-06-01T19:39:41.296Z DEBUG   plugin-manager: started(36323) /home/satinder/lightning/lightningd/../plugins/spenderp
2021-06-01T19:39:41.311Z UNUSUAL lightningd: You used `--addr=4rn3inojrpfnre7ewaizrcdeqopy6dcgjyso5nmqu76r72fa2xmchvqd.onion:9735` option with an .onion address, please use `--announce-addr` ! You are lucky in this node live some wizards and fairies, we have done this for you and announce, Be as hidden as wished
2021-06-01T19:39:41.312Z DEBUG   lightningd: testing /home/satinder/lightning/lightningd/lightning_channeld
2021-06-01T19:39:41.315Z DEBUG   lightningd: testing /home/satinder/lightning/lightningd/lightning_closingd
2021-06-01T19:39:41.316Z DEBUG   lightningd: testing /home/satinder/lightning/lightningd/lightning_connectd
2021-06-01T19:39:41.318Z DEBUG   lightningd: testing /home/satinder/lightning/lightningd/lightning_gossipd
2021-06-01T19:39:41.320Z DEBUG   lightningd: testing /home/satinder/lightning/lightningd/lightning_hsmd
2021-06-01T19:39:41.323Z DEBUG   lightningd: testing /home/satinder/lightning/lightningd/lightning_onchaind
2021-06-01T19:39:41.325Z DEBUG   lightningd: testing /home/satinder/lightning/lightningd/lightning_openingd
2021-06-01T19:39:41.327Z DEBUG   hsmd: pid 36331, msgfd 31
2021-06-01T19:39:41.343Z DEBUG   connectd: pid 36332, msgfd 35
2021-06-01T19:39:41.343Z DEBUG   hsmd: Client: Received message 11 from client
2021-06-01T19:39:41.343Z DEBUG   hsmd: Client: Received message 9 from client
2021-06-01T19:39:41.343Z DEBUG   hsmd: new_client: 0
2021-06-01T19:39:41.353Z DEBUG   connectd: Proxy address: 127.0.0.1:9050
2021-06-01T19:39:41.353Z DEBUG   connectd: Created IPv4 listener on port 9735
2021-06-01T19:39:41.353Z DEBUG   connectd: REPLY WIRE_CONNECTD_INIT_REPLY with 0 fds
2021-06-01T19:39:41.353Z DEBUG   gossipd: pid 36333, msgfd 34
topo 0
2021-06-01T19:39:41.358Z DEBUG   hsmd: Client: Received message 9 from client
2021-06-01T19:39:41.358Z DEBUG   hsmd: new_client: 0
2021-06-01T19:39:41.364Z INFO    plugin-bcli: bitcoin-cli initialized and connected to bitcoind.
topo 1
2021-06-01T19:39:41.364Z DEBUG   lightningd: All Bitcoin plugin commands registered
topo 2
2021-06-01T19:39:41.365Z DEBUG   gossipd: gossip_store_compact_offline: 0 deleted, 0 copied
2021-06-01T19:39:41.365Z DEBUG   gossipd: total store load time: 0 msec
2021-06-01T19:39:41.365Z DEBUG   gossipd: gossip_store: Read 0/0/0/0 cannounce/cupdate/nannounce/cdelete from store (0 deleted) in 1 bytes
2021-06-01T19:39:41.365Z DEBUG   gossipd: seeker: state = STARTING_UP New seeker
2021-06-01T19:39:41.393Z DEBUG   lightningd: Smoothed feerate estimate for opening initialized to polled estimate 12500
2021-06-01T19:39:41.393Z DEBUG   lightningd: Feerate estimate for opening set to 12500 (was 0)
2021-06-01T19:39:41.393Z DEBUG   lightningd: Smoothed feerate estimate for mutual_close initialized to polled estimate 12500
2021-06-01T19:39:41.393Z DEBUG   lightningd: Feerate estimate for mutual_close set to 12500 (was 0)
2021-06-01T19:39:41.393Z DEBUG   lightningd: Smoothed feerate estimate for unilateral_close initialized to polled estimate 12500
2021-06-01T19:39:41.393Z DEBUG   lightningd: Feerate estimate for unilateral_close set to 12500 (was 0)
2021-06-01T19:39:41.393Z DEBUG   lightningd: Smoothed feerate estimate for delayed_to_us initialized to polled estimate 12500
2021-06-01T19:39:41.393Z DEBUG   lightningd: Feerate estimate for delayed_to_us set to 12500 (was 0)
2021-06-01T19:39:41.393Z DEBUG   lightningd: Smoothed feerate estimate for htlc_resolution initialized to polled estimate 12500
2021-06-01T19:39:41.393Z DEBUG   lightningd: Feerate estimate for htlc_resolution set to 12500 (was 0)
2021-06-01T19:39:41.393Z DEBUG   lightningd: Smoothed feerate estimate for penalty initialized to polled estimate 12500
2021-06-01T19:39:41.393Z DEBUG   lightningd: Feerate estimate for penalty set to 12500 (was 0)
2021-06-01T19:39:41.393Z DEBUG   lightningd: Smoothed feerate estimate for min_acceptable initialized to polled estimate 6250
2021-06-01T19:39:41.393Z DEBUG   lightningd: Feerate estimate for min_acceptable set to 6250 (was 0)
2021-06-01T19:39:41.393Z DEBUG   lightningd: Smoothed feerate estimate for max_acceptable initialized to polled estimate 125000
2021-06-01T19:39:41.393Z DEBUG   lightningd: Feerate estimate for max_acceptable set to 125000 (was 0)
2021-06-01T19:39:41.436Z DEBUG   lightningd: Adding block 8090742: 000000506a7e4df9d0d9ef2c89b5d38bf0a65613f0333a989837019cd1f64a48
2021-06-01T19:39:41.438Z DEBUG   wallet: Loaded 0 channels from DB
2021-06-01T19:39:41.439Z DEBUG   plugin-autoclean: autocleaning not active
2021-06-01T19:39:41.439Z DEBUG   connectd: REPLY WIRE_CONNECTD_ACTIVATE_REPLY with 0 fds
2021-06-01T19:39:41.439Z INFO    lightningd: --------------------------------------------------
2021-06-01T19:39:41.439Z INFO    lightningd: Server started with public key 03e45318266315810eaf9f53e4ecb4dd683ec3d274574e823ff5b81a7d9ab093f6, alias VIOLENTMASTER (color #03e453) and lightningd chipsln.0.0.0
2021-06-01T19:39:41.440Z DEBUG   plugin-fetchinvoice: Killing plugin: disabled itself at init: offers not enabled in config
2021-06-01T19:39:41.440Z DEBUG   plugin-offers: Killing plugin: disabled itself at init: offers not enabled in config
2021-06-01T19:39:41.452Z DEBUG   lightningd: Adding block 8090743: 0000003e4cca5e26fe0d0ce842aa0d3640996c32429afbabe205932667acc015
2021-06-01T19:39:41.466Z DEBUG   lightningd: Adding block 8090744: 00000029ea741b67a84d2524ee6c5bf476776941aea8a378a87af6573cb430da
2021-06-01T19:39:41.480Z DEBUG   lightningd: Adding block 8090745: 0000000b95db2ffd75816f6bc57ced974fe2379295e6dd062f126fb0291e1534
2021-06-01T19:39:41.493Z DEBUG   lightningd: Adding block 8090746: 00000020a33e7f8cf3ab29acddb5505de5c5304671178950240e5292c25efb4a
2021-06-01T19:39:41.506Z DEBUG   lightningd: Adding block 8090747: 0000000da7f4c278e5f2fea59f7f8874564e66a9a1925a4d55fe3d9b22cc8370
2021-06-01T19:39:41.517Z DEBUG   lightningd: Adding block 8090748: 00000058aaac43a81193cfc121d7b2e52a2eaf55548c2907ded567ef3aade53b
2021-06-01T19:39:41.530Z DEBUG   lightningd: Adding block 8090749: 00000034c6e38829de3a4c14338e49cd1193a1f5673ee461210ec4ff4980ecf5
2021-06-01T19:39:41.541Z DEBUG   lightningd: Adding block 8090750: 000000048c43bbe0efba94c9834b1208ad2c8fb08fb59cfaf17aebb2fdd06802
2021-05-30T15:54:47.542Z DEBUG   gossipd: seeker: no peers, waiting
```
