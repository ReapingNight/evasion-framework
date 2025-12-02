#!/bin/bash

SYZROOT=../../../syzkaller/bin/syz-manager

$SYZROOT -config=dand$1.cfg
