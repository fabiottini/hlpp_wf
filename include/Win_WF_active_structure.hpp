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
*                           WORKER_1
*                         /          \
* GENERATOR --> EMITTER + - WORKER_2 - + COLLECTOR --> <OUTPUT TUPLE>
*                         \   ...    /
*                           WORKER_n
****************************************************************************/

#ifndef WIN_WF_TB_H
#define WIN_WF_TB_H


#define INITIAL_KEYWINSIZE 10
#define DIFF_MARGIN_ERROR 0.99999

#define INTERVAL_AVG_TO_PRINT 5000000 //10 secondi
#define NUM_TUPLE_TO_PRINT 10000 //10000 tuple
#define NUM_SPECIAL_TUPLE_TO_PRINT 10 //10000 tuple

double BOOT_TIMER = 0;
double TUPLE_AVG_INTERARRIVAL = 0;
double SPECIAL_TUPLE_TIME = 0;
long long int COUNT_TUPLE_IN = 0,COUNT_SPECIAL_TUPLE = 0;

using StateTBL = StateTB<FUN_ITERATOR_TYPE>;

/***********************************************************************
*  EMITTER
***********************************************************************/
class Emitter : public ff_node_t<tuple_t, tuple_t> {
private:
  double wSize, slide;
  int first_id, last_id; // first_id & last_id = first_id|last_id widow in witch
  // tuple belong
  int workerId = 0, max_worker = 4; // utilized in ff_send_out_to

  ff_loadbalancer *lb; // i need it to user ff_send_out_to

  int pardegree = 0;
  unsigned int *to_workers;
  int i = 0, totalCopies = 0, countWorkers = 0;

  double ts = 0, *value;
  int idKey = 0, idT = 0;

  unsigned int startWorkerId;

  string enableDebug;

  double media=0;
  int num = 0;

  int EOS_RECEIVED = 0;

  double lastTsTupleValue=0, limitUP = 0;

  output_t* (*F)(FUN_ITERATOR_TYPE,FUN_ITERATOR_TYPE,  output_t*);

public:

  #if INCREMENTAL_QUERY==1
  Emitter(output_t* (*FPOINTER)(FUN_ITERATOR_TYPE, FUN_ITERATOR_TYPE, output_t*),ff_loadbalancer *const lb, int pardegree, double wSize, double slide, string enableDebug)
  : F(FPOINTER), lb(lb), pardegree(pardegree), wSize(wSize), slide(slide), enableDebug(enableDebug) {

    #ifdef _STATS
    BOOT_TIMER = getusec(); //initialize the timer
    #endif

    assert(lb != null);
    assert(pardegree >= 1);

    to_workers = new unsigned int[pardegree];
  }

  #else
  Emitter(output_t* (*FPOINTER)(FUN_ITERATOR_TYPE,FUN_ITERATOR_TYPE, output_t*),ff_loadbalancer *const lb, int pardegree, double wSize, double slide, double _limitUP, string enableDebug)
  : F(FPOINTER), lb(lb), pardegree(pardegree), wSize(wSize), slide(slide), enableDebug(enableDebug), limitUP(_limitUP) {

    #ifdef _STATS
    BOOT_TIMER = getusec(); //initialize the timer
    #endif

    assert(lb != nullptr && lb != NULL);
    assert(pardegree >= 1);

    to_workers = new unsigned int[pardegree];

  }
  #endif

  ~Emitter(){
    delete [] to_workers;
  }


  bool sendTuple (tuple_t* t,bool isSpecialTuple){
    if(isSpecialTuple)   idKey =1;

    ts    = (*t).getTupleTimeStamp();
    idKey = (*t).getTupleKey();
    idT   = (*t).getTupleId();
    if(!isSpecialTuple) value = (*t).getTupleValue(); else value = nullptr;

    if (ts < (wSize))
    first_id = 1; // se ts < wSize es. 1.5 < 5 sec
    else
    first_id = ceil(((double)(ts - (wSize))) / ((double)slide)) + 1;

    last_id = ceil(((double)ts / slide));

    // evaluate the workers affected by the tuple
    workerId = first_id;
    countWorkers = 0;

    startWorkerId = (idKey - 1) % pardegree;

    i = first_id;
    last_id = (last_id < first_id) ? first_id : last_id;

    //decide to witch worker send the data
    while ((i <= last_id) && (countWorkers < pardegree)) {
      to_workers[countWorkers] = (startWorkerId + (i - 1)) % pardegree; // short transient phase with bounded queues
      countWorkers++;
      i++;
    }

    t->setTupleEmitterTick(); //set the tick before

    if(!isSpecialTuple) {
      if(!isSpecialTuple) {
        t->setWorkers(countWorkers); //update the tuple worker number
      }
      // update the statistics for the number of copies per tuple
      totalCopies += countWorkers;

      for (int i = 0; i < countWorkers; i++) {
        /*
        string valueStr;
        for (int i = 0; i < (sizeof(value) / sizeof(double)); i++) {
          valueStr += std::to_string(value[i]) + ", ";
        }
        if (ifDebug(enableDebug, "2")) {
          printf("*> E: {ID: %d, TS: %.2f, VAL: %s} -> W:%d\n", idT, ts, valueStr.c_str(), to_workers[i]);
        }
        */
        while (!lb->ff_send_out_to(t, to_workers[i]));      // SEND
      }
    }else{
      while (!lb->ff_send_out_to(t, to_workers[countWorkers-1]));
    }

    return true;
  }

