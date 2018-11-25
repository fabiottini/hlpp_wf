/**************************************************************************
* This file is part of HLPP_WP.
*
* HLPP_WP is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* HLPP_WP is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with HLPP_WP.  If not, see <http://www.gnu.org/licenses/>.
*
* Author: Fabio Lucattini <fabiottini [at] gmail [dot] com>
* Date:   Settember 2016
***************************************************************************/


#define FROM_SEC_TO_MICRO(sec) sec*1000000
#define FROM_MICRO_TO_SEC(sec) sec/1000000

#define FROM_MILLISEC_TO_SEC(sec) sec/1000
#define FROM_SEC_TO_MILLISEC(sec) sec*1000

#define FROM_MICRO_TO_MILLISEC(sec) sec/1000
#define FROM_MILLISEC_TO_MICRO(sec) sec*1000

#include "Win_WF_abs.hpp"

class tuple_t : public input_struct<double>{
  private:
    std::atomic<int> *numWorkers = NULL;

    double ts;    // timestamp tuple "time"
    int id;       // identifier of the tuple
    int key;      // key identifier of the source
    bool defined = false;
    int numVal=0;
    volatile ticks emitter_ticks = 0;

  public:
      double *value = NULL; // value of the tuple "transport"

/*
    ~tuple_t(){
        freeTuple();
    }
*/

    tuple_t(){
      ts = -1;
      id = -1;
      key = -1;
        defined = true;
        numVal=0;
    }

    tuple_t(double ts_val, int id_val, int key_val, double *value_val, int pardegree_val)
      :id(id_val), key(key_val){
        ts = FROM_MICRO_TO_MILLISEC(ts_val);
        numWorkers = new std::atomic<int>(pardegree_val);
        if(value_val!=NULL) {

            value = (double *) calloc(DIM_TUPLE_VALUE, sizeof(double));
            if (value_val != NULL && value_val != 0) {
                numVal = DIM_TUPLE_VALUE;
                memcpy(value, value_val, (DIM_TUPLE_VALUE) * sizeof(double));
            }

            numVal = DIM_TUPLE_VALUE;
            //value = *value_val;
        }else{
            value=NULL;
        }
        defined = true;
    }

    bool isDefined(){
        return defined;
    }

    void print() {
        string valueStr = "";
        if(value != NULL) {
            for (int i = 0; i < DIM_TUPLE_VALUE; i++) {
                if (value != NULL)
                    valueStr += std::to_string(value[i]) + ", ";
                else
                    valueStr = -1;
            }
        }
        printf("\t{ ID: %d; TS: %f; KEY: %d; VALUE: %s; #WORKER: %d ; isSpecial: %s }\n", id, (ts), key, valueStr.c_str(), getWorkersNumber(),(isSpecialTuple())?"true":"false");
    }

    string toString() {
      char buffer [500];
      string valueStr;
        if(numWorkers!=NULL ) {
            if(value != NULL) {
                for (int i = 0; i < DIM_TUPLE_VALUE; i++) {
                    valueStr += to_string(value[i]);
                    if (i < DIM_TUPLE_VALUE - 1) {
                        valueStr += ", ";
                    }
                }
            }
            sprintf (buffer, "{ ID: %d; TS: %f; KEY: %d; VALUE: {%s}; #WORKER: %d ; isSpecial: %s}", id, (ts), key, valueStr.c_str(), getWorkersNumber(),(isSpecialTuple())?"true":"false");
        }else{
            sprintf (buffer, "-- NULL TUPLE --");
        }
      return buffer;
    }



    bool isSpecialTuple(){
        if(numWorkers!= nullptr || numWorkers!=NULL) {
            if ((getNumWorkers()) < 0 && id < 0) return true;
        }
        return false;
    }

    bool isStopTuple(){
        if((getNumWorkers())<0 && value==nullptr && id<0 && ts<0) return true;
        return false;
    }

