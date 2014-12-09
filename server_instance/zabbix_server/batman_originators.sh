#!/bin/bash
batctl tg | cut -d")" -f2 | cut -d" " -f3 | grep -v "^$" | sort | uniq | wc -l
