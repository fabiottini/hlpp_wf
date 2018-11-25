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

#define ANSI_BG_COLOR_RED "\x1b[41m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define FROM_SEC_TO_MICRO(sec) sec*1000000
#define FROM_MICRO_TO_SEC(sec) sec/1000000
#define FROM_MILLISEC_TO_MICRO(sec) sec*1000

#define FROM_MICRO_TO_MILLISEC(sec) sec/1000



/****************************************************************************
 *  Descriptor of the window
 ****************************************************************************/
struct WinDes : public WindowDescriptor<double> {
public:
    long int startPos = 0,  //init position in the queue (queueType  *queue) in State
            endPos = 0,  //end  position in the queue (queueType  *queue) in State
            winId = 0;  //the  id of the window descriptor defined as the position in the queue
    double startDesc = 0,  //the  ts value start the interval in the queue (queueType  *queue) in State
            endDesc = 0;  //the  ts value end   the interval in the queue (queueType  *queue) in State
    bool isClosed = false,  //TRUE if the win is closed => no other element enter in the interval
            used = false,  //TRUE if the worker use it and apply the funcion on it
            sent = false;  //TRUE if after the evaluation the data is sent

    string toString() {
        char buffer[500];
        sprintf(buffer, "{ startPos: %ld; endPos: %ld; winId: %ld; startDesc: %.2f; endDesc: %.2f }\n",
                startPos, endPos, winId, startDesc, endDesc);
        return buffer;
    }

    WinDes() {}
};


/****************************************************************************
 *  State
 ****************************************************************************/
using winDesType = std::deque<WinDes>;

template <typename X>
class StateTB : public State<double, double, output_t> {
private:

    double start_ts, end_ts, sliding, slidingFactor, winLength, initWin, lastInsert=0;
    long long int countTupleIN = 0; //for the array version
    RANGE_TYPE start_pos, end_pos;
    int key;
    long int deletedElement = 0;
    queueType  *queue;//new queueType;
    winDesType *queueWin;
    winDesType::iterator queueWinIT;
    long long int elementInWin = 0;
    int idRes = 0;
    int workerId;

    WinDes *curWinDesc;
    bool startNewWin = true;
    long int countIdWin = 0;
    bool isFirst = true;
    int countOpenWindow = 0;
    int countCloseWindow = 0;
    long int deletedWin = 0;

    deque<output_t> *currentIncrementalData; //maintain the output_t element to update in the incremental query
    //#if INCREMENTAL_QUERY==1
    output_t *(*F)(X ,X , output_t *) = NULL;
    //#endif
public:
    long int fromIdToStartPos(WinDes *elem) {
        if (elem == NULL) return -1;
        return elem->startPos;
    }

    long int fromIdToEndPos(WinDes *elem) {
        if (elem == NULL) return -1;
        return elem->endPos;
    }

    double fromIdToStartValue(WinDes *elem) {
        if (elem == NULL) return -1;
        return elem->startDesc;
    }

    double fromIdToEndValue(WinDes *elem) {
        if (elem == NULL) return -1;
        return elem->endDesc;
    }

    long int getDeletedWin(){ return deletedWin; }

    ~StateTB() {
        free(queue);
        delete queueWin;
        curWinDesc = nullptr;
        queue = nullptr;
        queueWin= nullptr;
    }

    //Fake constructor
    StateTB() {
        start_ts = 0;
        end_ts = 0;
        sliding = 0;
        slidingFactor = 0;
        key = -1;
        queue = nullptr;
        queueWin= nullptr;
    }

    StateTB(output_t *(*FPOINTER)(X ,X , output_t *), int workerId, int currentKey, double initWin, double winLength, double sliding, queueType* _queue )
            : F(FPOINTER), workerId(workerId), key(currentKey), initWin(initWin), winLength(winLength),
              sliding(sliding) {
        queue = (queueType*)calloc(1, sizeof(queueType));
        queueWin = new winDesType;

        memcpy(queue,_queue, sizeof(queueType));
        start_ts = initWin;
        end_ts = initWin + winLength;

        #if INCREMENTAL_QUERY == 1
         currentIncrementalData = new deque<output_t>;
        #endif
        //queue = (queueType*)calloc(1, sizeof(queueType));
        //queue = new queueType(start_ts, sliding, winLength);
    }


