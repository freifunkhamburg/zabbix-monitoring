#!/bin/sh
ps ax | grep dhcpd | grep -v grep | wc -l