  tuple_t *svc(tuple_t *t) {
    lastTsTupleValue = (*t).getTupleTimeStamp();
    sendTuple(t,false);
    return GO_ON;
  }

  void propagateEOStoWORKER() {
    for (int i = 0; i < pardegree; i++) {
      lb->ff_send_out_to(EOS, i);
    }
  }

  void eosnotify(ssize_t) {
    if(lastTsTupleValue<limitUP-slide){
      tuple_t *tmpTuple = new tuple_t();
      sendTuple(tmpTuple->createSpeciaTuple(limitUP,-1,limitUP),true);
      delete tmpTuple;
    }
    EOS_RECEIVED = 1;
    propagateEOStoWORKER();
  }

  void svc_end(){
    #ifdef _STATS
    double currentTime = getusec()-BOOT_TIMER;
    printf("-> EMITTER TIME: %.2f sec. %.2f ms \n" ,FROM_MICRO_TO_SEC(currentTime) , FROM_MICRO_TO_MILLISEC(currentTime) ) ;
    #endif
  }

};


/***********************************************************************
*  WORKER
***********************************************************************/
class Worker : public ff_node_t<tuple_t, output_t> {
private:
  double wLength = 0, sliding = 0, resultFunction = 0;
  int insPos = 0, idRes = 0;
  string enableDebug;

  //Associate the window to the key
  StateTBL* main   = NULL;
  int *keyInWindow = NULL;
  int lastWinAdd = 0;
  long int MAX_KEYWINSIZE = INITIAL_KEYWINSIZE;

  int SPECIAL_TUPLE_ARRIVED = 0;
  double SPECIAL_TUPLE_startTs = 0, SPECIAL_TUPLE_endTs = 0;

  double lastStartTsSent   = 0,
  nextStartTsToSend = 0;

  double limitUP = 0;
  //AUX pointer to the structure
  StateTBL *currentStateTB = (StateTBL*) calloc(1,sizeof(StateTBL));
  deque<tuple_t> *currentQueue = NULL;

  output_t* (*F)(FUN_ITERATOR_TYPE,FUN_ITERATOR_TYPE, output_t*);

  double currentStart_ts = 0, currentEnd_ts = 0, slidingFactor = 0, nextInitWinVal = 0, initTupleTS = 0;
  int currentKey = 0, pardegree = 0, workerId = 0;
  #if STATS == 1
  double  BOOT_TIME = 0,
  TIME_winStart=0,
  TIME_winEnd=0;
  #endif
  /**
  * given an integer key allocate such window as necessary
  * @param  key the integer
  * @return     the pointer to the window structure
  */

  StateTBL* allocWin(double winLength, double slidingFactor, double initWindow, int currentKey){
    main = (StateTBL*)   realloc( main, (currentKey+1)*sizeof(StateTBL) );
    StateTBL *currentStateTB  = new StateTBL(F,workerId,currentKey,initWindow, winLength, slidingFactor,new queueType );
    main[currentKey]  = *currentStateTB;
    return currentStateTB;
  }

public:

  Worker(output_t* (*FPOINTER)(FUN_ITERATOR_TYPE,FUN_ITERATOR_TYPE, output_t*),double limitUP, double wLength, double sliding, double initTupleTS, int nworkers, string enableDebug)
  : F(FPOINTER), limitUP(limitUP), wLength(wLength), sliding(sliding), initTupleTS(initTupleTS), pardegree(nworkers), enableDebug(enableDebug) {
    main        = (StateTBL*) calloc( INITIAL_KEYWINSIZE, sizeof(StateTBL) );
    keyInWindow = (int*)      calloc( INITIAL_KEYWINSIZE, sizeof(int) );

  }

  ~Worker(){
    if(main == NULL) return ;
    free(main);
    free(keyInWindow);
    free(currentStateTB);
  }

