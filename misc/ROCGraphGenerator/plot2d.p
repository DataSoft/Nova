
set xtic auto
set ytic auto

#set xrange [0:1]
#set yrange [0:1]

set grid xtics ytics

set ylabel "True Positive Rate"
set xlabel "False Positive Rate"
set title "ROC Curve"

plot "output.txt" using 3:2
#plot "output.txt" using 3:2:1 with labels

# Useful for classification threshold vs true/false positive
#plot "classification.txt" using 1:2 title "True Positive Rate", "rocVsK.txt" using 1:3 title "False Positive Rate"
