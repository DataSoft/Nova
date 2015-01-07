set grid xtics ytics ztics
set xtic auto
set ytic auto
set ztic auto

set xlabel "K"
set ylabel "True Positive Rate"
set zlabel "False Positive Rate"
set title "ROC Curve"

splot "output.txt" with linespoints
