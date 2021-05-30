

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
usermod -a -G $TORGROUP $LIGHTNINGUSER
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

### Add following to config

```bash
nano ~/.lightning/config

proxy=127.0.0.1:9050
bind-addr=127.0.0.1:9735
addr=statictor:127.0.0.1:9051
always-use-proxy=true
```

### After editing the /etc/tor/torrc file looks like this

```bash
ControlPort 9051
CookieAuthentication 1
CookieAuthFileGroupReadable 1

# This is just a test service, pointing to the local
# nginx web server. Not needed in final setup.
HiddenServiceDir /var/lib/tor/hidden_service/
HiddenServiceVersion 3
HiddenServicePort 80 127.0.0.1:80

# This is the lightningd service setup with tor hidden service
HiddenServiceDir /var/lib/tor/lightningd-service_v3/
HiddenServiceVersion 3
HiddenServicePort 1234 127.0.0.1:9735
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

# Checking the other files, user permissions of this hidden service's files
satinder@ubuntu:~$ sudo su
root@ubuntu:/home/satinder# cd /var/lib/tor/lightningd-service_v3/
root@ubuntu:/var/lib/tor/lightningd-service_v3# ls -lha
total 20K
drwx--S--- 2 debian-tor debian-tor 4.0K May 30 07:54 .
drwx--S--- 4 debian-tor debian-tor 4.0K May 30 08:55 ..
-rw------- 1 debian-tor debian-tor   63 May 30 07:54 hostname
-rw------- 1 debian-tor debian-tor   64 May 30 07:54 hs_ed25519_public_key
-rw------- 1 debian-tor debian-tor   96 May 30 07:54 hs_ed25519_secret_key
root@ubuntu:/var/lib/tor/lightningd-service_v3# cat hostname 
4rn3inojrpfnre7ewaizrcdeqopy6dcgjyso5nmqu76r72fa2xmchvqd.onion
root@ubuntu:/var/lib/tor/lightningd-service_v3# 
```


### At the end just start lightningd server like before

