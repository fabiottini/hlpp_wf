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
  queueType::iterator *winIT;// = (queueType::iterator*) calloc(1,sizeof(queueType::iterator));
  StateTB<X> *currentWindow;// = (StateTB<X>*) calloc(1,sizeof(StateTB<X>));
  queueType *currentQueue;//= (queueType*) calloc(1,sizeof(queueType));
  double currentTs = 0, startWinTs = 0, endWinTs = 0, realStartTs = 0;
  WinDes *currentElem;//= (WinDes*) calloc(1,sizeof(WinDes));
  volatile ticks TIME_fireWin = 0;
  bool EOS_RECEIVE = false;
public:
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


  TaskTB(StateTB<X> *statePointer, queueType::iterator *_winIT, WinDes *_currentElem , double startTs, RANGE_TYPE winId) {
    currentWindow = statePointer;
    currentQueue  = *(currentWindow->getTupledeque());
    winIT         = _winIT;

    currentElem   = _currentElem;
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

  #if STATS==1
  double  TIME_boot     = 0,
  TIME_startWin = 0,
  TIME_endWin   = 0;
  #endif

  bool FORCE_CLEAN_QUEUE =false;

public:

  Emitter(output_t *(*FPOINTER)(FUN_ITERATOR_TYPE ,FUN_ITERATOR_TYPE , output_t *), ff_loadbalancer *const lb, int pardegree, double wSize, double slide, double _limitUP, string enableDebug)
  : F(FPOINTER), lb(lb), pardegree(pardegree), wSize(wSize), slide(slide), enableDebug(enableDebug), limitUP(_limitUP) {
    #ifdef _STATS
    BOOT_TIMER = getusec(); //initialize the timer
    #endif
    assert(lb != nullptr && lb != NULL);
    assert(pardegree >= 1);
    workerStatus     = (unsigned int *) calloc(pardegree, sizeof(unsigned int));
    //memorySendToWork = (TaskTBL *) calloc(pardegree, sizeof(TaskTBL));
    slideTmp         = slide;
    currentEndTs     = wSize;
    queueWindow      = (StateTBL *) calloc(DEFAULT_QUEUE_WINDOW_SIZE, sizeof(StateTBL));
  }

  ~Emitter() {
    confirmedTsWorker->clear();
    buffer.clear();
    free(queueWindow);
    //free(memorySendToWork);
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
  try {
    int idWorker = getNextAvailableWorker();
    if (idWorker == -1) return -1;

    if(currentStateTB->getDeletedWin()>0){
      toSend->updateWinListChunkId(toSend->getWinListChunkId()-currentStateTB->getDeletedWin());
    }

    //memorySendToWork[idWorker] = (*toSend);
    workerStatus[idWorker] = 1;

    lb->ff_send_out_to(toSend, idWorker);
    nextWorker++;

    return idWorker;
  } catch (exception &e) {
    cout << "sendToWorker EXCEPTION: " << e.what() << '\n';
  }
  return -1;
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

long int getEndValue() {
  auto retVal = currentQueue->size();// distance(currentQueue->begin(), endPointer);
  return (long int) retVal - 1; //partono da 0
}

/**
Check if something is in the queue and wait for the elaboration
*/
void checkIfSomethingInTheQueue() {
  std::vector<TaskTBL>::iterator it = buffer.begin();
  bool fine = false;
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

static bool comparedDeque(const double &a, const double &b) {
  return a<b;
}

/**
insert the confirmation ts value in the deque if out-of-order
the insertion guarantee to maintain the elements in the deque in order
*/
void insertInConfirmedTsWorker(double elem) {
  bool fine = false;
  confirmedTsWorker->push_back(elem);
  std::sort( confirmedTsWorker->begin() , confirmedTsWorker->end() , comparedDeque );
}

/**
se a partire dal valore che passo trovo i valori successivi devo cancellare fino a quei valori
*/
double cleanInConfirmedTsWorker(double elem) {
  bool fine = false;
  int count = 0;
  double lastVal = elem;
  double nextVal = elem + slide;

  deque<double>::iterator it = confirmedTsWorker->begin();

  for (;fine == false && it != confirmedTsWorker->end(); ) {
    if ((*it) <= nextVal) {
      count++;
      lastVal = nextVal;
      nextVal += slide;
      it = confirmedTsWorker->erase(it);
    } else {
      fine = true;
    }
  }
  return lastVal;
}

queueType::iterator *getClosedWindow(double startWin) {
  return (*currentStateTB->getTupledeque())->getNextAvailableWin(startWin);
}

static bool comparedTaskBuffer(const TaskTBL &a, const TaskTBL &b) {
  if(a.getStartWinValue() < b.getStartWinValue()) return true;
  return false;
}

/**
* If reach a normal tuple add it to the queue.
* This function check if the tuple fire a window
*/
void elaborateCurrentWin() {
  queueType::iterator *windowIT;
  while ((windowIT = getClosedWindow(nextStartingWin)) != NULL) {
    //create new TaskTB to assign to the worker
    double tmpTsStart   = windowIT->getTsStart();
    nextStartingWin += slide;
    RANGE_TYPE tmpWinID = windowIT->getWinId();
    TaskTBL *sendOut;
    WinDes *currentElem = currentStateTB->retrieveWinId(tmpWinID);

    if (currentElem != NULL) {
      sendOut = new TaskTBL(currentStateTB, windowIT, currentElem, tmpTsStart, tmpWinID);
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
    }
    delete windowIT;
  }
  //in case of fewer worker test if the last emitted task is in the buffer queue
  if(windowIT==NULL && EOS_RECEIVE == 1){
    checkIfSomethingInTheQueue();
  }
}

queueType::iterator *getLastWin() {
  return (*currentStateTB->getTupledeque())->getLastWin(nextStartingWin,limitUP);
}

/**
* Retrieve all the last Window iterator that are not fired
*/
void forceCleanQueue(){
  queueType::iterator *windowIT;
  while ((windowIT = getLastWin()) != NULL) {
    TaskTBL *sendOut;

    double tmpTsStart   = windowIT->getTsStart();
    nextStartingWin     += slide;
    RANGE_TYPE tmpWinID = windowIT->getWinId();

    //generate fake win descriptor
    WinDes *currentElem    = new WinDes();
    currentElem->startDesc = windowIT->getTsStart();
    currentElem->endDesc   = windowIT->getTsEnd();
    currentElem->startPos  = windowIT->getRangeStart();
    currentElem->endPos    = windowIT->getRangeEnd();
    currentElem->winId     = tmpWinID;

    nextStartingWin += slide;

    if (currentElem != NULL) {
      sendOut = new TaskTBL(currentStateTB, windowIT, currentElem, tmpTsStart, tmpWinID);
      sendOut->setEOS();

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
    }
    delete windowIT;
  }

  FORCE_CLEAN_QUEUE = true;
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

    //se mi arriva un mess dal worker con valore ts successivo a quello che mi aspettavo devo aspettare ad aggiornare e a cancellare
    if(minOnTheFly == ((*t).getTupleValue())[0]){
      //controllo se mi era già arrivata la conferma da altri worker per  intervalli successivi e nel caso aggiorno i valori
      minOnTheFly = cleanInConfirmedTsWorker(minOnTheFly);

      if(currentQueue->size() > 0) {
        //CLEAN PART
        lastDeleted = (*currentStateTB->getTupledeque())->erase(minOnTheFly);
        lastDeleted = currentStateTB->evictionUpdate(lastDeleted, minOnTheFly); //EVICTION FUNCTION
        //END CLEAN PART
      }
      if (lastDeleted > 0 && (startValue - lastDeleted) > 0) {
        startValue -= lastDeleted;
        lastDeleted = 0;
      }
      minOnTheFly+=slide;
    }else{
      insertInConfirmedTsWorker(((*t).getTupleValue())[0]);
    }

    if(EOS_RECEIVE==1){
      checkIfSomethingInTheQueue();
      if(((*t).getTupleValue())[0]+wSize >= limitUP){
        buffer.clear();
        propagateEOStoWORKER();
      }
    }

    (*t).freeTuple();

  } else {
//[NORMAL TUPLE]***********************************************
    if(currentStateTB->update(t) == 1){
      (*currentStateTB->getTupledeque())->incIsWinClose();
    }
    checkIfSomethingInTheQueue();
    elaborateCurrentWin();
  }

  //(*t).freeTuple();
  delete t;
  return GO_ON;//ff_node_t<tuple_t, TaskTB>::GO_ON;
}

void eosnotify(ssize_t) {
  EOS_RECEIVE = 1;
  elaborateCurrentWin();
  (*currentStateTB->getTupledeque())->close();
  if(FORCE_CLEAN_QUEUE==false){
    if(minOnTheFly+wSize < limitUP){
      forceCleanQueue();
      currentStateTB->closeAllPendentWin();
      checkIfSomethingInTheQueue(); //call it because if forceCleanQueue create the last element add them in the queue
    }
  }
  if (currentQueue->size() == 0 && buffer.size() == 0){ propagateEOStoWORKER(); }
}

void svc_end() {
  checkIfSomethingInTheQueue();
  //free(queueWindow);
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

  Worker(output_t *(*FPOINTER)(FUN_ITERATOR_TYPE ,FUN_ITERATOR_TYPE , output_t *), double limitUP, double wLength, double sliding,
  double initTupleTS, int nworkers, string enableDebug)
  : F(FPOINTER), limitUP(limitUP), wLength(wLength), sliding(sliding), initTupleTS(initTupleTS),
  pardegree(nworkers), enableDebug(enableDebug) {
  }

  int svc_init() {
    workerId = get_my_id();
    return 0;
  }

  tuple_t* createSpeciaTuple(double currentTs, int workerId, double startTs){
    double *value = (double*) calloc(DIM_TUPLE_VALUE,sizeof(double));
    value[0] = startTs;
    for(int i=1;i<DIM_TUPLE_VALUE;i++){  value[i] = -1;   }
    return new tuple_t(currentTs, -1, workerId, value, -1);
  }

  output_t *svc(TaskTBL *t) {
    double currentStartWinVal= 0, currentEndWinVal = 0;
    long int currentWinId = 0;
    WinDes *currentElem;// = (WinDes*)calloc(1,sizeof(WinDes));

    output_t *ret= NULL;// = (output_t*) calloc(1, sizeof(output_t));
    //currentStateTB = (StateTBL*) calloc(1, sizeof(StateTBL));

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
    //printf("--- START FUNCTION [%.2f;%.2f] ---\n",startTs,endTs);
    long long int size = 0;

    queueType::iterator *tmp = (*currentStateTB->getTupledeque())->getITListChunk(startTs, endTs); //retrieve the window iterator
    if( tmp != NULL ) {
      FUN_ITERATOR_TYPE _begin = *tmp->beginIT(); //retrieve the window start pointer iterator
      FUN_ITERATOR_TYPE _end   = *tmp->endIT();   //retrieve the window end pointer
      //memcpy(ret,currentStateTB->getResult(currentStartWinVal, workerId, _begin, _end, currentElem), sizeof(output_t));
      ticks CALCULATION_START = getticks();

      size = distance(_begin,_end);
      ret = currentStateTB->getResult(currentStartWinVal, workerId, _begin, _end, currentElem);

      //ret = new output_t(0, lastIdRes, currentStateTB->getKey(), workerId, startTs, endTs);
      ret->setCalculationTicks(getticks()-CALCULATION_START);
      lastIdRes = ret->getTupleId();
      //delete _begin;
      //delete _end;
      delete tmp;
    }else{
      ++lastIdRes;
      //memcpy(ret,new output_t(0, lastIdRes, currentStateTB->getKey(), workerId, startTs, endTs), sizeof(output_t));
      ret = new output_t(0, lastIdRes, currentStateTB->getKey(), workerId, startTs, endTs);
    }


    //printf("--- END FUNCTION [%.2f;%.2f] ---\n",startTs,endTs);

    if(ret == NULL){
      printf("%s SVC WORKER [%d] RET OUTPUT VAL NULL %s\n",ANSI_BG_COLOR_RED,workerId,ANSI_COLOR_RESET);
    }

    //tuple_t *check = createSpeciaTuple(0, workerId, currentStartWinVal);

    if (limitUP != -1 && ret != NULL && (*ret).getTupleEndInterval() <= limitUP) {
      try {
        ret->setNumElemInWin(size);
        ret->updateWorkerId(workerId);
        ret->updateCurrTs((*ret).getTupleStartInterval());
        #if STATS == 1
        ret->setEmitterLatencyTicks(TICKS_START);
        #endif

        ff_send_out_to(ret, 1);    //send to collector the results
        ff_send_out_to(createSpeciaTuple(0, workerId, currentStartWinVal), 0);  //send to emitter the special tuple
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
