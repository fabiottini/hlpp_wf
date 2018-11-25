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

#ifndef FQUEUE
#define FQUEUE 1

#define DIFF_UNIT 1
#define DIFF_MARGIN_ERROR 0.99999

/************************************[AUX FUNCTION]************************/
void FQUEUE_outputError(string s) {
  cerr << s << endl;
  throw runtime_error(s);
}
/************************************[AUX FUNCTION]************************/

using RANGE_TYPE = long long int;
#define MAX_SCROLL_CHUNK_SEARCH 1000 //no more X scroll Forward

/****************************************************************************
* fqueue is an list<chunk<T1,queueElem,queueTN>> where:
*    - T1: represent the type of the elements that delimit the minimal
*      group of elements (ex. the length is equal to the slide)
*    - RANGE: represent the type of the elements that delimit the range from
*      the init to the length of the queue (if the queue is an array
*      the length is fixed)
*    - queueTN: the data type that is containded in the queue
*
* T1:    0..........1000   1001........2000    2001.......3000
* RANGE: 0............99   100..........199    200.........299
*        +------------+     +------------+     +------------+
*        |  chunk 1   | ->  |  chunk 2   | ->  |  chunk 3   | ->
*        |            | <-  |            | <-  |            | <-
*        +------------+     +------------+     +------------+
*
*
***************************************************************************/

/**************************************************************************
*  CHUNK
**************************************************************************/

/**
* the ABSTRACTION class of the chunk that manage the fChunkQueue
*/
template<class queueElem, class queueTN>
class fChunkQueue {
public:
  // fChunkQueue(){}

  virtual queueElem
  get(RANGE_TYPE i)=0;   //if enter here TERRIBLE ERROR check in the caller not return NULL queueElem element
  virtual RANGE_TYPE add(queueElem value)=0;        //if enter here TERRIBLE ERROR check in the caller not return -1
  virtual bool remove(RANGE_TYPE pos)=0;
};


/**
* VECTOR SPECIALIZATION for the fChunkQueue
*/
template<class queueElem>
class fChunkQueue<queueElem, std::vector<queueElem>>{
  using qIterator = typename std::vector<queueElem>::iterator;
private:
  std::vector<queueElem> queue;
  RANGE_TYPE CIP = 0;
public:
  fChunkQueue() {
    CIP = 0;
    #if _DEBUG >= 1
    cout << "vector" << endl;
    #endif
  };

  RANGE_TYPE getLimitRange() { return -1; }

  RANGE_TYPE getCIP() { return CIP; }

  /**
  * add a new tuple to the vector queue of the chunk
  */
  RANGE_TYPE add(queueElem *value) {
    queue.push_back(*value);
    return ++CIP;
  }

  /**
  * retrieve and return the last element inserted
  */
  queueElem *getLastElement() {
    //printf("CIP: %lld | queue size: %ld\n",CIP,queue.size());
    if (CIP > 0) return &(queue[CIP - 1]);
    else return NULL;
  }

  queueElem *getFirstElement() {
    if (CIP > 0) return &(queue[0]);
    else return NULL;
  }


  /**
  * remove the element in the chunk
  */
  bool remove(RANGE_TYPE pos) {
    //cout << "entro remove CIP:" << CIP << "   pos:" << pos << endl;
    if(CIP==0) return false;
    if (pos < CIP) {
      qIterator it = queue.begin()+pos;
      //printf("elimino pos: %.2f | pos:  %d |  CIP: %d  | size: %lu \n",(*it).getTupleTimeStamp(),pos,CIP,queue.size());
      (*it).freeTuple();
      queue.erase(it);
      --CIP;
      return true;
    }
    return false;
  }

  bool deleteChunk(){
    //delete queue;
    //vector<queueElem>().swap(queue);
  }

  /**
  * get the i-th element from the chunk if in the range
  */
  queueElem *get(RANGE_TYPE i) {
    if(CIP==0) return NULL;
    if (i < CIP) {
      return &queue[i];
    }
    return NULL;
  }
};

/**
* ARRAY SPECIALIZATION for the fChunkQueue
*/
template<class queueElem, size_t N>
class fChunkQueue<queueElem, queueElem[N]> {
private:
  RANGE_TYPE MAX_ARR_SIZE = N;
  queueElem *queue;
  RANGE_TYPE CIP = 0;
  bool initDone = false;
public:
  fChunkQueue() {
    #if _DEBUG >= 1
    cout << "array" << endl;
    #endif
  }

  RANGE_TYPE getLimitRange() { return MAX_ARR_SIZE; }

  RANGE_TYPE getCIP() { return CIP; }

  /**
  * add a new tuple to the array queue of the chunk
  */
  RANGE_TYPE add(queueElem *value) {
    if (!initDone) {
      queue = new queueElem[MAX_ARR_SIZE];
      initDone = true;
    }
    queue[CIP] = *value;
    return ++CIP;
  }

  /**
  * retrieve and return the last element inserted
  */
  queueElem *getLastElement() {
    if (CIP > 0) return &(queue[CIP - 1]);
    else return NULL;
  }

  queueElem *getFirstElement() {
    if (CIP > 0) return &(queue[0]);
    else return NULL;
  }


  /**
  * remove the element in the chunk
  */
  bool remove(RANGE_TYPE pos) {
    if (pos < CIP) {
      delete queue[pos];
      for (int i = pos; i < CIP - 1; i++)
      queue[i] = queue[i + 1];
      --CIP;
      return true;
    }
    return false;
  }

  bool deleteChunk(){
    delete [] queue;
    //vector<queueElem>().swap(queue);
  }

  /**
  * get the i-th element from the chunk if in the range
  */
  queueElem *get(RANGE_TYPE i) {
    if (i < CIP) {
      return &queue[i];
    }
    return NULL;
  }
};


template<class T1, class queueElem, class queueTN>
struct chunk {
private:
  short int QUEUE_TYPE;
  using fChunkQueueT = fChunkQueue<queueElem, queueTN>;
  T1 startTs,             //the starting point of the managed timestamps
  endTs;               //the end point of the managed timestamps
  RANGE_TYPE startRange,  //the starting index of the managed elements (is possible to update it if delete prev. elements)
  endRange,       //the end index of the managed elements (is possible to update it if delete prev. elements)
  limitRange,     //-1 for the unlimited dataStructure ( es. vector ) N in case of limited structure
  elementsIn;     //the total elements in the queue (is a support var that let to retrieve immediatly the value)
  //if the chunk is not used animore for append elements
  fChunkQueueT *queue;// = (fChunkQueueT*)calloc(1, sizeof(fChunkQueueT)); //TODO: CLEAN

  /**
  * CHECK if queue type is correct
  * if it's wrong type throw and invalid_argument exception
  * otherwise set the QUEUE_TYPE var
  */
  void checkQueueTNType() {
    if (is_array<queueTN>::value == false && is_same<queueTN, vector<queueElem>>::value == false)
    throw invalid_argument(
      "The queueTN argument that define the queue type is invalid only vector<queueElem> or queueElem[] are allowed");
    }


  public:
    bool closeWin;

    ~chunk(){

      if(elementsIn>0){
        int i=0;
        //queue->deleteChunk();
        /*
        for(;i<=elementsIn;i++){
        removeTuple(i);
      }
      */
      delete queue;
    }

    //printf("chiamata distruzione chunk RIMOSSI: %d\n",i-startRange);
  }

  chunk() {
    startTs = 0, endTs = 0;
    startRange = 0, endRange = 0;
    elementsIn = 0;
    closeWin = false;
    queue = new fChunkQueueT();
    //checkQueueTNType(); //if it's wrong type throw and invalid_argument exception
  }

