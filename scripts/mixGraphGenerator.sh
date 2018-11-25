#!/bin/bash

DIROUT="mixedGraph/graph/"
GRAPH_TITLE="$1"
GRAPH_X="$2"
GRAPH_Y="$3"
IF_PLOT_FX=$4
INPUT_FILENAME="$5"
OUTPUT_FILENAME="$DIROUT$6"
COL1_NAME="$7"
COL2_NAME="$8"
IF_2COL_PERFILE=$9
IDEAL_MAX=${10}
IDEAL_MINY=${11}


#echo $IDEAL_MAX

Y_AXIS_MAX=$(echo "$IDEAL_MAX  + 10" |bc -l)
gnuplot <<- EOF

set title "$GRAPH_TITLE"
set xlabel "$GRAPH_X"
set ylabel "$GRAPH_Y"

set xtics 1

set term tikz

set size 1,1

set palette gray positive gamma 1.5

set linetype 1 dashtype ' - - '
set linetype 2 dashtype ' . . '

set style line 1 lt 8 lc rgb "#4d4d4d"  lw 10 ps 3
set style line 2 lt 4 lc rgb "#a0a0a0"  lw 10 ps 3
set style line 3 lc rgb 'black' lt 1 lw 10;
set style line 5 lc rgb 'black' lt 2 lw 10;

set grid ytics lt 0 lw 4 lc rgb "#bbbbbb"
set grid xtics lt 0 lw 4 lc rgb "#bbbbbb"

set xrange [0:]

if( $IDEAL_MAX > 0 ){
	set yrange [$IDEAL_MINY:$Y_AXIS_MAX]
}else{
	set yrange [$IDEAL_MINY:]
}

set key font ",18"

set ytics axis

#set x2lab "kx"
#set y2lab "ky"

#set key outside;
#set key right top;

set key above

set border back

set terminal pngcairo font 'Verdana,25' size 1024,768 enhanced
##set terminal png

f(x)=x
set xrange [1:13]

set output "$OUTPUT_FILENAME"
  
if ($IF_PLOT_FX == 1){
	set xrange [1:13]

   plot "$INPUT_FILENAME" using 0:2:xtic(1) with linespoints title "$COL1_NAME" ls 1,\
   					   '' using 0:3:xtic(1) with linespoints title "$COL2_NAME" ls 2,\
   					   '' using 0:f(x):xtic(1) with linespoints title "IDEAL" ls 3
}else{
	#
	if ($IF_2COL_PERFILE == 0){
		#set xrange [0:6]
   #plot for [col=1:2] "$INPUT_FILENAME" using 0:col with linespoints notitle ls col 
		   	plot "$INPUT_FILENAME" using 0:2:xtic(1) with linespoints title "$COL1_NAME" ls 1,\
		   					   '' using 0:3:xtic(1) with linespoints title "$COL2_NAME" ls 2
	}else{
		   	plot "$INPUT_FILENAME" using 0:2:xtic(1) with linespoints title "$COL1_NAME" ls 1,\
   					   			'' using 0:4:xtic(1) with linespoints title "$COL2_NAME"  ls 2,\
   					   			'' using 0:3:xtic(1) with linespoints title "IDEAL" ls 3,\
   					   			'' using 0:5:xtic(1) with linespoints title "IDEAL" ls 5,\
	}
 }

EOF
