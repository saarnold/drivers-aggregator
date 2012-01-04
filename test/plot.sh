#/bin/bash

file=$1
gnuplot -p<< EOF
plot '$file' using 0:1 smooth unique title 'hwTime - realtime',\
'$file' using 0:2 smooth unique title 'sampleTime - realtime', \
'$file' using 0:3 smooth unique title 'estimatedTime - realtime'
EOF
