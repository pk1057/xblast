#! /bin/sh
./configure --prefix=/usr/common --with-otherdatadir=/usr/common/share/aixblast --enable-sound --enable-nat --enable-admin --enable-chat-filter || exit 1
make         || exit 1
make install || exit 1
