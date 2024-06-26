host:
$ make push

device:
$ cd /data/local/tmp
$ insmod music_driver.ko
$ mknod /dev/music_driver c 242 0

