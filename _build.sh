#!/bin/bash

git pull
cmake .
cmake --build . -- -j $(cat /proc/cpuinfo | grep processor | wc -l)

mkdir -p build

# if building yt-dlp
git clone https://github.com/yt-dlp/yt-dlp.git
(
    cd yt-dlp
    git fetch origin
    git checkout master
    git reset --hard origin/master # guaranteed clean master на fresh origin/master
    wget -O - https://github.com/yt-dlp/yt-dlp/pull/13512.patch | git apply - # apply threads.com/net patch
    make yt-dlp
)
cp yt-dlp/yt-dlp build/yt-dlp
# else downloading the latest version
#wget -O build/yt-dlp https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp_linux

chmod +x build/yt-dlp # make sure yt-dlp has executable rights

wget -O build/gallery-dl https://github.com/mikf/gallery-dl/releases/latest/download/gallery-dl.bin
chmod +x build/gallery-dl

# don't update to nightly when building
#./build/yt-dlp --update-to nightly

PATH=$PATH:build

exec ./build/downloader-bot