    tuple_t* createSpeciaTuple(double currentTs, int workerId, double startTs){
        double *value = (double*) calloc(DIM_TUPLE_VALUE,sizeof(double));
        value[0] = startTs;
        // tuple_t *tmp = (tuple_t*) calloc(1,sizeof(tuple_t));
        // memcpy(tmp,new tuple_t(currentTs, -1, workerId, value, -1, true), sizeof(tuple_t));
        // tmp = new tuple_t(currentTs, -1, workerId, &value, -1);
        return new tuple_t(currentTs, -1, workerId, value, -1);
    }

    tuple_t* createStopTuple(){
        tuple_t *tmp = (tuple_t*) calloc(1,sizeof(tuple_t));
        tmp = new tuple_t((double)-1, -1, -1, nullptr, -1);//, sizeof(tuple_t));
        return tmp;
    }

    double getTupleTimeStamp(){ if(!defined) return -1; return this->ts;    }
    int    getTupleKey()      { if(!defined) return -1; return this->key;   }
    double *getTupleValue()   { if(!defined) return NULL; return this->value; }
    int    getTupleId()       { if(!defined) return -1; return this->id;    }

/*
    tuple_t *clone() {
      tuple_t *newTuple = new tuple_t(this->ts,this->id,this->key,this->value,getWorkersNumber());
      return newTuple;
    }
*/
    ticks getTupleEmitterTick(){ return emitter_ticks; }
    void setTupleEmitterTick(){ emitter_ticks=getticks(); }

    int getNumWorkers(){ return numWorkers->load(); }

    int incWorkers(){
        return (*numWorkers).fetch_add(1);
    }

    int decWorkers(){
        return (*numWorkers).fetch_sub(1);
    }

    int setWorkers(int val){
      (*numWorkers).store(val);
      return getWorkersNumber();
    }

    int getWorkersNumber(){
      if(numWorkers== nullptr) return -1;
      return (*numWorkers).load();
    }

    void freeTuple(){
        if(value!=NULL && value!= nullptr) {
            free(value);
        }
        if(numWorkers!=NULL && numWorkers!= nullptr) { delete numWorkers; }
        value = nullptr;
        numWorkers = nullptr;
    }

    // decrement the counter and check if is the last return true
    bool decWorkersCheck(){
        if(numWorkers==nullptr){ return false; }
        if(decWorkers()>1){
            return false;
        }else{
            //printf("delete: %.2f %d\n",ts,(*numWorkers).load());
            //freeTuple();
            return true;
        }
    }

   bool operator!=(tuple_t other) {
     if(this->ts != other.getTupleTimeStamp()) return true;
     if(this->id != other.getTupleId()) return true;
     if(this->key != other.getTupleKey()) return true;
     if(this->value != other.getTupleValue()) return true;
     return false;
   }

};


/****************************************[OUTPUT]********************************/
class output_t : public output_struct<double>{

  double ts = 0,          // timestamp in witch the tuple is emit
         tsWinStart = 0,
         tsWinEnd = 0;
  int    id = -1,          // id of the State
         key = 0,         // key identifier of the source
         workerId = 0;    // id of the F application

  double TIME_deltaTime = 0;

  long long int numElemInWin = 0;

  volatile ticks start_ticks = 0, latencyStart = 0;

 public:
  double **value = NULL;  //the output container of the value
  bool isNull = false;
  /*
  double *x_bid, *x_ask;
	double *y_bid, *y_ask;
  */

  double getTupleTimeStamp(){ return this->ts; }
  double **getTupleValue(){ return this->value; }
  double getTupleStartInterval() const { return this->tsWinStart; }
  double getTupleEndInterval() const { return this->tsWinEnd; }
  int    getTupleId() const { return this->id; }
  int    getTupleKey(){ return this->key; }
  int    getTupleWorkerId(){ return this->workerId; }

  //STATS
    ticks getCalculationTicks(){ return start_ticks; }
    void setCalculationTicks(ticks _t){ start_ticks=_t; }
    void setCalculationTicks(){ start_ticks=getticks(); }