    //retrieve the instance value
    int getKey() { return this->key; }

    double getInitInterval() { return this->start_ts; }

    double getEndInterval() { return this->end_ts; }

    double getStartPointer() { return this->start_pos; }

    double getEndPointer() { return this->end_pos; }

    double getSliding() { return this->sliding; }

    double getWLen() { return this->winLength; }

    double getDeletedElement() { return this->deletedElement; }

    queueType **getTupledeque() { return &(this->queue); }

    /**
     * Print the representation of the State
     */
    void print() {
        printf("\t{ key: %d; start_ts: %.2f; end_ts: %.2f; sliding: %.2f; queue: %d }\n",
               key, start_ts, end_ts, sliding, (int) queue->size());
    }

    /**
     * A string representation of the State
     * @return string
     */
    string toString() {
        char buffer[500];
        sprintf(buffer, "{ key: %d; start_ts: %.2f; end_ts: %.2f; sliding: %.2f; queue: %d }\n",
                key, start_ts, end_ts, sliding, (int) queue->size());
        return buffer;
    }


    /**************************** UPDATE ****************************/
    int updateARRAY(tuple_t *tuple) {
        //(*tuple).print();

        int retVal = 0;
        queue->push_back( *(tuple) );
        countTupleIN++;


        if(countTupleIN+1 >= sliding || isFirst==true){
            if ( isFirst == false) {
                end_ts=lastInsert;
                curWinDesc->endDesc = lastInsert;
                curWinDesc->isClosed = true;
                countCloseWindow++;
                startNewWin = true;
                retVal = 1;
            }

            if (startNewWin) { //is a new window
                countOpenWindow++;
                queueWin->push_back(*(new WinDes()));
                curWinDesc = &queueWin->back();

                if (isFirst) { //if is the first time start from 0
                    curWinDesc->startDesc = start_ts;
                    curWinDesc->startPos = curWinDesc->endPos = 0;
                    curWinDesc->winId = 0;
                } else {
                    curWinDesc->startDesc = lastInsert;
                    curWinDesc->startPos = curWinDesc->endPos = queue->size();
                    curWinDesc->winId = countOpenWindow-1;
                }
                curWinDesc->endDesc = curWinDesc->startDesc + winLength;

                start_ts  = curWinDesc->startDesc;
                end_ts    = lastInsert;//curWinDesc->endDesc;
                start_pos = curWinDesc->startPos;
                end_pos   = curWinDesc->endPos;

                // printf("CREA NUOVA WIN: [%.2f;%.2f]\n",start_ts,end_ts);
    #if _DEBUG == 3
                cout << "WIN interval: [" << start_ts << "; " << end_ts << "] " <<  "  POS: " << "[" << start_pos << "; " << end_pos << "]" << endl;
    #endif
                isFirst = false;
                startNewWin = false;
                if(!isFirst){
                    //cout << "curWinDesc->endPos: " << curWinDesc->endPos << endl;
                    //cout << "TUPLE IN: " << countTupleIN << endl;
                     countTupleIN = 0;
                }
            } else {  //update the current window
                curWinDesc->endPos = this->queue->size()-1;//distance(this->queue->begin(),this->queue->end());
            }
        }
        

        if(!isFirst) lastInsert = tuple->getTupleTimeStamp();
        return retVal;
    }

