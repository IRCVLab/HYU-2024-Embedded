load 'plot/style.gp'
set output filename
set title data.' -> '.filename noenhanced
plot data using 2:($1=="0"?$1:1/0) title "Process 0"
pause -1