  chunk(T1 startTsVal, T1 endTsVal, RANGE_TYPE startRangeVal) :
  startTs(startTsVal), endTs(endTsVal), startRange(startRangeVal), endRange(startRangeVal) {
    //printf("create new chunk [%.2f; %.2f]\n",startTsVal,endTsVal);
    elementsIn = 0;
    closeWin = false;
    queue = new fChunkQueueT();
    //checkQueueTNType(); //if it's wrong type throw and invalid_argument exception
  }

  //get function
  T1 getChunkStartTs() { return startTs; }

  T1 getChunkEndTs() { return endTs; }

  RANGE_TYPE getChunkStartRange() { return startRange; }

  RANGE_TYPE getChunkEndRange() { return endRange; }

  RANGE_TYPE getNumElem() { return elementsIn; }

  bool getCloseWin() { return closeWin; }

  RANGE_TYPE getLimitRange() { if(queue!=NULL) return queue->getLimitRange(); else return 0;}

  RANGE_TYPE getCurrIndex() { if(queue!=NULL) return queue->getCIP(); else return 0; }

  queueElem *getFirstElement() { if(queue!=NULL) return queue->getFirstElement(); else return NULL; }
  queueElem *getLastElement() { if(queue!=NULL)  return queue->getLastElement(); else return NULL; }

  //set function
  void setStartTs(T1 val) { startTs = val; }

  void setEndTs(T1 val) { endTs = val; }

  void setStartRange(RANGE_TYPE val) { startRange = val; }

  void setEndRange(RANGE_TYPE val) { endRange = val; }

  void setCloseChunk(bool val) { closeWin = val; }

  /**
  * the abstraction of add tuple elements it use the underload method for
  * the specific type of queue (array/vector)
  * @param elem queueElem type element
  */
  void add(queueElem *elem) {
    elementsIn++;//= endRange - startRange ;
    endRange = startRange + queue->add(elem) - 1;
  }

  queueElem *getElem(RANGE_TYPE i) {
    if (endRange == startRange && getNumElem() == 0) return NULL; //nothing inside
    if (i >= (endRange - startRange)+1) return NULL;
    return queue->get(i);
  }

  queueElem *getRealPosElem(RANGE_TYPE i) {
    if (i >= (endRange)) return NULL;
    return queue->get(i);
  }

  /**
  * remove the tuple from the chunk
  */
  bool removeTuple(RANGE_TYPE pos) {
    //cout << "entro removeTuple" << endl;
    if (queue->remove(pos)) {
      endRange--;
      elementsIn--;
      return true;
    }
    return false;
  }

  /**
  * print the detail of the current chunk (DEBUG)
  */
  void print() {
    printf("startTs: %.2f | endTs: %.2f | startRange: %lld | endRange: %lld | isClose: %s | size: %lld | realSize: %lld\n",
    startTs, endTs, startRange, endRange, (closeWin == true) ? "True" : "False", getNumElem(),
    (endRange - startRange));
    //printQueue();
  }

  string toString() {
    char buffer [500];
    sprintf (buffer, "startTs: %.2f | endTs: %.2f | startRange: %lld | endRange: %lld | isClose: %s | size: %lld | realSize: %lld",
    startTs, endTs, startRange, endRange, (closeWin == true) ? "True" : "False", getNumElem(),
    (endRange - startRange));
    return buffer;
  }

  /**
  * Print ALL elements in the current chunk queue (DEBUG)
  */
  void printQueue() {
    cout << "\t-----------" << endl;
    if (startRange != endRange) {
      for (RANGE_TYPE i = 0; i <= getNumElem(); i++) {
        queueElem elem = (*queue->get(i));
        elem.print();
      }
    }
    cout << "\t-----------" << endl;
  }

};



/**************************************************************************
*  List<chunk<T1,queueElem,queueTN>>
**************************************************************************/

/**
* create the SPECIFICATION for SOME of the fList function
*/
template<class T1, class queueElem, class queueTN>
class fListSpecFunction {
};

RANGE_TYPE chunkInsertionPoint = 0;
static bool isArray = false;


/**
* ABSTRACTION OF LIST CHUNK
* This data type represent the ABSTRACTION of the LIST of CHUNK
* and implement all the correlated function to abstract to the user
* the add remove ecc...
*/
template<typename T1, typename queueElem, typename queueTN>
class fList {

  using chunkT       = chunk<T1, queueElem, queueTN>; //just to be dynamic
  using fListT       = fList<T1, queueElem, queueTN>; //just to be dynamic
  using specFunction = fListSpecFunction<T1, queueElem, queueTN>;
  using listIT       = typename std::list<chunkT>::iterator;

  typename std::list<chunkT>::iterator nextStartClosedWin;
  bool hasNext = true;
  std::atomic<RANGE_TYPE> isWindowClose;
  T1 lastDelete = 0;
  bool itsFirstClosedWin = true;
public:
  specFunction sp = specFunction(this);
  list<chunkT> internalList;

  list<chunkT> getInternalList() { return internalList; }
  T1 getLastDelete(){ return lastDelete; }

  T1 lastStartTs,lastEndTs, slideVal, winSize, lastWinTs = 0; //maintain the characheristic of the chunk
  RANGE_TYPE countPos = 0,               //maintain the last inserted element
  currentInsertChunk = 0,     //the chunk currently in insert mode
  nextAvailableIter = 0 ,      //cointain the position of the next avilable iterator chunk
  nextWinEmit = 0,        //is the NEXT chunk number from witch the window start
  countRealPositionChunk = 0;

  fList(T1 startTs, T1 slideSize, T1 winSizeVal, RANGE_TYPE rangeStart) {
    isArray = sp.isArray();
    lastWinTs = 0;
    countRealPositionChunk = 0;
    lastEndTs = lastStartTs = startTs;
    slideVal = slideSize;
    winSize = winSizeVal;
    countPos = rangeStart;
    currentInsertChunk = -1;
    nextAvailableIter = -1;
    nextWinEmit = 0;
    isWindowClose = 0;
    
    if(isArray){
      addSpecialChunkARRAY(0);
    }else{
      addChunk();  //create the first chunk
    }
  }