    int update(tuple_t *tuple) {
        //(*tuple).print();

        int retVal = 0;
        queue->push_back( *(tuple) );


        //if tuple ts is over the win ts and not the first
        //printf("%.2f > %.2f\n",tuple->getTupleTimeStamp(),end_ts);
        while( tuple->getTupleTimeStamp() > end_ts ){
          //printf("%f > %f\n",tuple->getTupleTimeStamp(),end_ts);
            if ( tuple->getTupleTimeStamp() > end_ts && isFirst == false) {
                curWinDesc->isClosed = true;
                countCloseWindow++;
                startNewWin = true;
                retVal = 1;
            }

            if (startNewWin) { //is a new window
                countOpenWindow++;
                queueWin->push_back(*(new WinDes()));
                curWinDesc = &queueWin->back();

                if (isFirst) { //if is the first time start from 0
                    curWinDesc->startDesc = start_ts;
                    curWinDesc->startPos = curWinDesc->endPos = 0;
                    curWinDesc->winId = 0;
                } else {
                    curWinDesc->startDesc = start_ts + sliding;
                    curWinDesc->startPos = curWinDesc->endPos = queue->size();
                    curWinDesc->winId = countOpenWindow-1;
                }
                curWinDesc->endDesc = curWinDesc->startDesc + winLength;

                start_ts  = curWinDesc->startDesc;
                end_ts    = curWinDesc->endDesc;
                start_pos = curWinDesc->startPos;
                end_pos   = curWinDesc->endPos;

               // printf("CREA NUOVA WIN: [%.2f;%.2f]\n",start_ts,end_ts);
    #if _DEBUG == 3
                cout << "WIN interval: [" << start_ts << "; " << end_ts << "] " <<  "  POS: " << "[" << start_pos << "; " << end_pos << "]" << endl;
    #endif
                isFirst = false;
                startNewWin = false;

    #if INCREMENTAL_QUERY == 1
                output_t res = *new output_t(tuple->getTupleTimeStamp(),idRes,getKey(),workerId,NULL,start_ts,end_ts);
                currentIncrementalData->push_back(res); //create the element in the dequeue
    #endif

            } else {  //update the current window
    #if INCREMENTAL_QUERY == 1
                curWinDesc->endPos  = this->queue->size();//distance(this->queue->begin(),this->queue->end());
                currentIncrementalData->at((curWinDesc->winId)-1) = *getFunctionRet(curWinDesc->winId);
    #else
                curWinDesc->endPos = this->queue->size()-1;//distance(this->queue->begin(),this->queue->end());
    #endif

            }
        }

        //control the case of incremental query and not necessary the old data to evaluate the current value
#if MAINTAIN_TUPLE == 0 && INCREMENTAL_QUERY == 1
        this->queue->pop_back(); //remove the last element (in theory the only element in the queue)
        curWinDesc->endPos   = distance(this->queue->begin(),this->queue->end());
        curWinDesc->startPos = curWinDesc->endPos-1;
        curWinDesc->startPos = (curWinDesc->startPos<=0)?0:curWinDesc->startPos;
        curWinDesc->winId    = distance(this->queueWin->begin(),this->queueWin->end());
#endif

//cout << curWinDesc->toString().c_str() << endl;


        return retVal;
    }

    /**************************** END UPDATE ****************************/

    void generateFAKEWinDes(double _startTs, double _endTs){
        if (curWinDesc!=NULL) {
            curWinDesc->isClosed = true;
            countCloseWindow++;
        }

        countOpenWindow++;
        if(queueWin == nullptr || queueWin==NULL){
            queueWin = new winDesType;
        }
        queueWin->push_back(*(new WinDes()));
        curWinDesc = &queueWin->back();

        if (isFirst) { //if is the first time start from 0
            curWinDesc->startDesc = _startTs;
            curWinDesc->startPos = curWinDesc->endPos = 0;
            curWinDesc->winId = 0;
        } else {
            curWinDesc->startDesc = _startTs;
            if(queue!= nullptr){
                curWinDesc->startPos = curWinDesc->endPos = queue->size();
            }else{
                curWinDesc->startPos = curWinDesc->endPos = 0;
            }
            curWinDesc->winId = countOpenWindow-1;
        }
        curWinDesc->endDesc = _endTs;

        start_ts  = curWinDesc->startDesc;
        end_ts    = curWinDesc->endDesc;
        start_pos = curWinDesc->startPos;
        end_pos   = curWinDesc->endPos;

        //printf("+++ CREA NUOVA FAKE WIN: [%.2f;%.2f]\n",start_ts,end_ts);
        isFirst = false;
        startNewWin = false;
        if(queue!= nullptr) {
            curWinDesc->endPos = this->queue->size() - 1;
        }else{
            curWinDesc->endPos = 0;
        }
    }