    //latency from emitter -> collector
    void setEmitterLatencyTicks(ticks _t){ latencyStart=_t; }
    ticks getEmitterLatencyTicks(){ return latencyStart; }

    void setNumElemInWin(long long int _val){ numElemInWin=_val;  }
    long long int getNumElemInWin(){ return numElemInWin; }

  void updateCurrTs(double tsVal){
    ts = (tsVal);
  }

  void updateCurrTsMicro(double tsVal){
    ts = (tsVal)/1000;
  }
  void updateStart(double val){
    tsWinStart = (val);
  }
  void updateEnd(double val){
    tsWinEnd = (val);
  }
  void updateWorkerId(int val){
    workerId = val;
  }

  output_t(){}

    output_t(double currentTs, int id, int key, int workerId, double winStart, double winEnd)
            :ts(currentTs), tsWinStart(winStart), tsWinEnd(winEnd), id(id),   key(key),  workerId(workerId){
        #if STATS == 1
            start_ticks = getticks();
        #endif
        value = nullptr;//(double**) malloc(OUT_NUM_BLOCKS*OUT_NUM_VALUES*sizeof(double));
        /*
        for(int i=0;i<OUT_NUM_BLOCKS;i++){
            value[i] = new double[OUT_NUM_VALUES];
        }
        */
    }

  output_t(double currentTs, int id, int key, int workerId, double** value_val, double winStart, double winEnd)
    :ts(currentTs), tsWinStart(winStart), tsWinEnd(winEnd), id(id),   key(key),  workerId(workerId){
      #if STATS == 1
        start_ticks = getticks();
      #endif
    if(value_val!=NULL){
        value = (double**)calloc(OUT_NUM_BLOCKS*OUT_NUM_VALUES+1,sizeof(double));
        /*
        for(int i=0;i<OUT_NUM_BLOCKS;i++){
            value[i] = new double[OUT_NUM_VALUES];
        }
        */
        memcpy(value,value_val,OUT_NUM_BLOCKS*OUT_NUM_VALUES*sizeof(double));
    }else{
      value           = nullptr;
      isNull          = true;
    }
  }
/* TODO:  remove
  output_t *clone() {
      output_t *newTuple = new output_t(this->ts,this->id,this->key,this->workerId,this->value,this->tsWinStart,this->tsWinEnd);
      return newTuple;
    }
*/
    string toStringTupleValue(){
        string valueStr;
        if(value!=nullptr && value != NULL){
            for(int i=0;i<OUT_NUM_BLOCKS;i++){
                valueStr += "[ ";
                for(int j = 0; j<OUT_NUM_VALUES; j++){
                    valueStr += to_string(value[i][j]) ;
                    if(j<OUT_NUM_VALUES-1){
                        valueStr += ", ";
                    }
                }
                valueStr += "]";
                if(i<OUT_NUM_BLOCKS-1){
                    valueStr += ", ";
                }
            }
        }else{
            valueStr = "[]";
        }
        return valueStr;
    }

    void print() {
        printf("\t{ ID: %d; TS: %.2f; KEY: %d; VALUE: %s; WORKER_ID: %d; tsWinStart: %.2f; tsWinEnd: %.2f }\n",
               id, FROM_MILLISEC_TO_SEC(ts), key, toStringTupleValue().c_str(), workerId, FROM_MILLISEC_TO_SEC(tsWinStart), FROM_MILLISEC_TO_SEC(tsWinEnd));
    }

    string toString() {
        char buffer [500];
        sprintf (buffer, "{ ID: %d; TS: %.2f; KEY: %d; VALUE: %s; WORKER_ID: %d; tsWinStart: %.2f; tsWinEnd: %.2f }",
                 id, FROM_MILLISEC_TO_SEC(ts), key, toStringTupleValue().c_str(), workerId, FROM_MILLISEC_TO_SEC(tsWinStart), FROM_MILLISEC_TO_SEC(tsWinEnd));
        return buffer;
    }
};