  /*********************************************************************
  * CHUNK ITERATOR
  *********************************************************************/
  class iterator : public std::iterator<std::input_iterator_tag,      // iterator_category
                                        tuple_t,                      // value_type
                                        RANGE_TYPE,                   // difference_type
                                        tuple_t *,                     // pointer
                                        tuple_t                       // reference
                                        >
  {
  private:
    fList<T1, queueElem, queueTN> *data;

    //the list<chunk>::iterator
    listIT  bIT,           //point the FIRST element in the chunk (if exists)
            eIT,             //point the LAST element in the chunk (if exists)
            originalBeginIT,   //the original begin (copied in the constructor) interator THEY DON'T MOVE
            originalEndIT,     //the original end (copied in the constructor) interator THEY DON'T MOVE
            tmpPointer;        //aux pointer

    T1 startTS = 0, endTS = 0;
    bool isEqual = true;
    RANGE_TYPE startPOS = 0, endPOS = 0, currentPOS = 0, elementNumber = 0;

    RANGE_TYPE p = 0,       //the NEXT valid element in the current chunk (if 0 no element inside)
              winId = 0;   //the id of the current win
    queueElem *currElem;// = (queueElem*)calloc(1,sizeof(queueElem)); //the pointer to the current element (tuple)
    queueElem *tmpElem ;// = (queueElem*)calloc(1,sizeof(queueElem));
  public:
    RANGE_TYPE pEnd = 0;
    RANGE_TYPE totElemSBT = 0;

    explicit iterator() { }

    explicit iterator(listIT *_begin,
      listIT *_end,
      T1 startTs , T1 endTs,
      RANGE_TYPE startRange, RANGE_TYPE endRange
    ) {
      bIT = originalBeginIT = *_begin;
      eIT   = originalEndIT   = *_end;
      startTS     = startTs;
      endTS       = endTs;
      startPOS    = startRange;
      endPOS      = endRange;
      currentPOS  = startPOS;


      while ((currElem = (*bIT).getElem(p)) == NULL
      && bIT != eIT) { //if the element is null
        ++bIT;
      }

      pEnd = 0;
      RANGE_TYPE p2MAX =  (*eIT).getNumElem();
      while ((tmpElem = (*eIT).getElem(pEnd)) == NULL || ( (*eIT).getElem(pEnd)->getTupleTimeStamp() < endTs  && pEnd<=p2MAX) ) { //if the element is null
        pEnd++;
      }

      elementNumber = size();
      p++; //for the ++x case that goes to the next element

      //printf("p: %lld pEnd: %lld\n",p,pEnd);
    }

    explicit iterator(listIT *_begin,
      listIT *_end){
        bIT = originalBeginIT = *_begin;
        eIT   = originalEndIT   = *_end;

        startTS     = bIT->getChunkStartTs();
        endTS       = eIT->getChunkEndTs();
        startPOS    = bIT->getChunkStartRange();
        endPOS      = eIT->getChunkEndRange();
        currentPOS  = startPOS;
        elementNumber = size();

        while ((currElem = (*bIT).getElem(p)) == NULL
        && bIT != eIT) { //if the element is null
          ++bIT;
        }
        p++; //for the ++x case that goes to the next element

      }

      explicit iterator(listIT _begin,
        listIT _end) {
          bIT = originalBeginIT = _begin;
          eIT   = originalEndIT   = _end;
          startTS     = bIT->getChunkStartTs();
          endTS       = eIT->getChunkEndTs();
          startPOS    = bIT->getChunkStartRange();
          endPOS      = eIT->getChunkEndRange();
          elementNumber = size();

          while ((currElem = (*bIT).getElem(p)) == NULL
          && bIT != eIT) { //if the element is null
            ++bIT;
          }
          p++; //for the ++x case that goes to the next element

          currentPOS  = startPOS;
        }

        /**
        * Generate the begin and the end of an interval defined by an iterator
        * @param _begin    the starting pointer
        * @param _end      the ending pointer
        * @param val       0 if the returned iterator is the begin; otherwise for the end
        */
        explicit iterator(listIT _begin,
          listIT _end,
          int val) {
            bIT = originalBeginIT = _begin;
            eIT   = originalEndIT   = _end;

            startTS     = bIT->getChunkStartTs();
            endTS       = eIT->getChunkEndTs();
            startPOS    = bIT->getChunkStartRange();
            endPOS      = eIT->getChunkEndRange();
            elementNumber = size();

            //printf("elementNumber: %ld\n",elementNumber);

            int shiftBackEnd = 0;

            //adjust the starting point to the chunk with some tuple inside or to the end
            while ((currElem = (*bIT).getElem(p)) == NULL && bIT != eIT) { //if the element is null
              ++bIT;
            }
            //adjust the ending point to the chunk with some tuple inside or to the end
            while ((tmpElem = (*eIT).getLastElement()) == NULL && bIT != eIT) { //if the element is null
              --eIT;
              ++shiftBackEnd;
            }

            //adjust the auxiliary value
            if(val == 0){ //begin
              if(currElem!=NULL) p++; //for the ++x case that goes to the next element
              currentPOS  = 0;
              if(elementNumber==0){
                isEqual = false;
              }else{
                isEqual = true;
              }
            }else{ //end
              isEqual = false;
              currElem=tmpElem;
              p=(*eIT).getNumElem();
              currentPOS  = (elementNumber>0)?elementNumber-1:0; //0 if no element inside otherwise "point" to the last one
            }
          }

          explicit iterator(T1 start, T1 end, T1 sliding, listIT _begin, listIT _end, int val) {
              bIT = originalBeginIT = _begin;
              eIT   = originalEndIT   = _end;

              startTS     = bIT->getChunkStartTs();
              endTS       = eIT->getChunkEndTs();
              startPOS    = bIT->getChunkStartRange();
              endPOS      = eIT->getChunkEndRange();
             // elementNumber = size();

              //printf("elementNumber: %ld\n",elementNumber);

              //printf("BEFORE: [%f; %f] => [%f; %f] | element#: %f \n",start,end, (*bIT).getElem(0)->getTupleTimeStamp(), (*eIT).getLastElement()->getTupleTimeStamp(),sliding);


              int shiftBackEnd = 0;

              pEnd = ((*eIT).getNumElem()>1)? (*eIT).getNumElem()-2 : 0;

              while((*bIT).getElem(p)==NULL && p<(*bIT).getNumElem()){
                p++;
              }
              currElem = (*bIT).getElem(p);
              while((*eIT).getElem(pEnd)!=NULL && pEnd>0){
                pEnd--;
              }
              tmpElem = (*eIT).getElem(pEnd);

              while((*currElem).getTupleTimeStamp()<start && p< (*bIT).getNumElem()){
                 if((*currElem).getTupleTimeStamp() < start){ 
                   p++;
                 }
                 if( (*bIT).getNumElem() >= p ){ 
                   ++bIT;
                   p=0;
                 }
                 currElem = (*bIT).getElem(p);
              }
/*
              while( (*tmpElem).getTupleTimeStamp() > end && pEnd > 0){
                if((*tmpElem).getTupleTimeStamp() > end){ 
                  pEnd--;
                }else{
                  --eIT;
                }
                tmpElem = (*eIT).getElem(pEnd);
              }
 */             
              queueElem *tmpNext,*tmpPrev;
              while( (*tmpElem).getTupleTimeStamp() != end && pEnd > 0){
                tmpPrev = (*eIT).getElem(pEnd-1);
                printf("pEnd: %lld | #ele: %lld | [%f; %f] \n",pEnd,(*eIT).getNumElem(),start,end);
                if(pEnd<(*eIT).getNumElem()-1){
                  tmpNext = (*eIT).getElem(pEnd+1);
                  if((*tmpNext).getTupleTimeStamp() > end){ 
                    pEnd--;
                  }else if((*tmpPrev).getTupleTimeStamp() < end){ 
                    pEnd++;
                  }
                }else{
                  pEnd--;
                }
                tmpElem = (*eIT).getElem(pEnd);
              }

              //cout << (*eIT).getElem(pEnd)->getTupleId() << endl;
              elementNumber = (*eIT).getElem(pEnd)->getTupleId()-(*bIT).getElem(pEnd)->getTupleId()+2;
              totElemSBT =(elementNumber>0)?elementNumber:0;


              //adjust the auxiliary value
              if(val == 0){ //begin
                if(currElem!=NULL) p++; //for the ++x case that goes to the next element
                currentPOS  = 0;
                if(elementNumber==0){
                  isEqual = false;
                }else{
                  isEqual = true;
                }
              }else{ //end
                isEqual = false;
                currElem=tmpElem;
                
                  p=pEnd+1;
                  currentPOS  = (elementNumber>0)?elementNumber-1:0; //0 if no element inside otherwise "point" to the last one
                
              }

              if(val != 0){
                //printf("AFTER: [%f; %f] => [%f; %f] | element#: %f | p: %lld | pEnd: %lld \n",start,end, (*bIT).getElem(0)->getTupleTimeStamp(), (*eIT).getElem(pEnd)->getTupleTimeStamp(),sliding,p,pEnd);
              }
            }

          /**
          * Redefine the ++ operator for the current data structure
          * @return the pointer to this with the currentElement that point to the
          *          NEXT element (tuple) or object this with currentElement = null if some problem
          */
          iterator &operator++() {
            if(elementNumber > 0 && currentPOS==(elementNumber-1) && isEqual == true){
              isEqual = false;
            }
            while (elementNumber > 0 && currentPOS < elementNumber &&  isEqual == true) {
              if (currElem != NULL && p < (*bIT).getNumElem()) {
                currElem = (*bIT).getElem(p);
                p++;
                currentPOS++;
                if (currElem != NULL) {
                  return *this;
                }
              } else {
                if (currentPOS < elementNumber - 1) {
                  ++bIT;
                  p = 0;
                  currElem = (*bIT).getElem(p);
                } else {
                  p = 0;
                  break;
                }
              }
            }
            return *this;
          }

          /**
          * Redefine the ++ operator for the current data structure
          * @return the pointer to this with the currentElement that point to the
          *          NEXT element (tuple) or object this with currentElement = null if some problem
          */
          iterator &operator--() {
            if(elementNumber > 0 && currentPOS==0 && isEqual == true){ isEqual = false; }
            if(currentPOS==0 && elementNumber==0){ isEqual = false; }

            while (currentPOS>=0  && isEqual==true)  {
              if (currElem != NULL && p > 0) {
                if( p == (*eIT).getNumElem() && p>1 ){ p=p-2; }
                else{ p--; }
                currElem = (*eIT).getElem(p);
                currentPOS --;
                if(currElem != NULL){
                  return *this;
                }
              }else{
                if(currentPOS>0){
                  --eIT;
                  p = (*eIT).getNumElem();
                  if(p>0){
                    p--;
                    currentPOS --;
                    currElem = (*eIT).getElem(p);
                    return *this;
                  }else{
                    currElem = NULL;
                  }
                }else{
                  p = ((*eIT).getNumElem()>=1)?1:0;
                  break;
                }
              }
            }
            return *this;
          }

          /**
          * return the size (the number of the tuple) in the interval begin end
          * @return
          */
          RANGE_TYPE size() {
            tmpPointer = originalBeginIT;
            RANGE_TYPE numElem = 0, tmpPos = startPOS;
            for(;tmpPointer!=eIT;++tmpPointer){
              if(tmpPointer!=eIT){
                numElem += (*tmpPointer).getNumElem();
              }
            }
            numElem += (*tmpPointer).getNumElem();
            return numElem;

          }

          RANGE_TYPE sizeSBT() {
            RANGE_TYPE tmp_p=p,tmp_pEnd=pEnd;
            tmpPointer = bIT;
            RANGE_TYPE numElem = 0, tmpPos = startPOS;
            for(;tmpPointer!=eIT;++tmpPointer){
              numElem += (*tmpPointer).getNumElem();
            }
            numElem+=pEnd-p;

            return numElem;

          }


          friend long distance(const iterator& begin,
                         const iterator& end)
          { 
            if(isArray){
              return (end).totElemSBT;
            }else{
              return std::distance(begin,end);
            }
          }

          /**
          * Redefine the == operator to compare ONLY the relevant parameter
          * @param other the other objet of the same type
          * @return true if the other object and this have the same values;  false otherwise
          */
          bool operator==(iterator other) const {
            if(
              this->currentPOS == other.currentPOS &&
              this->isEqual    == other.isEqual){
                return true;
              }
              return false;
            }

            bool operator!=(iterator other) const {
              return !(*this == other);
            }

            /**
            * Deferentiate return the currentElement the tuple
            * @return
            */
            queueElem operator*() { return *currElem; }

            T1 getTsStart() { return startTS; }
            T1 getTsEnd() { return endTS; }
            RANGE_TYPE getRangeStart()  { return startPOS; }
            RANGE_TYPE getRangeEnd()    { return endPOS; }
            RANGE_TYPE getCurrentPos()  { return currentPOS; }





            queueElem *begin() { return (*originalBeginIT).getFirstElement(); }
            queueElem *end() {
              while((*eIT).getLastElement() == NULL && eIT!=bIT){ --eIT; }
              return (*eIT).getLastElement();
            }


             //Return the starting pointer of the current iterator
            iterator *beginITSBT(T1 start,T1 end,T1 sliding){ return new iterator(start,end,sliding,bIT,eIT,0); }
            //Return the ending pointer of the current iterator
            iterator *endITSBT(T1 start,T1 end,T1 sliding){
              return new iterator(start,end,sliding,bIT,eIT,-1);
            }


            //Return the starting pointer of the current iterator
            iterator *beginIT(){ return new iterator(originalBeginIT,originalEndIT,0); }
            //Return the ending pointer of the current iterator
            iterator *endIT(){ return new iterator(originalBeginIT,originalEndIT,-1); }

            void setWinId(RANGE_TYPE _winId) { winId = _winId; }
            void setStartTS(T1 _val){ startTS = _val; }
            void setEndTS(T1 _val){ endTS = _val; }
            void setStartPOS(RANGE_TYPE _val){ startPOS = _val; }
            void setEndPOS(RANGE_TYPE _val){ endPOS = _val; }
            RANGE_TYPE getWinId() { return winId; }

            bool isCurrentElementNULL(){ if(currElem==NULL) return true; else return false; }

            void print() { this->print(); }
          };


