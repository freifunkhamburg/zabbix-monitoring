#!/bin/sh
TARGET=8.8.8.8
PACKETMARK=1
GATEWAY_IP=#CHANGEME
NUMPROBES=4
THRESHOLD=2

RECEIVED=$(ping -m"$PACKETMARK" -I "$GATEWAY_IP" -c "$NUMPROBES" "$TARGET" | grep -oP '\d+(?= received)')

if [ "$RECEIVED" -ge "$THRESHOLD" ]; then
	echo 1
else
	echo 0
fi
