#!/bin/bash

./rundude.sh -p attiny85 -U eeprom:r:eeprom.bin:r
mc -v eeprom.bin