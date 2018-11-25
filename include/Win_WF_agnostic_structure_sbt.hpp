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

/***************************************************************************
* Library Window Farming time based
***************************************************************************
* @Author Fabio Lucattini
* @Date   September 2016
***************************************************************************
* [STRUCTURE]
*
*                   ------- WORKER_1
*                   |     /          \
* GENERATOR --> EMITTER + - WORKER_2 - + COLLECTOR --> <OUTPUT TUPLE>
*                   |     \   ...    /
*                   ------- WORKER_n
****************************************************************************/

#ifndef WIN_WF_TB_H
#define WIN_WF_TB_H

#define DEF_DEFAULT_QUEUE_WINDOW_SIZE 10


/****************************************************************************
*  Descriptor of the task that represent the special tuple from emitter
*  to workers in the AGNOSTIC CASE
****************************************************************************/
/** viene usata solo nel caso agnostico */
template <typename X>
class TaskTB : public Task<double, double, output_t> {
private:
  long int startValue = 0, endValue = 0, winID = 0, winListChunkID = 0;
  queueType::iterator *winIT;
  StateTB<X> *currentWindow;
  queueType *currentQueue;
  double currentTs = 0, startWinTs = 0, endWinTs = 0, realStartTs = 0;
  WinDes *currentElem;
  volatile ticks TIME_fireWin = 0;
  bool EOS_RECEIVE = false;
public:
  RANGE_TYPE loopNumber = 0;
  StateTB<X> *getWindow() { return currentWindow; }
  queueType *getQueue() { return currentQueue; }
  long int getStartPosition() { return startValue; }
  long int getEndPosition() { return endValue; }
  double getCurrentValue() { return currentTs; }
  double getStartWinValue() const { return startWinTs; }
  double getEndWinValue() const { return endWinTs; }
  double getRealStartTs() { return realStartTs; }
  queueType::iterator *getWinIT() { return winIT; }

  void setEOS(){ EOS_RECEIVE=true; }
  bool getEOS(){ return EOS_RECEIVE; }

  long int getWinId() { return winID; }
  long int getWinListChunkId() { return winListChunkID; }
  void updateWinListChunkId(long int _val ){ winListChunkID = _val; }
  WinDes* getCurrentElem (){ return currentElem; }

  //STATS PART
  ticks getTicksFireWin(){ return TIME_fireWin; }
  void updateTicksFireWin(){ TIME_fireWin=getticks(); }

  TaskTB() {}


  TaskTB(StateTB<X> *statePointer, queueType::iterator *_winIT, WinDes *_currentElem , double startTs, RANGE_TYPE winId, RANGE_TYPE _loopNumber) {
    currentWindow = statePointer;
    currentQueue  = *(currentWindow->getTupledeque());
    winIT         = _winIT;
    loopNumber    = _loopNumber;
    startValue    = (*winIT).getRangeStart();
    endValue      = (*winIT).getRangeEnd();
    startWinTs    = (*winIT).getTsStart();
    endWinTs      = (*winIT).getTsEnd();
    winID         = winId;
    realStartTs   = startTs;
  }

  string toString() {
    char buffer[1000];
    sprintf(buffer, "currentTs: %.2f ; TS_INTERVAL: [%.2f;%.2f] ; QUEUE_INTERVAL: [%ld,%ld]", currentTs, startWinTs,
    endWinTs, startValue, endValue);
    return buffer;
  }

};

/***********************************************************************
*  EMITTER
***********************************************************************/
using TaskTBL = TaskTB<FUN_ITERATOR_TYPE>;
using StateTBL = StateTB<FUN_ITERATOR_TYPE>;

class Emitter : public ff_monode_t<tuple_t, TaskTBL> {
private:

  ff_loadbalancer *lb; // i need it to user ff_send_out_to

  int workerId = 0, // utilized in ff_send_out_to
  pardegree = 0,
  key = 0, id = 0, idT = 0, idWorkerSpecialTuple = 0;
  double ts = 0, *value, wSize = 0, slide = 0, limitUP = 0;

  string enableDebug;

  int nextWorker = 0;
  double nextStartingWin = 0;

  long int startValue = 0, endValue = 0;

  StateTBL *currentStateTB = NULL;                       // the window that i use inside all the program
  StateTBL *queueWindow = NULL;
  queueType *currentQueue = NULL;                       // AUX pointer to the queue inside the window

  unsigned int *workerStatus = NULL;                       //cointains 1 if the worker is working and not reply with special tuple 0 otherwise
  std::vector<TaskTBL> buffer;                    //cointains the TaskTB (the tuple for the worker) that is not sent
  deque<double> *confirmedTsWorker = new deque<double>;          //cointains the confirm (special tuple ts value) out of order (is an ordered queue)

