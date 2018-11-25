#!/bin/bash

PROGRAM_NAME="mixedGraph"
FILEEXT=".txt"
FILEEXTIMG=".jpg"
DIR="graphData2/"
DIR_OUT="mixedGraph/"
DIR_SCRIPT="scripts/"
MIXEDGRAPH="mixGraphGenerator.sh"

FILE1=$1
FILE2=$2

IF_2COL_PERFILE=$3
TYPE=$4

GLOBAL_FILE1=$5
GLOBAL_FILE2=$6


MAX_Y=$8
MIN_Y=$7

if [ -z $MAX_Y ]; then
	MAX_Y=0
fi
if [ -z $MIN_Y ]; then
	MIN_Y=0
fi

IDEAL_MAX=0

METRIC="LAT"
RATE=100000
BURSTINESS=50000
WIN=5000
SLIDE=100

#retrieve all the file from the folder
ALL_FILES_IN_FOLDER=$(ls $DIR*$FILEEXT)

function mixGraph(){
	declare -a LINEARRAY_FILE_1
	declare -a LINEARRAY_FILE_2
	
	FILEOUTPUT=""
	FILE_1="$DIR$1"
	FILE_2="$DIR$2"
	FILE_CHECK_1=$(checkFileExists "$FILE_1")
	FILE_CHECK_2=$(checkFileExists "$FILE_2")
	
	#GENERATE FILEOUTPUT NAME
	IFS='/' read -a arr <<< "$FILE_1"
  	IFS='.' read -a arr <<< "${arr[1]}"
  	FILENAME="${arr[0]}"
  	IFS='_' read -a arr <<< "$FILENAME"

  	IFS='/' read -a arr2 <<< "$FILE_2"
  	IFS='.' read -a arr2 <<< "${arr2[1]}"
  	FILENAME2="${arr2[0]}"
  	IFS='_' read -a arr2 <<< "$FILENAME2"

  	TIPO=""
  	if [ "${arr[6]}" = "SCALABILITY" ]; then
  		TIPO="SCALABILITY"
  	elif [ "${arr[6]}" = "BW" ]; then
  		TIPO="BW"
  	fi

  	CONTA=0
  	FILEOUTPUT="MIXED_"${arr2[0]}
  	for i in "${arr[@]}"; do
  		if [ $CONTA -gt 0 ]; then
  			FILEOUTPUT=$FILEOUTPUT"_"$i"_"${arr2[$CONTA]}
  		fi
  		let CONTA=$CONTA+1
  	done
  	FILEOUTPUT=$DIR_OUT$FILEOUTPUT$FILEEXT

	deleteFile $FILEOUTPUT

	### READ DATA FROM FILE 1
	#echo "> READ: $FILE_1"
	CONTA=0
	if [ $FILE_CHECK_1 -eq 1 ]; then
		while IFS="" read -r line || [[ -n "$line" ]]; do
			if [ ! -z "$line" ]; then
				LINEARRAY_FILE_1[$CONTA]="$line"
				let CONTA=$CONTA+1
			fi
		done < "$FILE_1"
	else
		echo "[readFile] NO FILE FOUND 1"
	fi


	### READ DATA FROM FILE 2
	#echo "> READ: $FILE_2"
	CONTA=0
	if [ $FILE_CHECK_2 -eq 1 ]; then
		while IFS="" read -r line || [[ -n "$line" ]]; do
			if [ ! -z "$line" ]; then 						# check that i have the same row number
				LINEARRAY_FILE_2[$CONTA]="$line"
				let CONTA=$CONTA+1
			fi
		done < "$FILE_2"
	else
		echo "[readFile] NO FILE FOUND 2"
	fi


	### MIX DATA FROM FILE 1 AND FILE 2 AND OUTPUT TO FILEOUTPUT
	if [ ${#LINEARRAY_FILE_1[@]} -eq ${#LINEARRAY_FILE_2[@]}  ]; then
		#echo "ok sono uguali => ${#LINEARRAY_FILE_1[@]}"
		CONTA=0
		CORRENTE=0
		for i in "${LINEARRAY_FILE_1[@]}"; do
			IFS=' ' read -a arr <<< "$i"
			FILE_INDEX=${arr[0]}
			FILE_1_VAL=${arr[1]}
			FILE_2_VAL=${LINEARRAY_FILE_2[$CONTA]}
			IFS=' ' read -a FILE_2_VAL_ARR <<< "$FILE_2_VAL"
			FILE_2_VAL=${FILE_2_VAL_ARR[1]}
			
			if [ $IF_2COL_PERFILE -eq 1 ]; then
				if [ "${arr[2]}" != "" ]; then
					if [ $(echo "${arr[2]}  > $IDEAL_MAX" |bc -l) ]; then
						IDEAL_MAX=${arr[2]}
					fi
					if [ $(echo "${FILE_2_VAL_ARR[2]}  > $IDEAL_MAX" |bc -l)  ]; then
						IDEAL_MAX=${FILE_2_VAL_ARR[2]}
					fi
				fi
				FILE_1_VAL=$FILE_1_VAL" "${arr[2]}
				FILE_2_VAL=$FILE_2_VAL" "${FILE_2_VAL_ARR[2]}
			fi

			if [ ! $CORRENTE -eq $FILE_INDEX ]; then 
				writeAppend $FILEOUTPUT "$CORRENTE "
				let CORRENTE=$CORRENTE+1
			fi

			# if [ "$TIPO" = "BW" ]; then
			# 	FILE_1_VAL=$(bc <<< "scale=3; 0.01/$FILE_1_VAL")
			# 	FILE_2_VAL=$(bc <<< "scale=3; 0.01/$FILE_2_VAL")
			# elif [ "$TIPO" = "SCALABILITY" ]; then
			# 	if [ ! $CORRENTE -eq $FILE_INDEX ]; then 
			# 		writeAppend $FILEOUTPUT "$CORRENTE "
			# 		let CORRENTE=$CORRENTE+1
			# 	fi
			# fi
			writeAppend $FILEOUTPUT "$FILE_INDEX $FILE_1_VAL $FILE_2_VAL"
			let CONTA=$CONTA+1
			let CORRENTE=$CORRENTE+1
		done
	else
		echo -e "\n> SONO DIVERSI CONTROLLA: \n\t $FILE_1: ${#LINEARRAY_FILE_1[@]} \n\t $FILE_2: ${#LINEARRAY_FILE_2[@]}"	
	fi

	echo $IDEAL_MAX"#"$FILEOUTPUT

}

function deleteFile(){
	rm -rf $1
}

function writeAppend(){
	echo $2 >> $1;
}

function checkFileExists(){
	FILE=$1
	if [ -f $FILE ]; then
		echo 1
	else
		echo 0
	fi
}

#if found 1 data graph i check if exist the corrispective 
#if found agnostic -> check active
#if found active -> check agnostic
function convertFile(){
	FILENAME_NEW="";
	FILENAME_ORIG="";
	CONTA=0
	arr=$1

	for i in "${arr[@]}"; do
		if [ $CONTA -eq 0 ]; then
			if [ "$i" == "agnostic" ]; then
				FILENAME_NEW="active"
				FILENAME_ORIG="agnostic"
			elif [ "$i" == "active" ]; then
				FILENAME_NEW="agnostic"
				FILENAME_ORIG="active"
			fi
		elif [ $CONTA -ge 1 ]; then
		  	FILENAME_NEW=$FILENAME_NEW"_"$i
		  	FILENAME_ORIG=$FILENAME_ORIG"_"$i
		fi
		let CONTA=$CONTA+1
	done

	FILENAME_NEW=$FILENAME_NEW$FILEEXT
	FILENAME_ORIG=$FILENAME_ORIG$FILEEXT
	
	FILE_STATUS=$(checkFileExists "$DIR$FILENAME_NEW")

	if [ $FILE_STATUS -eq 1 ]; then #file exists
		mixGraph $FILENAME_ORIG $FILENAME_NEW #mix the 2 graph
	else
		echo "FILE: '"$FILENAME_NEW"' NOT FOUND"
	fi
}

#SCAN $ALL_FILES_IN_FOLDER and mix 
function readDataFromFile(){
	for i in $ALL_FILES_IN_FOLDER; do
		IFS='/' read -a arr <<< "$i"
	  	IFS='.' read -a arr <<< "${arr[1]}"
	  	FILENAME="${arr[0]}"
	  	IFS='_' read -a arr <<< "$FILENAME"


	  	if [ ${#arr[@]} -eq 7 ]; then
	  		#solo attivi sennò farei 2 volte il solito mix
	  		if [ "${arr[0]}" = "active" ]; then 
	  			convertFile ${arr[@]}
	  		fi
	  	fi
	done

}

function plotGraph(){
	ALL_MIXED_IN_FOLDER=$(ls $DIR_OUT*$FILEEXT)

	X_AXIS="# Workers"
	TITLE_GRAPH=""
	FILE_INPUT_NAME=$DIR_OUT""$1
	IDEAL_MAX=$2
	TYPE=$3

	#for i in $ALL_MIXED_IN_FOLDER; do
		IFS='/' read -a arr <<< "$FILE_INPUT_NAME"
	  	IFS='.' read -a arr <<< "${arr[2]}"
	  	FILENAME="${arr[0]}"
	  	IFS='_' read -a arr <<< "$FILENAME"
	  	#TYPE="${arr[7]}"
	  	GENERATE=1
	  	GENERATE_YX=0
echo $TYPE
	  	if [ "$TYPE" = "LAT" ]; then
	  		Y_AXIS="Latency (usec)"
	  	elif [ "$TYPE" = "AVGTCALC" ]; then
	  		Y_AXIS=""
	  		GENERATE=0
	  	elif [ "$TYPE" = "BW" ]; then
	  		Y_AXIS="BW (#Window/sec)"
	  	elif [ "$TYPE" = "SCALABILITY" ]; then
	  		Y_AXIS="Scalability"
	  		GENERATE_YX=1
	  	elif [ "$TYPE" = "TCALC" ]; then
	  		Y_AXIS="Calculation Time (ms)"
	  	elif [ "$TYPE" = "TCOMPL" ]; then
	  		Y_AXIS="Completition Time (s)"
	  	fi

	  	if [ $MAX_Y -gt 0 ]; then 
	  		IDEAL_MAX=$MAX_Y
	  	fi

	  	if [ $GENERATE -eq 1 ]; then
	  		OUTPUT_FILE_GRAPH=$FILENAME$FILEEXTIMG
			$("./"$DIR_SCRIPT$MIXEDGRAPH "$TITLE_GRAPH" "$X_AXIS" "$Y_AXIS" $GENERATE_YX $DIR_OUT$FILENAME".txt" $OUTPUT_FILE_GRAPH "$GLOBAL_FILE1" "$GLOBAL_FILE2" $IF_2COL_PERFILE $IDEAL_MAX $MIN_Y)
		fi
	#done 
}

#readDataFromFile 
FILEOUT=$(mixGraph "$FILE1" "$FILE2")
#echo $FILEOUT
IFS='#' read -a arr <<< "$FILEOUT"
#echo $TYPE
#echo "${arr[1]}"
plotGraph "${arr[1]}" "${arr[0]}" $TYPE
