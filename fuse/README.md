# Mike's Super-Dope Encrypted File System
...or just `MS-DEFS` for short. Pretty much `MS-DOS` but cooler.

To run this amazing file system, simply run the following commands in your terminal!
```bash
$ make
$ mkdir test_uno
$ mkdir test_dos
$ ./pa5-encfs your_Amaz!ng_pa55w0rd test_uno/ test_dos/
```

Congrats! Now `test_uno/` is mirrored to the mounted directory `test_dos/`

So you want to write some notes?
```bash
$ touch test_dos/notez.txt
$ echo "my beautiful notes I never took for OS" >> test_dos/notez.txt
```

Now let's see if they're there
```bash
$ ls test_uno/
notez.txt
$ ls test_dos/
notez.txt
```

Dope, they're in both directories. Let's take a look at our notes
```bash
$ cat test_uno/
<garbled mumbo jumbo gibberish>
```

Aw damn, I can't read that... but have no fear... it's there!
```bash
$ cat test_dos/
my beautiful notes I never took for OS
```

Yay it's all there! Wow! Many encryption! Such amazement! 11/10

Alright I should probably go study for this final now so I don't fail this class...