          T1 getSlideVal() { return slideVal; }
          T1 getLastStartTs() { return lastStartTs; }
          RANGE_TYPE getCurrentInsertChunk() { return currentInsertChunk; }
          RANGE_TYPE getCountPos() { return countPos; }
          RANGE_TYPE size() { return countPos; }
          void incCountPos() { countPos++; }
          void incIsWinClose (){ isWindowClose++; }
          void decIsWinClose (){ isWindowClose--; }
          bool getIsWinClose (){ if(isWindowClose>0){ return true; }else{return false;} }
          typename std::list<chunkT>::iterator getLastChunk (){ return internalList.end(); }

          //iterator *getFullIterator(){ return new iterator(internalList.begin(),internalList.end());  }

          /**
          * return the next available window if it's possible
          */
          iterator *getWinFromTsInterval(T1 startTs, T1 endTs) {
            listIT *itB, *itE;
            bool setITE = false;
            RANGE_TYPE NUM_CHUNK_IN_WIN = 0,INDIETRO = 0;
            T1 currentStartITBTs = 0, currentEndITBTs = 0;

            itB = (listIT*) calloc(1,sizeof(listIT));
            itE = (listIT*) calloc(1,sizeof(listIT));

            *itE = internalList.end();
            *itE = prev(*itE);

            INDIETRO         = ((**itE).getChunkEndTs()/slideVal)-(endTs/slideVal);
            NUM_CHUNK_IN_WIN = (winSize/slideVal)-1;

            *itE = prev(*itE,INDIETRO);
            if((**itE).getChunkEndTs() != endTs) {
              while ((**itE).getChunkEndTs() > endTs && internalList.begin() != *itE) {
                *itE = prev(*itE);
              }
            }

            *itB = *itE;

            *itB = prev(*itB,NUM_CHUNK_IN_WIN);
            if((**itB).getChunkEndTs() != startTs) {
              while ((**itB).getChunkStartTs() > startTs && internalList.begin() != *itB) {
                *itB = prev(*itB);
              }
            }

            currentStartITBTs = (**itB).getChunkStartTs();
            currentEndITBTs   = (**itE).getChunkEndTs();

            return new iterator(itB, itE);
          }


          iterator *getWinFromTsInterval_sbt(T1 startTs, T1 endTs) {
            listIT *itB, *itE;
            bool setITE = false;
            RANGE_TYPE NUM_CHUNK_IN_WIN = 0,INDIETRO = 0;
            T1 currentStartITBTs = 0, currentEndITBTs = 0;

            itB = (listIT*) calloc(1,sizeof(listIT));
            itE = (listIT*) calloc(1,sizeof(listIT));

            *itE = internalList.end();
            *itE = prev(*itE);

            if((**itE).getChunkEndTs() != endTs) {
              while ((**itE).getChunkEndTs() > endTs && internalList.begin() != *itE) {
                *itE = prev(*itE);
              }
            }

            *itB = *itE;
            if((**itB).getChunkStartTs() != startTs) {
              while ((**itB).getChunkStartTs() > startTs && internalList.begin() != *itB) {
                *itB = prev(*itB);
              }
            }

            currentStartITBTs = (**itB).getChunkStartTs();
            currentEndITBTs   = (**itE).getChunkEndTs();

            return new iterator(itB, itE);
          }