    deque<WinDes> *retrieveQueueWin() {
        return queueWin;
    }

    /**
     * Return the WindowDescriptor
     * @param  wID [description]
     * @return     [description]
     */
    WinDes *retrieveWinId(RANGE_TYPE wID) {
        WinDes *ret = NULL;
        if(!queueWin){return NULL; }
        bool end = false;
        int i = 0;
        for (winDesType::iterator it = queueWin->begin(); it != queueWin->end() && end == false; ++it) {
            if ((*it).winId == wID) {
                ret = &(*it);
                end = true;
            }
            i++;
        }
        return ret;
    }

    WinDes *retrieveWinId(double winTs) {
        if(!queueWin){return NULL; }
        if(queueWin->size()==0){ return  NULL; }
        winDesType::iterator it = queueWin->begin();
        WinDes *ret = NULL;
        bool end = false;
        double lastTs = 0;

        if(winTs == 0 && (*it).startDesc > winTs){
            printf( "%s retrieveWinId searchFor: %.2f  first element ts: %.2f %s\n",ANSI_BG_COLOR_RED,winTs,(*it).startDesc,ANSI_COLOR_RESET);
            return ret;
        }

        for (; it != queueWin->end() && end == false; ++it) {
            if ((*it).startDesc == winTs) {
                ret = &(*it);
                end = true;
            }
            lastTs = (*it).startDesc;
        }
        //printf( "retrieveWinId searchFor: %.2f - lastTS: %.2f\n",winTs,lastTs);
        return ret;

    }


    void setWinAsSent(double winTs){
        WinDes *tmp = retrieveWinId(winTs);
        if(tmp != NULL) tmp->sent = true;
    }

    /**
     * Return the position of the first window that is closed but not used
     * @return [description]
     */
    winDesType::iterator *getClosedWindow(){
      if(countCloseWindow==0){return nullptr;}
      if(queueWin->size()<=0){return nullptr;}
      if(countOpenWindow > 0){ //if there are some closed win
        long int pos = 0;
        bool end  = false;
        winDesType::iterator *it = (winDesType::iterator*) calloc(1,sizeof(winDesType::iterator));
        for (*it = queueWin->begin(); *it!=queueWin->end() && end == false; ++*it){
          if((**it).isClosed && !(**it).used && !(**it).sent){ //se è chiuso ma non assegnato a nessun worker
            (**it).winId = pos;  //set the id that i return to the caller it helps me to recover the window in the datastructure
            (**it).used  = true; //set use to signal that another worker work on it
             // printf("%ld)it: %s\n",pos,(**it).toString().c_str());
            end = true;
            countCloseWindow--;
            return it;
          }
          pos ++;
        }
        free(it);
      }
      return nullptr;
    }


    /**
     * Return the position of the first position window NOT used
     * and set used status true
     * @return [description]
     */
    winDesType::iterator *getPendentWin() {
        if (queueWin->size() <= 0) { return NULL; }
        if (countOpenWindow <= 0) { return NULL; }
        long int pos = 0;
        bool end = false;
        winDesType::iterator *it = (winDesType::iterator*) calloc(1,sizeof(winDesType::iterator));

        for (*it = queueWin->begin(); *it != queueWin->end() && end == false; ++*it) {
            if (!(**it).used) { //se non e' stato assegnato a nessun worker
                (**it).winId = pos; //set the id that i return to the caller it helps me to recover the window in the datastructure
                (**it).used = true; //set use to signal that another worker work on it
                end = true;
                return it;
            }
            pos++;
        }
        return NULL;
    }

