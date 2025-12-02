#!/bin/bash

SYZROOT=../../../syzkaller/bin/syz-manager

$SYZROOT -config=android.cfg $1