          /**
          * retrieve the last window in the queue
          * @param limitUP the TS value of the last tuple
          * @return  the iterator of the window interval
          */
          iterator *getLastClosedWin(T1 startWin,T1 limitUP) {
            listIT *itB, *itE, *itTMP;
            iterator *ret;

            if(hasNext==false) return NULL; //the previous check is the last

            itB = (listIT*) calloc(1,sizeof(listIT));
            itE = (listIT*) calloc(1,sizeof(listIT));
            itTMP = (listIT*) calloc(1,sizeof(listIT));

            RANGE_TYPE NUM_CHUNK_IN_WIN = (winSize / slideVal) - 1;
            T1 endTs = 0;
            if (itsFirstClosedWin) { //only first time
              nextStartClosedWin = begin();
              itsFirstClosedWin  = false;
            }else if(lastStartTs > startWin){
              while((*nextStartClosedWin).getChunkStartTs() < startWin){
                ++nextStartClosedWin;
              }
            }else{
              //add chunk if missing to reach the limitUP
              while(lastEndTs < limitUP){
                addSpecialChunkComplete(lastStartTs);
                (*internalList.end()).setCloseChunk(true);
              }
            }

            *itE = *itB = nextStartClosedWin;
            std::advance(*itE,NUM_CHUNK_IN_WIN);

            endTs = (*itB)->getChunkStartTs() + winSize;
            *itTMP = *itE;

            if((*itE)->getChunkEndTs()+DIFF_UNIT > endTs) {
              *itE = *itB;
              std::advance(*itE,NUM_CHUNK_IN_WIN-1);
            }else if((*itE)->getChunkEndTs()+DIFF_UNIT < endTs) {
              T1 tmpValMemory = (*itE)->getChunkEndTs();

              typename std::list<chunkT>::iterator itEndPointer = end();
              bool stopCondition = false;
              ++*itTMP;
              while (*itTMP != itEndPointer &&
                tmpValMemory <= (*itTMP)->getChunkEndTs() &&
                (*itTMP)->getChunkEndTs() + DIFF_UNIT <= endTs) {

                ++*itE;
                ++*itTMP;
                if(tmpValMemory <= (*itTMP)->getChunkEndTs()){
                  hasNext = false;
                  free(itB);
                  free(itE);
                  free(itTMP);
                  return NULL;
                }
              }

              if ((*itE)->getChunkEndTs() + DIFF_UNIT < endTs) {
                free(itB);
                free(itE);
                free(itTMP);
                return NULL;
              }
            }

            if (itB != itE) {
              if( (*nextStartClosedWin).getChunkStartTs()+winSize  > limitUP  ){ hasNext = false; }
              ++nextStartClosedWin; //point to the next
              ret = new iterator(itB, itE, (*itB)->getChunkStartTs(), (*itE)->getChunkEndTs(),(*itB)->getChunkStartRange(), (*itE)->getChunkEndRange()  );
            } else {
              ret = NULL;
            }
            free(itB);
            free(itE);
            free(itTMP);
            return ret;
          }


          iterator *getNextClosedWinSBT(T1 startWin, T1 lastTuple) {
            iterator *ret;
            decIsWinClose();
            listIT *itB, *itE;

            itB = (listIT*) calloc(1,sizeof(listIT));
            itE = (listIT*) calloc(1,sizeof(listIT));

            *itB = *itE = internalList.end();
            long long int tmpCurrentChunk = internalList.size();
            listIT tmp=internalList.begin();
            //cout << "(*itE)->getChunkStartTs(): " << (*itE)->getChunkStartTs()  << endl;
            while((*itE)->getChunkStartTs() > lastTuple && tmpCurrentChunk>0){
              //printf("tmpCurrentChunk: %lld | #elem: %lld |  (*itE)->getChunkStartTs(): %f >= lastTuple: %f\n", tmpCurrentChunk,(*itB)->getNumElem(),(*itE)->getChunkStartTs(),lastTuple);

              tmpCurrentChunk--;
              --*itE;
            }
            //cout << "(*itE)->getChunkStartTs(): " << (*itE)->getChunkStartTs()  << endl;
            *itB = *itE;
            while((*itB)->getChunkStartTs() > startWin && tmpCurrentChunk>0){
              //printf("tmpCurrentChunk: %lld | #elem: %lld |  (*itB)->getChunkStartTs(): %f >= startWin: %f\n", tmpCurrentChunk,(*itB)->getNumElem() , (*itB)->getChunkStartTs(),startWin);
              tmpCurrentChunk--;
               --*itB;
            }

            ret = new iterator(itB, itE, startWin, lastTuple,(*itB)->getChunkStartRange(), (*itE)->getChunkEndRange()  );

            ////free(itB);
            ////free(itE);
            return ret;
          }



          /**
          * retrieve the next closed window
          * @return the iterator of the the window
          */
          iterator *getNextClosedWin(T1 startWin, T1 ratio) {
            if(!getIsWinClose()) return NULL;
            iterator *ret;
            decIsWinClose();
            listIT *itB, *itE, *itTMP;

            itB = (listIT*) calloc(1,sizeof(listIT));
            itE = (listIT*) calloc(1,sizeof(listIT));
            itTMP = (listIT*) calloc(1,sizeof(listIT));

            RANGE_TYPE NUM_CHUNK_IN_WIN = (winSize / slideVal) - 1;
            T1 endTs = 0;
            if (itsFirstClosedWin) { //only first time
              nextStartClosedWin = begin();
              itsFirstClosedWin  = false;
            }else{
              while((*nextStartClosedWin).getChunkStartTs() < startWin){
                ++nextStartClosedWin;
              }
            }

            *itE = *itB = nextStartClosedWin;
            std::advance(*itE,NUM_CHUNK_IN_WIN);

            //SPECIAL CASE ARRAY
            endTs = (*itB)->getChunkStartTs() + winSize;
            *itTMP = *itE;

            if((*itE)->getChunkEndTs() + DIFF_UNIT > endTs) {
              *itE = *itB;
              std::advance(*itE,NUM_CHUNK_IN_WIN-1);
            }else if((*itE)->getChunkEndTs() + DIFF_UNIT < endTs) {
              T1 tmpValMemory = (*itE)->getChunkEndTs();
              while (*itTMP != end() &&
              ((*itTMP)->getChunkEndTs() - endTs) < 0 &&
              (*itE)->getChunkEndTs() + DIFF_UNIT != endTs
            ) {
              ++*itE;
              *itTMP = next(*itE,1);
              if(tmpValMemory < (*itTMP)->getChunkEndTs()){
                return NULL;
              }
            }
            if ((*itE)->getChunkEndTs() + DIFF_UNIT < endTs) { return NULL; }
          }
          //END SPECIAL CASE ARRAY


          if (itB != itE &&
            (*itE)->getCloseWin()) {
              for(int i=0;i<ratio;i++){ ++nextStartClosedWin; } //point to the next
              ret = new iterator(itB, itE, (*itB)->getChunkStartTs(), (*itE)->getChunkEndTs(),(*itB)->getChunkStartRange(), (*itE)->getChunkEndRange()  );
            } else {
              ret = NULL;
            }
            free(itTMP);
            free(itB);
            free(itE);
            return ret;
          }