  /**
  * Remove the tuple in deque in the Status that are older tha ts value
  * @param  ts [description]
  * @return    [description]
  */
  long int eviction(queueType *tmpQ, double ts) {
    if (tmpQ == NULL) return 0;

    long int deletedTuples = 0;
    bool end = false;
    queueType::iterator it = tmpQ->begin();

    double startTMP, endTMP;
    int inTheQueueBEF, inTheQueueAFT;

    if((it != tmpQ->end())){
      startTMP = (*it).getTupleTimeStamp();
      inTheQueueBEF = tmpQ->size();
    }

    while(it != tmpQ->end() && end == false) {
      if ((*it).getTupleTimeStamp() <= ts) {
        //printf("DELETE:  ts: %.2f  >  %.2f\n", ts, (*it).getTupleTimeStamp());
        //delete (*it).getTupleValue();

        tmpQ->erase(it);
        //tmpQ->pop_front();
        if(it != tmpQ->end()) it++;
        deletedTuples++;
      } else {
        end = true;
      }
    }
    //tmpQ->shrink_to_fit();
    //printf("AFTER delete: %.2f  %lu \n",(*it).getTupleTimeStamp(),tmpQ->size());

    if((it != tmpQ->end())){
      endTMP = (*it).getTupleTimeStamp();
      inTheQueueAFT = tmpQ->size();
    }
    return deletedTuples;

  }

  int svc_init() {
    workerId          = get_my_id();
    nextStartTsToSend = workerId*sliding;
    #if STATS == 1
    BOOT_TIME = getusec();
    #endif
    return 0;
  }


  output_t *svc(tuple_t *t) {

    if(!(*t).isSpecialTuple()){

      currentKey = (*t).getTupleKey();
      // IF the key of the tuple is NEW => allocate all the w
      if ( keyInWindow[currentKey] == 0 ) {
        slidingFactor     = pardegree * sliding;
        double initWindow = workerId*sliding;

        if(MAX_KEYWINSIZE<currentKey){ //se la chiave della tupla è maggiore di quelle istanziate
          currentStateTB   = allocWin(wLength, slidingFactor,initWindow, currentKey); //allocation OR reallocation of the window
          keyInWindow     = (int*)realloc( keyInWindow, (currentKey+1)* sizeof(int) );
          MAX_KEYWINSIZE  = currentKey+1;
        }else{
          if(currentStateTB!=NULL){
            free(currentStateTB);
          }
          currentStateTB     = new StateTBL(F,workerId,currentKey,initWindow, wLength, slidingFactor, new queueType);
          main[currentKey]  = *currentStateTB;
        }

        // INIT THE WINDOW
        currentStart_ts = currentStateTB->getInitInterval(),
        currentEnd_ts   = currentStateTB->getEndInterval();

        keyInWindow[currentKey] = 1;
      }

      // get the queue of the window
      currentQueue = *(currentStateTB->getTupledeque());

      // BEFORE INSERT IN THE QUEUE CHECK IF THERE ARE OTHER ELEMENT IN THE QUEUE
      output_t* ret = NULL;

      currentStateTB->update(t);

      winDesType::iterator *itCurrent;// = (winDesType::iterator*)calloc(1,sizeof(winDesType::iterator));
      while ( (itCurrent=currentStateTB->getClosedWindow()) != nullptr ){

        #if _DEBUG == 2
        cout << "(*t).ts: " << (*t).getTupleTimeStamp() << "; currentEnd_ts: " << currentEnd_ts << endl;
        #endif
        ticks CALCULATION_START = getticks();

        ret = doOnTrigger((*t).getTupleTimeStamp(),itCurrent);
        free(itCurrent);

        while(ret->getTupleStartInterval() > nextStartTsToSend){
          output_t* tmpRet = new output_t((*t).getTupleTimeStamp(), ++lastIdRes, ret->getTupleKey(), workerId, nextStartTsToSend, nextStartTsToSend+wLength);
          if(limitUP != -1 && ret != NULL && (*ret).getTupleEndInterval() <= limitUP){
            ff_send_out(tmpRet);
          }
          lastStartTsSent   = nextStartTsToSend;
          nextStartTsToSend = lastStartTsSent+slidingFactor;
        }

        if(limitUP != -1 && ret != NULL && (*ret).getTupleEndInterval() <= limitUP){
          ret->updateWorkerId(workerId);
          ret->setEmitterLatencyTicks(t->getTupleEmitterTick());
          //ret->updateCurrTsMicro(getusec());
          ret->setCalculationTicks(getticks()-CALCULATION_START);

          ff_send_out(ret);
          // printf("  >>inviata: %s\n",ret->toString().c_str());
          //(*itCurrent)->sent = true;
        }
        lastStartTsSent   = (*ret).getTupleStartInterval();
        nextStartTsToSend = lastStartTsSent+slidingFactor;
      }

      if((*t).decWorkersCheck()) {           // decrement of the worker number in the tuple and remove it if is the last
        //printf("w: %d %.2f free\n",workerId,(*t).getTupleTimeStamp());
        delete t;
      }

    }else{
      SPECIAL_TUPLE_ARRIVED = 1;
      double *tsValue = t->getTupleValue();
      SPECIAL_TUPLE_startTs = tsValue[0]-wLength;
      SPECIAL_TUPLE_endTs = tsValue[0];
      delete t;
    }

    return GO_ON;
  }

