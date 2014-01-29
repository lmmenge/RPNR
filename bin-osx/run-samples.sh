#!/bin/bash

./rpnr -i ../samples/esccomp-356p-100samples -e 0001.png
./rpnr -i ../samples/esccomp-1080p-100samples -e 0001.png -fradius 15
./rpnr -i ../samples/salatv-356p-2x100samples -e 0001.png -fradius 15
./rpnr -i ../samples/salatv-356p-100samples -e 0001.png -fradius 15
./rpnr -i ../samples/salatv-356p-200samples -e 0001.png -fradius 15