          /**
          * return the iterator of the next available window
          */
          iterator *getNextAvailableWin(T1 startWin) {
            iterator *ret = getNextClosedWin(startWin);
            if (ret != NULL) {
              nextWinEmit++;
            }
            return ret;
          }

          /**
          * Return the iterator of the lastest window that remain pendent
          * @param limitUp the TS limit i'm interested
          * @return the iterator of the window
          */
          iterator *getLastWin(T1 startWin, T1 limitUp) {
            iterator *ret = getLastClosedWin(startWin,limitUp);
            //iterator *ret = getNextClosedWin(startWin);
            if (ret != NULL) {
              nextWinEmit++;
            }
            return ret;
          }

          /**
          * se the current chunk (the chunk that is in insertion mode) to sign as close
          */
          void closeCurrentChunk() {
            listIT *currentChunk;
            currentChunk = (listIT*) calloc(1,sizeof(listIT)); //TODO: DA RIVEDERE!!?!?!?!?!
            *currentChunk = begin();
            std::advance(*currentChunk, currentInsertChunk);
            if (!(**currentChunk).getCloseWin()) {
              (**currentChunk).setCloseChunk(true);
              if ((**currentChunk).getChunkStartRange() == (**currentChunk).getChunkEndRange()) {
                //nothing inside the current chunk
              }
            }
            free(currentChunk);
          }

          void closeCurrentChunk(T1 endTsToClose) {
            listIT *currentChunk;
            //currentChunk = (listIT*) calloc(1,sizeof(listIT));
            *currentChunk = begin();
            std::advance(*currentChunk, currentInsertChunk);
            if (!(**currentChunk).getCloseWin()) {
              (**currentChunk).setEndTs(endTsToClose);
              closeCurrentChunk();
            }
            #if _DEBUG >= 2
            (**currentChunk).print();
            #endif
          }

          RANGE_TYPE cancella(T1 ts){
            RANGE_TYPE count = 0, deletedElem = 0;
            bool trovato = false;

            if (ts <= 0) return 0;
            typename std::list<chunkT>::iterator it = internalList.begin(),
                                              itEnd = internalList.end();
            --itEnd;
            for (; it != itEnd  && trovato == false; ++it) {
              if ((*it).getChunkEndTs() <= ts) {
                //printf("delete: [%f; %f]\n",(*it).getChunkStartTs(),(*it).getChunkEndTs());
                count++;
                deletedElem += (*it).getNumElem();
                it = internalList.erase(it);
                nextStartClosedWin = it;
              } else {
                trovato = true;
              }
            }
            it = internalList.begin();
            lastDelete = (*it).getChunkStartTs();
            //printf("#deleted elem: %lld\n",deletedElem);
            return count;
          }

          /**
          * add a new chunk element to the back of the list
          */
          void addChunk() {
            if (currentInsertChunk >= 0) closeCurrentChunk();
            //chunkT *elem = (chunkT*) calloc(1,sizeof(chunkT));
            chunkT *elem = new chunkT(lastStartTs, lastStartTs + slideVal - DIFF_UNIT, countPos);
            lastStartTs += slideVal;
            lastEndTs   = lastStartTs+winSize;
            internalList.push_back(*elem);
            currentInsertChunk++;
            nextAvailableIter = currentInsertChunk - 1; //set the last available chunk number
            //printf("addChunk - [ %.2f; %.2f ] - countpos: %lld - currentInsertChunk: %lld - nextAvailableIter: %lld\n",lastStartTs,lastStartTs + slideVal - DIFF_UNIT , countPos,currentInsertChunk,nextAvailableIter);
          }

          /**
          * add a new chunk element to the back of the list BUT starts from the specified value
          * @param startTs the value to start from the next chunk
          */
          void addSpecialChunk(T1 startTs) {
            if (currentInsertChunk >= 0) closeCurrentChunk(startTs - DIFF_UNIT);
            //chunkT *elem = (chunkT*) calloc(1,sizeof(chunkT));
            chunkT *elem = new chunkT(lastStartTs, lastStartTs + slideVal - DIFF_UNIT, countPos);
            internalList.push_back(*elem);
            currentInsertChunk++;
            lastStartTs = startTs+slideVal;
            lastEndTs   = lastStartTs+winSize;
            nextAvailableIter = currentInsertChunk - 1; //set the last available chunk number
            //printf("addSpecialChunk - [ %.2f; %.2f ] - countpos: %lld - currentInsertChunk: %lld - nextAvailableIter: %lld\n",lastStartTs,lastStartTs + slideVal - DIFF_UNIT , countPos,currentInsertChunk,nextAvailableIter);

          }

          void addSpecialChunkARRAY(T1 startTs) {
            //if (currentInsertChunk >= 0) closeCurrentChunk(startTs - DIFF_UNIT);
            //chunkT *elem = (chunkT*) calloc(1,sizeof(chunkT));
            chunkT *elem = new chunkT(startTs, startTs+winSize-DIFF_UNIT, 0);
            internalList.push_back(*elem);
            currentInsertChunk++;
            lastStartTs = startTs+slideVal;
            lastEndTs   = lastStartTs+winSize;
            nextAvailableIter = currentInsertChunk - 1; //set the last available chunk number
//            printf("addSpecialChunk - [ %.2f; %.2f ]\n",elem->getChunkStartTs(),elem->getChunkEndTs() );

          }

          void addSpecialChunkComplete(T1 startTs) {
            if (currentInsertChunk >= 0) closeCurrentChunk(startTs - DIFF_UNIT);
            //chunkT *elem = (chunkT*) calloc(1,sizeof(chunkT));
            chunkT *elem = new chunkT(lastStartTs, lastStartTs + winSize - DIFF_UNIT, countPos);
            internalList.push_back(*elem);
            currentInsertChunk++;
            lastStartTs = startTs+slideVal;
            lastEndTs   = lastStartTs+winSize;
            nextAvailableIter = currentInsertChunk - 1; //set the last available chunk number
            //printf("addSpecialChunk - [ %.2f; %.2f ] - countpos: %lld - currentInsertChunk: %lld - nextAvailableIter: %lld\n",lastStartTs,lastStartTs + slideVal - DIFF_UNIT , countPos,currentInsertChunk,nextAvailableIter);

          }

          /**
          * add a new chunk element to the back of the list BUT starts from the specified value and end to the specific values
          * @param startTs the value to start from the next chunk
          * @param endTs   the value that ends the chunk intervals
          */
          void addSpecialChunk(T1 startTs, T1 endTs) {
            if (currentInsertChunk >= 0) closeCurrentChunk(startTs - DIFF_UNIT);
            //chunkT *elem = (chunkT*) calloc(1,sizeof(chunkT));
            chunkT *elem =new chunkT(startTs, endTs, countPos);
            internalList.push_back(*elem);
            currentInsertChunk++;
            lastStartTs = startTs+slideVal;
            lastEndTs   = lastStartTs+winSize;
            nextAvailableIter = currentInsertChunk - 1; //set the last available chunk number
            //printf("addSpecialChunk2 - [ %.2f; %.2f ] - countpos: %lld - currentInsertChunk: %lld - nextAvailableIter: %lld\n",lastStartTs,lastStartTs + slideVal - DIFF_UNIT , countPos,currentInsertChunk,nextAvailableIter);

          }

          /**
          * return an iterator obj. point the top of the queue
          */
          typename std::list<chunkT>::iterator begin() {
            return internalList.begin();
          }

          /**
          * return an iterator obj. point the end of the queue
          */
          typename std::list<chunkT>::iterator end() {
            return internalList.end();
          }

