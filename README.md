# ~~kittyhook-tf2~~ tfkitty!

~~barely~~ mostly functional poc that makes the dedicated server ""compatible"" with the x360 version of tf2

## how to use 

### for hosters and pc players

- grab the build of tf2 from [here](https://gmod9.com/~bt/xboxtf.7z) (if the link dies please reup it somewhere and post in issues or something kthx)

- [build the project](#plugin) or grab the files from the release (assuming i will put one up)

- if you did everything correctly the dedicated server and client now should have `(kittyhook)` in the window title

- host a server or connect to one

#### so um, can we run plugins now?

indeed, we can! you can find my hl2sdk and sm fork made compatible for this build [here](https://github.com/eepycats/hl2sdk-x360) and [here](https://github.com/eepycats/sourcemod-x360)
![smrunning](https://github.com/user-attachments/assets/d5dab0f5-bb09-4548-aa47-4e769a3c437b)

### for console players

- you'll need a **xex copy** of the orange box **title update 5** for each of the options below

#### i have a devkit

- grab the `default.xex` from [patched_bins](https://github.com/eepycats/tfkitty/tree/main/patched_bins) (or find another way of passing in `-xnet_bypass_security`)

- replace your `default.xex` with the one from the repo (i stole it from a debug build of postal 3 and modified it :steamhappy:)


#### i have a rgh/jtag

- grab the `default.xex` from [patched_bins](https://github.com/eepycats/tfkitty/tree/main/patched_bins) (or find another way of passing in `-xnet_bypass_security`)

- replace your `default.xex` with the one from the repo (i stole it from a debug build of postal 3 and modified it :steamhappy:)

- grab `engine_360.dll` from [patched_bins/bin](https://github.com/eepycats/tfkitty/tree/main/patched_bins/bin) and replace it (for some reason udp networking just doesn't work unless you set some undocumented socket option)

- replace your `engine_360.dll` with the one from this repo

##### i already have a patched engine_360 and i want to manually patch mine
here are the patches i used for the binary (please remember to decrypt and decompress before attempting to patch it)

```
va (file offset) | original -> patched
0x86112500 (0x114500) | 38 8B 23 18 -> 38 8B 1C 88
0x86112534 (0x114534) | 38 A0 00 04 -> 38 A0 58 01
```

#### xenia

- for xenia you might be able to get away without patching anything, as far as i know it should just work if you pass `-xnet_bypass_security` in the config (`cl =` field)

#### connecting to the server

so this is the tricky part i guess, as far as i know theres no way to execute console commands other than binding stuff which is,,, not ideal to be honest

i added my litte tool i wrote for executing console commands (and dumping sendtables) remotely which should work on devkits and retail systems (assuming you have the xdbm and xdrpc DL plugins installed) in the [kitty/ directory](https://github.com/eepycats/tfkitty/tree/main/kitty). i know the code quality sucks this is my first time doing anything c# :c

the usage is simple, pass the console ip through command line or set the console as default in neighborhood after that it should connect automatically and then it should print `connected to: (your xbox name)` and you should be able to execute console commands

## building

### plugin

- get xmake (i LOVE lua!!)

- get vs2022 build tools or install visual studio 2022 (cxx23)

- run `xmake`

- copy `tf` from `dist` to the root of your game directory so `addons` ends up in the same directory as `gameinfo.txt`, `maps`, etc


### c# tool

- just load the solution in visual studio 2022 and build it

## bugs

* putting this up mostly as a poc, tho its a pretty functional one

* there's quite a bit of crashes in different places, notably gameui and shaderapi, not sure what causes those will probably need a bit of debugging

* there's some weird condition that just breaks the server for all xbox players, not sure what it is. that also will have to be solved one day

* feel free to contribute fixes and patches (tyvm!!) :3

## credits

- Dr. (i think) - released the postal 3 build i stole the xex from

- wanderer - helped with entity networking stuff

- weezer - spent hours debugging with me with his consoles, reintalled the sdk probably like 10 times too

- dashlaunch (i stole the undocumented flag from its code)

- the SourceMod and MetaMod: Source project by AlliedModders (the sendtable dumper is pretty much a rewrite of theirs)

- all the nice people who supported me while making this! big thanks!!

![itworks](https://github.com/user-attachments/assets/b38d8f5f-d519-4583-b5b6-1a6a08d42c96)

<sup><sub>🐈</sub></sup>
