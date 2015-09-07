#!/bin/sh

#avrdude -D -B 5 -c usbasp $*
avrdude -B 5 -c usbasp $*
