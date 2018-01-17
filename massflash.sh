#!/bin/bash

while [ 1 ]; do
	clear
	echo "insert badge"
	until [[ "$(st-info --chipid)" -eq "0x0448" ]]; do
		true
	done

	st-flash reset
	st-flash write $1 0x8000000

	echo "done. remove badge"
	while [[ "$(st-info --chipid)" -eq "0x0448" ]]; do
		sleep 0.5s
	done


done