```bash
satinder@ubuntu:~/lightning$ ./lightningd/lightningd --log-level debug
2021-05-30T15:53:47.149Z DEBUG   plugin-manager: started(7062) /home/satinder/lightning/lightningd/../plugins/autoclean
2021-05-30T15:53:47.150Z DEBUG   plugin-manager: started(7063) /home/satinder/lightning/lightningd/../plugins/bcli
2021-05-30T15:53:47.159Z DEBUG   plugin-manager: started(7064) /home/satinder/lightning/lightningd/../plugins/fetchinvoice
2021-05-30T15:53:47.161Z DEBUG   plugin-manager: started(7065) /home/satinder/lightning/lightningd/../plugins/keysend
2021-05-30T15:53:47.164Z DEBUG   plugin-manager: started(7066) /home/satinder/lightning/lightningd/../plugins/offers
2021-05-30T15:53:47.165Z DEBUG   plugin-manager: started(7067) /home/satinder/lightning/lightningd/../plugins/pay
2021-05-30T15:53:47.167Z DEBUG   plugin-manager: started(7068) /home/satinder/lightning/lightningd/../plugins/txprepare
2021-05-30T15:53:47.177Z DEBUG   plugin-manager: started(7069) /home/satinder/lightning/lightningd/../plugins/spenderp
2021-05-30T15:53:47.205Z DEBUG   lightningd: testing /home/satinder/lightning/lightningd/lightning_channeld
2021-05-30T15:53:47.222Z DEBUG   lightningd: testing /home/satinder/lightning/lightningd/lightning_closingd
2021-05-30T15:53:47.242Z DEBUG   lightningd: testing /home/satinder/lightning/lightningd/lightning_connectd
2021-05-30T15:53:47.259Z DEBUG   lightningd: testing /home/satinder/lightning/lightningd/lightning_gossipd
2021-05-30T15:53:47.274Z DEBUG   lightningd: testing /home/satinder/lightning/lightningd/lightning_hsmd
2021-05-30T15:53:47.293Z DEBUG   lightningd: testing /home/satinder/lightning/lightningd/lightning_onchaind
2021-05-30T15:53:47.314Z DEBUG   lightningd: testing /home/satinder/lightning/lightningd/lightning_openingd
2021-05-30T15:53:47.323Z DEBUG   hsmd: pid 7077, msgfd 31
2021-05-30T15:53:47.358Z INFO    database: Creating database
2021-05-30T15:53:47.384Z DEBUG   connectd: pid 7078, msgfd 35
2021-05-30T15:53:47.384Z DEBUG   hsmd: Client: Received message 11 from client
2021-05-30T15:53:47.385Z UNUSUAL hsmd: HSM: created new hsm_secret file
2021-05-30T15:53:47.385Z DEBUG   hsmd: Client: Received message 9 from client
2021-05-30T15:53:47.385Z DEBUG   hsmd: new_client: 0
2021-05-30T15:53:47.497Z DEBUG   connectd: Created IPv6 listener on port 9735
2021-05-30T15:53:47.498Z DEBUG   connectd: Failed to connect 10 socket: Network is unreachable
2021-05-30T15:53:47.498Z DEBUG   connectd: Created IPv4 listener on port 9735
2021-05-30T15:53:47.498Z DEBUG   connectd: REPLY WIRE_CONNECTD_INIT_REPLY with 0 fds
2021-05-30T15:53:47.500Z DEBUG   gossipd: pid 7079, msgfd 34
topo 0
2021-05-30T15:53:47.510Z DEBUG   hsmd: Client: Received message 9 from client
2021-05-30T15:53:47.510Z DEBUG   hsmd: new_client: 0
2021-05-30T15:53:47.519Z DEBUG   gossipd: total store load time: 0 msec
2021-05-30T15:53:47.519Z DEBUG   gossipd: gossip_store: Read 0/0/0/0 cannounce/cupdate/nannounce/cdelete from store (0 deleted) in 1 bytes
2021-05-30T15:53:47.519Z DEBUG   gossipd: seeker: state = STARTING_UP New seeker
2021-05-30T15:53:47.554Z INFO    plugin-bcli: bitcoin-cli initialized and connected to bitcoind.
topo 1
2021-05-30T15:53:47.554Z DEBUG   lightningd: All Bitcoin plugin commands registered
topo 2
2021-05-30T15:53:47.605Z DEBUG   lightningd: Smoothed feerate estimate for opening initialized to polled estimate 12500
2021-05-30T15:53:47.605Z DEBUG   lightningd: Feerate estimate for opening set to 12500 (was 0)
2021-05-30T15:53:47.605Z DEBUG   lightningd: Smoothed feerate estimate for mutual_close initialized to polled estimate 12500
2021-05-30T15:53:47.605Z DEBUG   lightningd: Feerate estimate for mutual_close set to 12500 (was 0)
2021-05-30T15:53:47.605Z DEBUG   lightningd: Smoothed feerate estimate for unilateral_close initialized to polled estimate 12500
2021-05-30T15:53:47.605Z DEBUG   lightningd: Feerate estimate for unilateral_close set to 12500 (was 0)
2021-05-30T15:53:47.605Z DEBUG   lightningd: Smoothed feerate estimate for delayed_to_us initialized to polled estimate 12500
2021-05-30T15:53:47.605Z DEBUG   lightningd: Feerate estimate for delayed_to_us set to 12500 (was 0)
2021-05-30T15:53:47.605Z DEBUG   lightningd: Smoothed feerate estimate for htlc_resolution initialized to polled estimate 12500
2021-05-30T15:53:47.605Z DEBUG   lightningd: Feerate estimate for htlc_resolution set to 12500 (was 0)
2021-05-30T15:53:47.605Z DEBUG   lightningd: Smoothed feerate estimate for penalty initialized to polled estimate 12500
2021-05-30T15:53:47.605Z DEBUG   lightningd: Feerate estimate for penalty set to 12500 (was 0)
2021-05-30T15:53:47.605Z DEBUG   lightningd: Smoothed feerate estimate for min_acceptable initialized to polled estimate 6250
2021-05-30T15:53:47.605Z DEBUG   lightningd: Feerate estimate for min_acceptable set to 6250 (was 0)
2021-05-30T15:53:47.605Z DEBUG   lightningd: Smoothed feerate estimate for max_acceptable initialized to polled estimate 125000
2021-05-30T15:53:47.605Z DEBUG   lightningd: Feerate estimate for max_acceptable set to 125000 (was 0)
2021-05-30T15:53:53.188Z DEBUG   lightningd: Adding block 8073896: 0000002ea48cbcc40f9f25e6214eb3e75814bc6885c8f3d25b1120484a384738
2021-05-30T15:53:53.191Z DEBUG   wallet: Loaded 0 channels from DB
2021-05-30T15:53:53.192Z DEBUG   plugin-autoclean: autocleaning not active
2021-05-30T15:53:53.193Z DEBUG   connectd: REPLY WIRE_CONNECTD_ACTIVATE_REPLY with 0 fds
2021-05-30T15:53:53.193Z INFO    lightningd: --------------------------------------------------
2021-05-30T15:53:53.193Z INFO    lightningd: Server started with public key 03e45318266315810eaf9f53e4ecb4dd683ec3d274574e823ff5b81a7d9ab093f6, alias VIOLENTMASTER (color #03e453) and lightningd chipsln.0.0.0
2021-05-30T15:53:53.194Z DEBUG   plugin-fetchinvoice: Killing plugin: disabled itself at init: offers not enabled in config
2021-05-30T15:53:53.194Z DEBUG   plugin-offers: Killing plugin: disabled itself at init: offers not enabled in config
2021-05-30T15:53:53.208Z DEBUG   lightningd: Adding block 8073897: 0000004fc861d1825991004ab068997a3558a35eb4ac2d682be8817d1fec662b
2021-05-30T15:53:53.241Z DEBUG   lightningd: Adding block 8073898: 00000057afc4281572d706fca5b45bab5f0f4fb603a15238358cfc48cd60d4fc
2021-05-30T15:53:53.259Z DEBUG   lightningd: Adding block 8073899: 0000001898dc3f981278567090df6549acbbd948ad80e76eef305c6297e06adc
2021-05-30T15:53:53.277Z DEBUG   lightningd: Adding block 8073900: 0000006170b6a67b942cb29df219f26686b57ec6a7c2c924a0223915552cf739
2021-05-30T15:53:53.295Z DEBUG   lightningd: Adding block 8073901: 00000039714c93afcf3a360cfb2615dd3b32483f5a78b9a7a1b8200341b9dd59
2021-05-30T15:53:53.311Z DEBUG   lightningd: Adding block 8073902: 000000296a0c1b02fe740a2b8169bec8b4e808920196b5b013e9ef452ecf39d1
2021-05-30T15:53:53.326Z DEBUG   lightningd: Adding block 8073903: 000000535d8dc0f0c96855eb8d33c7e1e678c4c5c673e412b33e93c2c5a9efb9
2021-05-30T15:53:53.344Z DEBUG   lightningd: Adding block 8073904: 00000045351bdc0f7f8c1bfcba0e5b491fc3a4ee15904a8e35fc4542dfbb5a65
2021-05-30T15:53:53.366Z DEBUG   lightningd: Adding block 8073905: 0000000a35c18ac829c861c33465cac856e027852e17c090ce3f2b2648836a8d
2021-05-30T15:53:53.392Z DEBUG   lightningd: Adding block 8073906: 000000529a3baa1ccca2e8fa277208ca5438261558abfa44bd471954ff70550d
2021-05-30T15:53:53.418Z DEBUG   lightningd: Adding block 8073907: 000000317076339f99c8d2842871537fd7f8eac8fa8b1a7b36ee91fee2cdb63f
2021-05-30T15:53:53.441Z DEBUG   lightningd: Adding block 8073908: 0000002df76b8907f13830ce9235226fe4fb164b778659f1ec6e7c38bbdbd266
2021-05-30T15:53:53.465Z DEBUG   lightningd: Adding block 8073909: 0000001ea8dc7d7f75f9207068c38376a5a2fa33bfea113c1c5e324fa598983d
2021-05-30T15:53:53.486Z DEBUG   lightningd: Adding block 8073910: 000000114fd537e91a89433d2fd29c2ca988ea306fe8a5ad3ebc25c9b10ed60b
2021-05-30T15:53:53.510Z DEBUG   lightningd: Adding block 8073911: 0000001a33e8d082bb8b30e99e4b4f75013df3475b6bfb98c768af86363d0ded
2021-05-30T15:54:23.565Z DEBUG   lightningd: Adding block 8073912: 0000005267dc5e51f603f1fb4894a4e4a94ce78f80e7361df0feedfe4eb04200
2021-05-30T15:54:47.542Z DEBUG   gossipd: seeker: no peers, waiting
```
