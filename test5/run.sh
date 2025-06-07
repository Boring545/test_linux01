#!/bin/bash
EXEC=./dnstest
URLS=./url.txt
if [ ! -x "$EXEC" ]; then
    exit 1
fi
if [ ! -r "$URLS" ];then
    exit 1
fi
while read -r url; do
    if [ -n "$url" ]; then
        echo "域名：$url"
        $EXEC "$url"
        echo "-------------------------------------"
    fi
done < "$URLS"