          /**
          * remove the tuple from the chunk in position pos
          * @param pos the tuple position to remove
          */
          bool removeTuple(RANGE_TYPE pos) {
            bool found = false;
            typename std::list<chunkT>::iterator it = begin();
            for (; it != end() && !found; ++it) {
              if (pos >= (*it).startRange && pos <= (*it).endRange) {
                found = true;
                return (*it).removeTuple(pos);
              }
            }
            return false;
          }

          /**
          * remove the chunk in chunkPos
          * @param chunkPos the chunk position to remove
          */
          bool removeChunk(RANGE_TYPE chunkPos) {
            if (chunkPos <= nextAvailableIter || chunkPos == 0) { //i can remove only the available chunk
              typename std::list<chunkT>::iterator it = begin();
              //to jump directly to the current insertion pointer (chunk)
              if (chunkPos > 0) { std::advance(it, chunkPos); }
              internalList.erase(it);
              updateChunk();
              return true;
            }
            return false;
          }

          /**
          * remove all the chunk with T1 less than refer
          * @param tsRefer the T1 reference var
          */
          virtual bool removeChunkSmaller(T1 tsRefer) {
            bool found = false;
            typename std::list<chunkT>::iterator it = begin();
            RANGE_TYPE to = -1;
            T1 currentEndTs = -1;
            for (; it != end() && !found; ++it) {
              currentEndTs = (*it).getChunkEndTs();
              if (tsRefer >= currentEndTs) {
                ++to;
              } else {
                found = true;
              }
            }

            if (to >= 0) {
              typename std::list<chunkT>::iterator itFROM = begin();
              typename std::list<chunkT>::iterator itTO = begin();
              std::advance(itTO, to + 1);
              internalList.erase(itFROM, itTO);
              updateChunk();
              currentInsertChunk -= to;
            }
            return true;
          }

          /**
          * update all the chunk after a remove event
          * update all the start/end range only
          */
          void updateChunk() {
            typename std::list<chunkT>::iterator it = begin();
            RANGE_TYPE tmpCurrentVal = 0, oldStartRange, oldNumElem;
            for (; it != end(); ++it) {
              oldStartRange = (*it).getChunkStartRange(),
              oldNumElem = (*it).getNumElem();
              (*it).setStartRange(tmpCurrentVal);
              (*it).setEndRange(tmpCurrentVal + oldNumElem - 1);
              tmpCurrentVal += oldNumElem;
            }

            countPos = tmpCurrentVal + 1; //update the counter
          }

          /**
          * add a tuple element to the correct chunk
          * @param elem the tuple to add
          */
          void push_back(queueElem *elem) {
            sp.push_back(elem);
          }

          /**
          * request to the last chunk to close it because finish to add elements
          */
          void close() {
            closeCurrentChunk();
          }

          /**
          * Aux function to print all the chunk in the list (DEBUG)
          */
          void print() {
            print(-1);
          }

          void print(int limit) {
            typename std::list<chunkT>::iterator it = internalList.begin();
            int i = 0;
            for (; it != internalList.end(); ++it, i++) {
              cout << i << ") ";
              (*it).print();
              if(limit!=-1 && limit == i) return ;
            }
            if(i==0){
              cout << "NOTHING TO PRINT" << endl;
            }
          }
        };


        /**
        * create the ARRAY SPECIFICATION for SOME of the fList function
        */
        template<class T1, class queueElem, size_t N>
        class fListSpecFunction<T1, queueElem, queueElem[N]> {
        private:
          using chunkT = chunk<T1, queueElem, queueElem[N]>; //just to be dynamic
          using fListT = fList<T1, queueElem, queueElem[N]>; //just to be dynamic
          fListT *myList;// = (fListT*)calloc(1,sizeof(fListT));
        public:
          fListSpecFunction(fListT *myListVal) {
            myList = myListVal;
          }
          bool isArray(){ return true; }

          void push_back(queueElem *_elem) {
            //cout << "push_back: " << (_elem)->toString().c_str() << endl;
            bool found = false;
            //queueElem *elem = (queueElem *)calloc(1, sizeof(queueElem));
            //memcpy(elem,_elem,sizeof(queueElem));
            queueElem *elem = _elem;
            typename std::list<chunkT>::iterator it = myList->begin();

            T1 slideVal = myList->getSlideVal();
            //RANGE_TYPE currentInsertChunk = myList->getCurrentInsertChunk();

            //to jump directly to the current insertion pointer (chunk)
            ////if (currentInsertChunk > 0) { std::advance(it, currentInsertChunk); }
            it = myList->getLastChunk();
            --it;

            #if _DEBUG >= 3
            (*it).print();
            #endif

            chunkInsertionPoint++;

//            elem->print();

            if ((*it).getNumElem() < (*it).getLimitRange()) { //queue has enougth space
              //printf("%lld < %lld\n",(*it).getNumElem(),(*it).getLimitRange() - 1);
              (*it).add(elem);
              myList->incCountPos();
              found = true;
            } else { //queue doesn't has enougth space
              //1) close the current chunk
              //RANGE_TYPE limitElementPos = (*it).getLimitRange()-1;
              queueElem lastElemet = *((*it).getLastElement());
              (*it).setEndTs(lastElemet.getTupleTimeStamp()); // set the end of the chunk as the last tuple inserted
//              printf("update chunk - [ %.2f; %.2f ]\n", (*it).getChunkStartTs(), (*it).getChunkEndTs() );

              T1 oldEndTs = (*it).getChunkEndTs();
              //2) create a new one and add the tuple
              myList->addSpecialChunkARRAY(elem->getTupleTimeStamp());
              ++it;  //move to the new
              (*it).add(elem);
              //printf("CREATE NEW CHUNK: %lld - %lld\n",(*it).getNumElem(),(*it).getLimitRange() - 1);
              myList->incCountPos();
              found = true;
              chunkInsertionPoint++;
            }
        }
      };

      /**
      * create the VECTOR SPECIFICATION for SOME of the fList function
      */
      template<class T1, class queueElem>
      class fListSpecFunction<T1, queueElem, std::vector<queueElem>> {
        using chunkT = chunk<T1, queueElem, std::vector<queueElem>>; //just to be dynamic
        using fListT = fList<T1, queueElem, std::vector<queueElem>>; //just to be dynamic

        fListT *myList;// = (fListT*)calloc(1,sizeof(fListT));
      public:
        fListSpecFunction(fListT *myListVal) {
          myList = myListVal;
        }
        bool isArray(){ return false; }

        void push_back(queueElem *_elem) {
          //list<chunkT> superList = fList<T1,queueElem,queueTN>::internalList;
          bool found = false;

          /*
          queueElem *elem = (queueElem *)calloc(1, sizeof(queueElem));//TODO: DA SISTEMARE OCCUPAZIONE MEMORIA
          memcpy(elem,_elem,sizeof(queueElem));
          */
          queueElem *elem = _elem;
          typename std::list<chunkT>::iterator it = myList->end();
          --it;
          chunkInsertionPoint = myList->size();

          T1 slideVal = myList->getSlideVal();
          //printf("(*elem).getTupleTimeStamp(): %.2f | (*it).getChunkEndTs(): %.2f \n",(*elem).getTupleTimeStamp(),(*it).getChunkEndTs());
          if ((*elem).getTupleTimeStamp() < (*it).getChunkEndTs()+DIFF_MARGIN_ERROR) { //the tuple belong to the current chunk
            (*it).add(elem);
            myList->incCountPos();
            found = true;
          }else{
            // printf("endTs: %.2f\n",(*it).getChunkEndTs());
          }

          if(found == false){
            //the tuple belong to one of the next chunks
            if (((*elem).getTupleTimeStamp() - ((*it).getChunkEndTs()+DIFF_UNIT)) < slideVal) {
              //(1) BASE CASE: the tuple belong to the NEXT chunk => create it and add tuple
              (*it).setCloseChunk(true);
              myList->addChunk();
              ++it;
              chunkInsertionPoint++;
              (*it).add(elem);
              myList->incCountPos();
              found = true;
            } else {
              //(2) SPECIAL CASE: the tuple belong in one of the SUCCESSIVE chunk => create it and add tuple

              T1 tmpEvaluation = (*it).getChunkEndTs();
              int howManyChunk = 0;
              //to create the CORRECT chunk interval
              while ((*elem).getTupleTimeStamp() > tmpEvaluation+DIFF_UNIT   && howManyChunk < MAX_SCROLL_CHUNK_SEARCH) {
                tmpEvaluation += slideVal;
                (*it).setCloseChunk(true);
                myList->addChunk();
                ++it;
                chunkInsertionPoint++;
                howManyChunk++;
              }
              if (howManyChunk < MAX_SCROLL_CHUNK_SEARCH) {
                //if go FW but respect the max scroll add it as next and add tuple
                // (*it).setCloseChunk(true);
                // myList->addSpecialChunk(tmpEvaluation - slideVal + DIFF_UNIT);
                // ++it;
                (*it).add(elem);
                myList->incCountPos();
                found = true;
              } else {
                FQUEUE_outputError("too many Forward scroll done! check it! tuple: {" + (*elem).toString() +
                "} non inserted");
              }
            }
          }
          //printf("\tTUPLE inserted in chunk: %lld\n",chunkInsertionPoint);

        }
      };