  int lastDeleted = 0; // count the lastDeleted tuple
  int EOS_RECEIVE = 0; // is 0 if EOS is not received 1 otherwise
  double currentStartTs = 0, currentEndTs = 0; //cointains tha actual value of the window

  double slideTmp = 0;  //temporary value that i use to identify next init value interval

  int DEFAULT_QUEUE_WINDOW_SIZE = DEF_DEFAULT_QUEUE_WINDOW_SIZE;

  //TO REMOVE long int idWindow = 0; //the window to elaborate/emit
  output_t *(*F)(FUN_ITERATOR_TYPE ,FUN_ITERATOR_TYPE , output_t *);

  double minOnTheFly = 0;
  long long int minIdOnTheFly = 1;
  long long int loopNumber = 0;
  double stopDeletion=0;

  #if STATS==1
  double  TIME_boot     = 0,
  TIME_startWin = 0,
  TIME_endWin   = 0;
  #endif

  bool FORCE_CLEAN_QUEUE =false;

  double *lastAssigned;

public:

  Emitter(output_t *(*FPOINTER)(FUN_ITERATOR_TYPE ,FUN_ITERATOR_TYPE , output_t *), ff_loadbalancer *const lb, int pardegree, double wSize, double slide, double _limitUP, string enableDebug)
  : F(FPOINTER), lb(lb), pardegree(pardegree), wSize(wSize), slide(slide), enableDebug(enableDebug), limitUP(_limitUP) {
    #ifdef _STATS
    BOOT_TIMER = getusec(); //initialize the timer
    #endif
    assert(lb != nullptr && lb != NULL);
    assert(pardegree >= 1);
    workerStatus     = (unsigned int *) calloc(pardegree, sizeof(unsigned int));
    slideTmp         = slide;
    currentEndTs     = wSize;
    lastAssigned     = (double*) calloc(pardegree,sizeof(double));
    queueWindow      = (StateTBL *) calloc(DEFAULT_QUEUE_WINDOW_SIZE, sizeof(StateTBL));
    stopDeletion     = limitUP-(wSize*2);
  }

  ~Emitter() {
    confirmedTsWorker->clear();
    buffer.clear();
    free(queueWindow);
    free(lastAssigned);
    free(workerStatus);
  }