    void closeAllPendentWin() {
        if (queueWin->size() <= 0) { return; }
        if (countOpenWindow <= 0) { return; }
        long int pos = 0;
        for (winDesType::iterator it = queueWin->begin(); it != queueWin->end(); ++it) {
            if (!(*it).isClosed) { //se non e' stato assegnato a nessun worker
                (*it).winId = pos; //set the id that i return to the caller it helps me to recover the window in the datastructure
                (*it).isClosed = true;
            }
            pos++;
        }
    }

    /**
     * Return the position of the first position window NOT used
     * doesn't set used var true
     * @return [description]
     */
    long int getPendentWinNOLOCK() {
        if (queueWin->size() <= 0) { return -1; }
        if (countOpenWindow <= 0) { return -1; }
        long int pos = 0;
        bool end = false;
        for (winDesType::iterator it = queueWin->begin(); it != queueWin->end() && end == false; ++it) {
            if (!(*it).used) { //se non e' stato assegnato a nessun worker
                (*it).winId = pos; //set the id that i return to the caller it helps me to recover the window in the datastructure
                return pos;
            }
            pos++;
        }
        return -1;
    }

    void printPendentWinNOLOCK() {
        if (queueWin->size() <= 0) { return; }
        if (countOpenWindow <= 0) { return; }
        long int pos = 0;
        bool end = false;
        for (winDesType::iterator it = queueWin->begin(); it != queueWin->end() && end == false; ++it) {
            if (!(*it).used) { //se non e' stato assegnato a nessun worker
                (*it).winId = pos; //set the id that i return to the caller it helps me to recover the window in the datastructure
                cout << "WIN: " << (*it).toString().c_str() << endl;
            }
            pos++;
        }
    }

    /**
     * Return the tuple result
     * - in the incremental query it is evaluate eachtime
     * - in the non incremental query it is evaluate once
     * @param  idWindow [description]
     * @return          [description]
     */
    output_t *getFunctionRet(double winTsStart, int workerId, X isStart, X isEnd, WinDes *currentElem) {
        long int start = 0, end = 0;
        double currentTs = 0, startTs = 0, endTs = 0;

        if (currentElem == NULL) {
            printf("%s  W[%d]  getFunctionRet => NULL DATA %s\n",ANSI_BG_COLOR_RED,workerId,ANSI_COLOR_RESET);
            return NULL;
        }

        start = fromIdToStartPos(currentElem);
        end = fromIdToEndPos(currentElem);
        startTs = fromIdToStartValue(currentElem);
        endTs = fromIdToEndValue(currentElem);

        currentElem->used = true;
        currentElem->sent = true;
        output_t *ret;

#if INCREMENTAL_QUERY == 1

#else
        ret = new output_t(currentTs, idRes, getKey(), workerId, startTs, endTs);
#endif
        ret = F(isStart,isEnd,ret);

        idRes++;

        return ret;
    }

     output_t *getFunctionRetIdWin(double winTsStart, int workerId, X isStart, X isEnd, WinDes *currentElem, long long int idWin) {
        long int start = 0, end = 0;
        double currentTs = 0, startTs = 0, endTs = 0;

        start = (*isStart).getTupleId();
        end   = (*isEnd).getTupleId();
        startTs = (*isStart).getTupleTimeStamp();
        endTs = (*isEnd).getTupleTimeStamp();

        output_t *ret = new output_t(endTs, idWin, getKey(), workerId, startTs, endTs);
        ret = F(isStart,isEnd,ret);

        return ret;
    }


    /**
     * IN THE INCREMENTAL_QUERY
     * Return the current output for the window
     * @param  idWindow [description]
     * @return          [description]
     */
    output_t *getIncResultQuery(long int idWindow) {
        //printCurrentIncrementalData();
        return &currentIncrementalData->at(idWindow);
    }

