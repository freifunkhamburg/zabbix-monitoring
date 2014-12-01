#!/bin/sh
expr $(/usr/sbin/batctl gwl | wc -l) - 1