  int svc_init(){
    #if STATS==1
    TIME_boot = getusec();
    #endif
    return 0;
  }

/**
* SEND DATA TO WORKER ff_send_out
*/
int sendToWorker(TaskTBL *toSend) {
  int idWorker = getNextAvailableWorker();
  if (idWorker == -1) return -1;

  if(currentStateTB->getDeletedWin()>0){
    toSend->updateWinListChunkId(toSend->getWinListChunkId()-currentStateTB->getDeletedWin());
  }

  workerStatus[idWorker] = 1;
  lastAssigned[idWorker] = toSend->getStartWinValue();
  lb->ff_send_out_to(toSend, idWorker);
  nextWorker++;

  return idWorker;
}

/**
* get first available worker
*/
int getNextAvailableWorker() {
  bool found = false;
  int ret = -1;
  if (nextWorker < pardegree) {
    for (int i = 0; i < pardegree && found == false; i++) {
      if (workerStatus[i] == 0) {
        return i;
      }
    }
  }
  return ret;
}

/**
Check if something is in the queue and wait for the elaboration
*/
void checkIfSomethingInTheQueue() {
  std::vector<TaskTBL>::iterator it = buffer.begin();
  bool fine = false;
  //printf("checkIfSomethingInTheQueue: %lld\n",buffer.size());
  if (buffer.size() > 0) {
    //invio le elaborazioni in coda fino a quando ci sono worker a disposizione
    for (;it!=buffer.end() && fine == false; ) {
      if (nextWorker < pardegree) {
        TaskTBL* tmp = (TaskTBL*) calloc(1,sizeof(TaskTBL));
        *tmp = (*it);
        int idW = sendToWorker(tmp);
        buffer.erase(it);  //to remove the element
      } else {
        fine = true;
      }
    }
  }
}

/**
* print the buffer queue
*/
void printQueue(){
  vector<TaskTBL>::iterator it = buffer.begin();
  for (;it!=buffer.end();++it) {  printf(" >> %s\n",(*it).toString().c_str());  }
}

/**
SIGNAL THE TERMINATION TO THE WORKER (wrap_around)
*/
void propagateEOStoWORKER() {
  for (int i = 0; i < pardegree; i++) {
    lb->ff_send_out_to(EOS, i);
  }
}

static bool comparedTaskBuffer(const TaskTBL &a, const TaskTBL &b) {
  if(a.getStartWinValue() < b.getStartWinValue()) return true;
  return false;
}

queueType::iterator *getClosedWindowSBT(double startWin, double endWin) {
  return (*currentStateTB->getTupledeque())->getNextAvailableWinSBT(startWin, endWin);
}

/**
* If reach a normal tuple add it to the queue.
* This function check if the tuple fire a window
*/
void elaborateCurrentWinSBT(double firstTupleTs, double lastTupleTs) {
  queueType::iterator *windowIT;

  if ((windowIT = getClosedWindowSBT(firstTupleTs, lastTupleTs)) != NULL){
    //create new TaskTB to assign to the worker
    double tmpTsStart   = windowIT->getTsStart();
    RANGE_TYPE tmpWinID = windowIT->getWinId();
    TaskTBL *sendOut = new TaskTBL(currentStateTB, windowIT, NULL, lastTupleTs, loopNumber,loopNumber);
    #if STATS == 1
    TIME_endWin = getusec();
    sendOut->updateTicksFireWin();
    #endif
    if (nextWorker < pardegree) { //there are available worker
      int toWorker = sendToWorker(sendOut);
    } else { //no available worker => put the TaskTB in the queue and go on
      buffer.push_back(*sendOut);
      if(buffer.size()>1){
        std::sort( buffer.begin() , buffer.end() , comparedTaskBuffer );
      }
    }
    loopNumber++;
  }
  //in case of fewer worker test if the last emitted task is in the buffer queue
  if(windowIT==NULL && EOS_RECEIVE == 1){
    checkIfSomethingInTheQueue();
  }
}

TaskTBL *svc(tuple_t *t) {

  //GESTIONE DELLA FINESTRA SU CUI LAVORARE
  if (!(*t).isSpecialTuple()) {
    key = (*t).getTupleKey();

    //se non esiste una coda per la chiave indicata la creo
    if (key >= DEFAULT_QUEUE_WINDOW_SIZE) {
      DEFAULT_QUEUE_WINDOW_SIZE = key + 10;
      queueWindow = (StateTBL *) realloc(queueWindow, sizeof(StateTBL) * DEFAULT_QUEUE_WINDOW_SIZE);
    }

    //se esiste la coda ma non è ancora stato inizializzata
    if (queueWindow[key].getKey() == 0) {
      queueWindow[key] = *(new StateTBL(F, 0, key, 0, wSize, slide, new queueType(0, slide, wSize)));
    }

    currentStateTB = &(queueWindow[key]);
    currentQueue = *(currentStateTB->getTupledeque());
  }else{
    idWorkerSpecialTuple = (*t).getTupleKey();
  }

  if ((*t).isSpecialTuple()) {
    //[SPECIAL TUPLE] (message from worker)***********************************************
    workerStatus[idWorkerSpecialTuple] = 0;        //reset the status of the worker
    nextWorker--;                                  //update the number of worker in use

    int count = 0,i=0;
    for(;i<pardegree;i++){
      if(lastAssigned[i]<=minOnTheFly){
        count ++;
        break;
      }
    }
    i--;
    
    if(count==0){
      if(((*t).getTupleValue())[0]<stopDeletion){
       // RANGE_TYPE sizeBefore = (*currentStateTB->getTupledeque())->size();
        //CLEAN PART
        lastDeleted = (*currentStateTB->getTupledeque())->erase(minOnTheFly);
        lastDeleted = currentStateTB->evictionUpdate(lastDeleted, minOnTheFly); //EVICTION FUNCTION
        //END CLEAN PART 
      }
      minOnTheFly = ((*t).getTupleValue())[0];
    }

    checkIfSomethingInTheQueue();

    if(EOS_RECEIVE==1){
      if (buffer.size() == 0 || minOnTheFly >= limitUP){
        propagateEOStoWORKER();
      }
    }

    (*t).freeTuple();

  } else {
//[NORMAL TUPLE]***********************************************
    if((*t).getTupleTimeStamp() <= limitUP){
      if(currentStateTB->updateARRAY(t) == 1){
        (*currentStateTB->getTupledeque())->incIsWinClose();
        double startWin = (*t).getTupleTimeStamp()-wSize;
        startWin = (startWin>0)?startWin:0;
        elaborateCurrentWinSBT(startWin,(*t).getTupleTimeStamp());
      }
    }
  }
  delete t;
  return GO_ON;
}

void eosnotify(ssize_t) {
  EOS_RECEIVE = 1;
  if (buffer.size() == 0){ 
    propagateEOStoWORKER(); 
  }else{ 
    checkIfSomethingInTheQueue(); 
  }
}

void svc_end() {
  checkIfSomethingInTheQueue();
}

};


