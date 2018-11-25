#!/bin/bash
FILEEXT=".txt"
DIR_DATA="testDone5/"
GRAPH_DATA="graphData5/"
GRAPHS="graphs5/"
ALL_FILES_FRAG=$(ls -r $DIR_DATA*$FILEEXT)   ########### OCCHIO FA LS INVERTITO

FRAG_GRAPH_NAME_DATA=$GRAPH_DATA"fragGraph.txt"
OUTPUT_GRAPH_TCALC=$GRAPHS"TCALC_FRAGMENTATION.png"

ARRAY_COUNT=0
ARRAY_FOUND[0]="";

LINEACTIVE=2

#echo ${ALL_FILES_FRAG[@]}

#retrieve all the possible combination that have agnostic and active
for i in $ALL_FILES_FRAG; do
  IFS='/' read -a arr <<< "$i"
  IFS='.' read -a arr <<< "${arr[1]}"
  FILENAME="${arr[0]}"
  IFS='_' read -a arr <<< "$FILENAME"

  FILENAME2="";
  CONTA=0

  for i in "${arr[@]}"; do
    if [ $CONTA -eq 0 ]; then
      FILENAME2=$FILENAME2$i
    elif [ $CONTA -eq 1 ]; then
      FILENAME2=$FILENAME2"_agnostic"
    elif [ $CONTA -ge 1 ]; then
      FILENAME2=$FILENAME2"_"$i
    fi
    let CONTA=$CONTA+1
  done


  TMP_FILENAME_AGNOSTIC=$DIR_DATA$FILENAME3$FILEEXT


  if [ -a $TMP_FILENAME_ACTIVE ] && [ -a $TMP_FILENAME_ACTIVE ]; then
    if ! [[ " ${ARRAY_FOUND[*]} " == *"$FILENAME2"* ]]; then
      #echo "AGGIUNGO: "$FILENAME2
      ARRAY_FOUND[$ARRAY_COUNT]=$FILENAME2
      let ARRAY_COUNT=$ARRAY_COUNT+1
    fi
  fi
done

#load data infos from files
AGNOSTIC_AVG_TCALC_VALUE=0
ACTIVE_AVG_TCALC_VALUE=0
AGNOSTIC_AVG_FRAG_VALUE=0
echo "" &> $FRAG_GRAPH_NAME_DATA
POS_X=0
FRAGVAL=""
FRAGVAL_POS=""
TEST_NAME=""
MAX_X_VAL=0
MAX_Y_VAL=0

COUNT=0

for i in "${ARRAY_FOUND[@]}"; do
#echo $i
 # AGNOSTIC_AVG_TCALC=$(ls $GRAPH_DATA"agnostic_"$i"_AVGTCALC"$FILEEXT)
 # ACTIVE_AVG_TCALC=$(ls $GRAPH_DATA"active_"$i"_AVGTCALC"$FILEEXT)
 # AGNOSTIC_FRAG=$(ls $GRAPH_DATA"agnostic_"$i"_FRAG"$FILEEXT)
 # 
  FILENAMEx="$i"
  #echo $FILENAMEx
  IFS='_' read -a arrx <<< "$FILENAMEx"
  #echo ${arrx[@]}
  FILENAME4="";
  CONTA=0
  for j in "${arrx[@]}"; do
    if [ $CONTA -eq 0 ]; then
      FILENAME4=$j
    elif [ $CONTA -eq 1 ]; then
      FILENAME4=$FILENAME4"_agnostic"
    elif [ $CONTA -gt 1 ]; then
      FILENAME4=$FILENAME4"_"$j
    fi
    let CONTA=$CONTA+1
  done


  AGNOSTIC_AVG_TCALC=$GRAPH_DATA$FILENAME4"_AVGTCALC"$FILEEXT
  AGNOSTIC_FRAG=$GRAPH_DATA$FILENAME4"_FRAG"$FILEEXT
  
  #echo ${AGNOSTIC_FRAG[@]}


  IFS='_' read -r -a EXP_TITLE <<< $i
  IFS=' ' read -r -a EXP_TITLE <<< $EXP_TITLE
  EXP_TITLE="${EXP_TITLE[1]}-${EXP_TITLE[2]}-${EXP_TITLE[3]}-${EXP_TITLE[4]}"

  if [ -a $AGNOSTIC_AVG_TCALC ]; then
    AGNOSTIC_AVG_TCALC_VALUE=$(<$AGNOSTIC_AVG_TCALC)
  else
    echo "ERROR: FILE "$AGNOSTIC_AVG_TCALC" NOT FOUND"
    exit 0;
  fi


  if [ $(bc -l <<< $AGNOSTIC_AVG_TCALC_VALUE">="$ACTIVE_AVG_TCALC_VALUE ) -eq 1 ]; then
    if [ $(bc -l <<< $AGNOSTIC_AVG_TCALC_VALUE">"$MAX_Y_VAL ) -eq 1 ]; then
      MAX_Y_VAL="$AGNOSTIC_AVG_TCALC_VALUE"
    fi
  fi

  if [ -a $AGNOSTIC_FRAG ]; then
    AGNOSTIC_AVG_FRAG_VALUE=$(<$AGNOSTIC_FRAG)
  else
    echo "ERROR: FILE "$AGNOSTIC_FRAG" NOT FOUND"
    exit 0;
  fi

  FRAGVAL=$FRAGVAL${AGNOSTIC_AVG_FRAG_VALUE%.*}'#'


  let POS_XX=$POS_X+1
  FRAGVAL_POS=$FRAGVAL_POS$POS_XX"#"  #".5#"

  MAX_X_VAL=$POS_X".5"
  ##MAX_X_VAL=$(( $MAX_X_VAL + 5 ))
  let POS_X=$POS_X+1
  echo $POS_X" "$AGNOSTIC_AVG_TCALC_VALUE >> $FRAG_GRAPH_NAME_DATA
  let POS_X=$POS_X+1
  let POS_X=$POS_X+1

  #let POS_X=$POS_X+2
  let COUNT=$COUNT+1

  TEST_NAME="" ##$TEST_NAME"{"$EXP_TITLE"}#"
done
#MAX_X_VAL=200

TEST_NAME="$(echo -e "${TEST_NAME}" | tr -d '[:space:]')"
FRAGVAL="$(echo -e "${FRAGVAL}" | tr -d '[:space:]')"
FRAGVAL_POS="$(echo -e "${FRAGVAL_POS}" | tr -d '[:space:]')"

if [ "${TEST_NAME:(-1)}" == "#" ]; then
  TEST_NAME="${TEST_NAME:0:(${#TEST_NAME}-1)}"
fi

if [ "${FRAGVAL:(-1)}" == "#" ]; then
  FRAGVAL="${FRAGVAL:0:(${#FRAGVAL}-1)}"
fi

if [ "${FRAGVAL_POS:(-1)}" == "#" ]; then
  FRAGVAL_POS="${FRAGVAL_POS:0:(${#FRAGVAL_POS}-1)}"
fi

./scripts/barGraphGenerator " " "Fragmentation (#Chunk in Window)" "T_{CALC} (ms)" 0 $FRAG_GRAPH_NAME_DATA "$OUTPUT_GRAPH_TCALC" "$FRAGVAL" "$FRAGVAL_POS" "$TEST_NAME" "$MAX_X_VAL" "$MAX_Y_VAL" $LINEACTIVE