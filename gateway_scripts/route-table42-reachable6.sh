#!/bin/sh
TARGET=2001:4860:4860::8888
PACKETMARK=1
GATEWAY_IP6=#CHANGEME
NUMPROBES=4
THRESHOLD=2

RECEIVED=$(ping6 -m"$PACKETMARK" -I "$GATEWAY_IP6" -c "$NUMPROBES" "$TARGET" | grep -oP '\d+(?= received)')

if [ "$RECEIVED" -ge "$THRESHOLD" ]; then
	echo 1
else
	echo 0
fi