/***********************************************************************
*  WORKER
***********************************************************************/
class Worker : public ff_monode_t<TaskTBL, output_t> {
private:
  double wLength = 0, sliding = 0, resultFunction = 0, limitUP = 0, initTupleTS = 0;
  int insPos = 0, idRes = 0, workerId = 0, pardegree = 0;
  string enableDebug;
  bool operationDone = false;
  bool receiveEOS = false;
  int EOS_RECEIVE = 0;
  RANGE_TYPE lastIdRes = 0;
  StateTBL *currentStateTB = NULL;
  output_t *(*F)(FUN_ITERATOR_TYPE ,FUN_ITERATOR_TYPE , output_t *); //the function signature
  #if STATS == 1
  volatile ticks  TICKS_START   = 0;
  #endif
public:

  Worker(output_t *(*FPOINTER)(FUN_ITERATOR_TYPE ,FUN_ITERATOR_TYPE , output_t *), double limitUP, double wLength, double sliding, double initTupleTS, int nworkers, string enableDebug)
  : F(FPOINTER), limitUP(limitUP), wLength(wLength), sliding(sliding), initTupleTS(initTupleTS), pardegree(nworkers), enableDebug(enableDebug) {
  }

  int svc_init() {
    workerId = get_my_id();
    return 0;
  }

  tuple_t* createSpeciaTuple(double currentTs, int workerId, double startTs, double idWin){
    double *value;
    if(DIM_TUPLE_VALUE>=2){
      value = (double*) calloc(DIM_TUPLE_VALUE,sizeof(double));
    }else{
      value = (double*) calloc(2,sizeof(double));
    }
    value[0] = startTs;
    value[1] = idWin;
    for(int i=2;i<DIM_TUPLE_VALUE;i++){  value[i] = -1;   }
    return new tuple_t(currentTs, -1, workerId, value, -1);
  }

  output_t *svc(TaskTBL *t) {
    double currentStartWinVal= 0, currentEndWinVal = 0;
    long int currentWinId = 0;
    WinDes *currentElem;
    output_t *ret= NULL;

    //ricavo i dati dalla tupla inviata
    currentStateTB      = (*t).getWindow();
    currentWinId        = (*t).getWinId();
    currentStartWinVal  = (*t).getStartWinValue();
    currentEndWinVal    = (*t).getEndWinValue();

    currentElem          = (*t).getCurrentElem();
    double startTs       = (*t).getStartWinValue();
    double endTs         = (*t).getEndWinValue();

    #if STATS == 1
    TICKS_START = (*t).getTicksFireWin();
    #endif

    queueType::iterator *tmp = (*t).getWinIT();
    if( tmp != NULL ) {
      FUN_ITERATOR_TYPE _begin = *tmp->beginITSBT(startTs,endTs,sliding); //retrieve the window start pointer iterator
      FUN_ITERATOR_TYPE _end   = *tmp->endITSBT(startTs,endTs,sliding);   //retrieve the window end pointer
      ticks CALCULATION_START = getticks(); 
      ret = currentStateTB->getResultIDWin(currentStartWinVal, workerId, _begin, _end, currentElem, currentWinId);

      ret->setNumElemInWin(distance(_begin,_end));

      ret->setCalculationTicks(getticks()-CALCULATION_START);
      lastIdRes = ret->getTupleId();

      delete tmp;
    }else{
      ++lastIdRes;
      ret = new output_t(0, lastIdRes, currentStateTB->getKey(), workerId, startTs, endTs);
      ret->setNumElemInWin(0);

    }

    if(ret == NULL){
      printf("%s SVC WORKER [%d] RET OUTPUT VAL NULL %s\n",ANSI_BG_COLOR_RED,workerId,ANSI_COLOR_RESET);
    }

    if (limitUP != -1 && ret != NULL) {
      try {
        ret->updateWorkerId(workerId);
        #if STATS == 1
        ret->setEmitterLatencyTicks(TICKS_START);
        #endif

        ff_send_out_to(ret, 1);    //send to collector the results
        tuple_t *tmp = createSpeciaTuple(0, workerId, currentStartWinVal,(*t).loopNumber);
        ff_send_out_to(tmp, 0);  //send to emitter the special tuple
        operationDone = true;
        if (receiveEOS == true) {
          ff_send_out_to(EOS, 1);    //send to collector the EOS
        }
      } catch (exception &e) {
        printf(ANSI_COLOR_WHITE ANSI_BG_COLOR_RED "WORKER SEND_OUT EXCEPTION: %s | WINID: %ld \n" ANSI_COLOR_RESET,
        e.what(), currentWinId);
      }
    }


    delete t;
    return GO_ON;
  }

  void eosnotify(ssize_t) {
    EOS_RECEIVE = 1;
    if (operationDone) {
      ff_send_out_to(EOS, 1);    //send to collector the EOS
    }
  }

  void svc_end() {
    if (receiveEOS && operationDone) {
      ff_send_out_to(EOS, 1);    //send to collector the EOS
    }
  }
};

#endif