  /// elaborate the last element in the window
  void eosnotify(ssize_t) {

    if (ifDebug(enableDebug, "3")) {
      printf(ANSI_BG_COLOR_RED "EOS NOTIFY START %s\n", ANSI_COLOR_RESET);
    }

    if(SPECIAL_TUPLE_ARRIVED==1){
      currentStateTB->generateFAKEWinDes(SPECIAL_TUPLE_startTs,SPECIAL_TUPLE_endTs);
    }
    if(currentQueue == NULL) return ;
    if(currentQueue->end() == currentQueue->begin()) return;


    output_t* ret = NULL;
    winDesType::iterator *itCurrent; //(winDesType::iterator*)calloc(1,sizeof(winDesType::iterator));
    while ((itCurrent = currentStateTB->getPendentWin())!=NULL){
      ticks CALCULATION_START = getticks();

      ret = doOnTrigger(0,itCurrent);

      if(ret != NULL && (*ret).getTupleEndInterval() <= limitUP){
        ret->setCalculationTicks(getticks()-CALCULATION_START);

        ret->setEmitterLatencyTicks(((*currentStateTB->getTupledeque())->begin())->getTupleEmitterTick());

        ff_send_out(ret);
        (*itCurrent)->sent = true;
      }
    }

    #if INCREMENTAL_QUERY==1
    currentStateTB->eviction(currentEnd_ts);
    #endif

    if(main != nullptr && main != NULL) {
      free(main);
      free(keyInWindow);
      delete currentStateTB;
      main = nullptr;
      currentStateTB = nullptr;
    }

    if (ifDebug(enableDebug, "3")) {
      printf(ANSI_BG_COLOR_RED "W[%d] EOS NOTIFY END %s\n", workerId, ANSI_COLOR_RESET);
    }

  }

  /**
  * the core of the worker
  * apply the user function and update the ouput and delete the oldest tuple
  * @param  currentTs [description]
  * @param  idW       [description]
  * @return           [description]
  */

  long long int lastIdRes = 0;
  output_t* doOnTrigger(double currentTs, winDesType::iterator *itWin){

    output_t* ret = NULL;
    int deleted = 0;
    WinDes * currentElem = &(**itWin);
    currentStart_ts = currentElem->startDesc;
    currentEnd_ts   = currentElem->endDesc;
    long int startPos = currentElem->startPos,
    endPos   = currentElem->endPos;


    currentEnd_ts -= DIFF_UNIT;

    FUN_ITERATOR_TYPE tmpStart = ((*currentStateTB->getTupledeque())->begin());
    FUN_ITERATOR_TYPE tmpEnd = ((*currentStateTB->getTupledeque())->end());


    //adjust the starting pointer
    while ((*tmpStart).getTupleTimeStamp() < (currentStart_ts) && tmpEnd != tmpStart) {
      ++tmpStart;
    }
    //        if((*tmpStart).getTupleTimeStamp() > currentStart_ts) { --tmpStart; }

    //adjust the ending pointer
    --tmpEnd;

    if((*tmpEnd).getTupleTimeStamp() > (currentEnd_ts+DIFF_MARGIN_ERROR)) {
      while ((*tmpEnd).getTupleTimeStamp() >= (currentEnd_ts+DIFF_MARGIN_ERROR) && tmpEnd != tmpStart) {
        --tmpEnd;
      }
    }

    if((*tmpEnd).getTupleTimeStamp() < (currentEnd_ts+DIFF_MARGIN_ERROR)) { ++tmpEnd; }

    ret = currentStateTB->getResult(currentStart_ts,workerId,tmpStart,tmpEnd,currentElem);

    if(ret==NULL){
      ret = new output_t(0, ++lastIdRes, currentStateTB->getKey(), workerId, NULL, currentStart_ts, currentEnd_ts);
    }else{
      lastIdRes = ret->getTupleId();
    }

    ret->setNumElemInWin(distance(tmpStart,tmpEnd));
    ret->updateCurrTs(currentTs);

    //DELETE OLD WINDOW VALUE (the values in the window less than currentStart_ts)
    if(currentStateTB->getElementInTheQueue()>0){
      #if INCREMENTAL_QUERY==0
      deleted  = eviction(*currentStateTB->getTupledeque(),currentStart_ts);
      deleted  = currentStateTB->evictionUpdate(deleted, currentStart_ts);
      #endif

      #if _DEBUG == 1
      cout << "DELETE: " << currentStart_ts << endl;
      #endif
    }
    return ret;
  }
};


#endif