    /**
     * Return the output evaluate for the window
     * @param  idWindow [description]
     * @return          [description]
     */
    output_t *getResult(double winTsStart, int workerId, X itStart, X itEnd, WinDes *currentElem) {
#if INCREMENTAL_QUERY == 1
        return getIncResultQuery(idWindow);
#else
        return getFunctionRet(winTsStart,workerId, itStart, itEnd, currentElem );
#endif
    }

    output_t *getResultIDWin(double winTsStart, int workerId, X itStart, X itEnd, WinDes *currentElem, long long int idWin) {

        return getFunctionRetIdWin(winTsStart,workerId, itStart, itEnd, currentElem,idWin );
    }

    /**
     * print the list of the deque that maintain the output tuple to send
     */
    void printCurrentIncrementalData() {
        int i = 0;
        for (std::deque<output_t>::iterator it = currentIncrementalData->begin();
             it != currentIncrementalData->end(); ++it) {
            cout << i << ")" << (*it).toString().c_str() << endl;
            i++;
        }
    }

    /**************************** EVICTION ****************************/
    /**
     * Remove old tuple from the deque in the status and update the data in the windows deque
     * @param  ts [description]
     * @return    [description]
     */

    long int evictionUpdate(long int lastDeleted, double ts) {
       // long int lastDeleted = eviction(ts);
        int i = 0;
#if INCREMENTAL_QUERY == 1
        std::deque<output_t>::iterator it2 = currentIncrementalData->begin();
#endif
        deletedWin += lastDeleted;
        if(queueWin->size()==0){
            deletedWin = 0;
        }

        winDesType::iterator previousEnd = queueWin->end();
        --previousEnd; //points to the predecessor of the last elem to FIX the problem of the
        winDesType::iterator it;
        for (it = queueWin->begin(); previousEnd!=it; ++it) {
#if INCREMENTAL_QUERY == 1
            ++it2;
#endif
/*
            if((*it).startDesc <= ts && (*it).used == true && (*it).sent == true ){
                queueWin->erase(it);
            }else {
 */
                if ( (*it).startPos <= 0 && (*it).endPos <= 0 && (*it).startDesc <= ts && (*it).used == true && (*it).sent == true ) {
                        queueWin->erase(it);

#if INCREMENTAL_QUERY == 1
                    currentIncrementalData->erase(it2);
#endif
                } else {
                    (*it).startPos -= lastDeleted;
                    (*it).endPos -= lastDeleted;

                    (*it).startPos = ((*it).startPos < 0) ? 0 : (*it).startPos;
                    (*it).endPos = ((*it).endPos < 0) ? 0 : (*it).endPos;

                    if ((*it).startPos > 0 && ((*it).startPos > (*it).endPos)) {
                        (*it).endPos = (*it).startPos;
                    }
                }
  //          }
            i++;
        }
        // queueWin->shrink_to_fit();
        //printf("delete queueWin size: %lu \n",queueWin->size());
        return lastDeleted;
    }


    /**************************** END EVICTION ****************************/

    /**
     * Return the NUMBER of the element in the deque in the status
     * @return [description]
     */
    long int getElementInTheQueue() {
        if (queue->size() <= 0) { return 0; }
        else { return queue->size(); }
    }

    /**
     * Given a ts value return the position in the deque
     * @param  ts [description]
     * @return    [description]

    long int getValuePosition(double ts){
      bool trovato = false;
      long int ret= 0;
      for (queueType::iterator it = queue->begin() ; trovato == false && it!=queue->end() ; ++it){
        if((*it).getTupleTimeStamp() >= ts){
          trovato = true;
        }else{
          ret++;
        }
      }
      #if _DEBUG == 1
      cout << "TS: " << ts << "; RET: " << ret << endl;
      #endif
      return ret;
    }*/
};