/**
* Represent the FQUEUE object that the user invoke to manage the tuple
* As explained in the top of the docuemnt:
* - T1:          is correlated to the tuple internal type to order the tuple
*                (ex. double)
* - queueElem:     represent the tuple array or vector that specify the internal
*                chunk queue (ex. tuple_t[10], vector<tuple_t>)
* - queueTN: represent the type of the tuple (ex. tuple_t)
*/
template<typename T1, typename queueElem, typename queueTN>
struct fqueue {
private:
using chunkT = chunk<T1, queueElem, queueTN>;
using fListT = fList<T1, queueElem, queueTN>;
using fQueueT = fqueue<T1, queueElem, queueTN>;//just to be dynamic
T1 firstTs,       //first timestamps in the queue
   lastTs,    //last timestamps in the queue
   chunkSize, //the slide size or the chunk size
   winSize,   //the size of the window  (needed for the iterator)
   ratio,
   mcdValue;
RANGE_TYPE firstElem,          //first element in the queue
  lastElem,           //last element in the queue
  fromWin;
  fList<T1, queueElem, queueTN> *listQueue;
list<queueElem> listElem;
public:
typedef typename fListT::iterator iterator;
RANGE_TYPE idWin = 0, realIdWin = 0;

fqueue(T1 startTs, T1 slideSize, T1 winSizeVal) {
  if (startTs < 0 || slideSize < 0 || winSizeVal <= 0) {
    throw invalid_argument("The start timestamp and the slide value need to be >= 0");
    return;
  }
  firstTs = lastTs = 0;
  firstElem = lastElem = 0, fromWin = 0;
  winSize = winSizeVal;
  mcdValue = mcd(winSize,slideSize);
  ratio = slideSize/mcdValue;
  slideSize = slideSize / ratio ;
  listQueue = new fList<T1, queueElem, queueTN>(startTs, startTs + slideSize, winSize, 0);
  #if _DEBUG >= 2
  printf("startTs: %.2f \nslide:   %.2f \nwinSize: %.2f \n", startTs, slideSize, winSize);
  #endif
}

//get function
RANGE_TYPE size() { return listQueue->size(); }

RANGE_TYPE erase(T1 ts){
  RANGE_TYPE del = listQueue->cancella(ts);
  return del;
}

bool empty() { return listQueue->empty(); }
void deleteSmaller(T1 tsValue) { listQueue->removeChunkSmaller(tsValue); }

/**
* add the tuple to the queue
* this function let to abstract to the user all the underline manage system
*/
void push_back(queueElem elem) { listQueue->push_back(&elem); }

void incIsWinClose(){listQueue->incIsWinClose();}
void decIsWinClose(){listQueue->decIsWinClose();}

queueElem begin() { return listQueue->begin(); }
queueElem end() { return listQueue->end(); }

queueElem getLastTuple() { return (*listQueue->end()); }

T1 mcd(T1 a, T1 b){
  if(a*b==0) return 0;
  while(a!=b)
  {
    if(a>b)a-=b;
    else b-=a;
  }
  return a;
}


/**
* retrieve the window starting from chunk
*/
/*
iterator *getWin(RANGE_TYPE winNum, T1 endTs) {
  iterator *it = listQueue->getUpdateWin(winNum, endTs);
  if (it != NULL) {
    idWin = winNum; //evaluate the win number
    (*it).setWinId(winNum);
  }
  return it;
}
*/
    /*
    list<RANGE_TYPE> *getListChunk(T1 startTs, T1 endTs) {
    iterator *it = listQueue->getWinFromTsInterval(startTs, endTs);
    if (it != NULL) {
    idWin = (*it).getTsStart() / listQueue->getSlideVal(); //evaluate the win number
    (*it).setWinId(idWin);
  }
  return it;
}
*/
iterator *getITListChunk(T1 startTs, T1 endTs) {
  iterator *it = listQueue->getWinFromTsInterval(startTs, endTs);
  if (it != NULL) {
    if( (*it).getTsStart() != startTs ||
    (*it).getTsEnd()   != endTs    ){
      return NULL;
    }
    idWin = (*it).getTsStart() / listQueue->getSlideVal(); //evaluate the win number
    (*it).setWinId(idWin);
  }
  return it;
}

iterator *getITListChunk_sbt(T1 startTs, T1 endTs) {
  iterator *it = listQueue->getWinFromTsInterval_sbt(startTs, endTs);
  if (it != NULL) {
    idWin = (*it).getTsStart() / listQueue->getSlideVal(); //evaluate the win number
    (*it).setWinId(idWin);
  }
  return it;
}

/*
iterator *getWin(RANGE_TYPE winNum, T1 startTs, T1 endTs) {
iterator *it = listQueue->getWinFromPos(winNum, startTs, endTs);
if (it != NULL) {
idWin = (*it).getTsStart() / listQueue->getSlideVal(); //evaluate the win number
(*it).setWinId(idWin);
}
else
idWin = winNum;
return it;
}
*/

/**
* retrieve the next available window
*/
iterator *getNextAvailableWin(T1 startWin) {
  iterator *it = listQueue->getNextClosedWin(startWin, ratio);
  if (it != NULL) {
    idWin = (*it).getTsStart() / (listQueue->getSlideVal()*ratio); //evaluate the win number
    (*it).setWinId(idWin);
  }
  return it;
}

iterator *getNextAvailableWinSBT(T1 startWin, T1 endWin) {
  iterator *it = listQueue->getNextClosedWinSBT(startWin,endWin);
  if (it != NULL) {
    idWin = (*it).getTsStart() / (listQueue->getSlideVal()*ratio); //evaluate the win number
    (*it).setWinId(idWin);
  }
  return it;
}

/**
* retrieve the last window in the queue
*/
iterator *getLastWin(T1 startWin, T1 limitUP) {
  iterator *it = listQueue->getLastWin(startWin, limitUP);
  //iterator *it = listQueue->getNextClosedWin(startWin, ratio);
  if (it != NULL) {
    idWin = (*it).getTsStart() / listQueue->getSlideVal(); //evaluate the win number
    (*it).setWinId(idWin);
  }
  return it;
}

/**
* request to the last chunk to close it because finish to add elements
*/
void close() {
  listQueue->close();
}

/**
* invoke the list queue system for the print specification (DEBUG)
*/
void print() {
  listQueue->print();
}

};
#endif
