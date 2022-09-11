#!/bin/sh

wordlist=$1
output=$2
if [ -f "$wordlist" ] && [ "$#" -eq 2 ]
then
	grep -o -w '[a-zA-Z]\{3,15\}' $wordlist | sort -k 1.3 | uniq -u > $output
else
	echo "Please check the number of arguments you supplied and the wordlist file"
	echo "Example usage: ./Task1.sh /usr/share/dict/words ./out.txt"
fi
