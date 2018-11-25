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

#ifndef WIN_WF_COLLECTOR
#define WIN_WF_COLLECTOR 1

/***********************************************************************
 *  COLLECTOR
 ***********************************************************************/
class Collector : public ff_node_t<output_t> {
private:
    WriteOnFile *fileWriter = NULL;
    using reorderBufferType = vector<output_t>;
    string enableDebug = NULL, fileName = NULL;
    int countOut = 0,     //count the current sendOut element
            inTheQueue = 0,     //how many elements in the queue
            lastAdd = 0,     //count HOW MANY elements is already in the queue
            maxWinSize = 0,
            numWorkers = 0,
            arrivato = 0,
            compare  = 0,
            countEOS = 0;
    double currentTs = 0,
            tsLastInTheQueue = 0,
            winSize_val = 0,
            sliding = 0,
            limitUP = 0,
            _expWin = 0;
    double div = 0;
    reorderBufferType *reorderBuffer;   //data Structure that let to reorder the tuples

    int colorOutput = 0;
    long long int idLastInTheQueue = 0;

    #if STATS == 1
        double avg_latency = 0,avg_latency_calc=0, avg_tot_latency=0, avg_tot_calc_latency=0;
        double  TIME_last_check = 0, TIME_last_diff = 0, TIME_boot = 0;
        long long int tot_rec_tuple = 0;
        long long int totPartialTuple = 0;
        double TIME_AVG_latency_last = 0;
        long long int highestWorkerID = 0;
        bool isFirst = true;
        double avg_numElemInWin = 0,
              avg_numElemInWin_old = 0;
        long long int avg_tot_sum_elem = 0;
        double dev_standard=0;
    #endif

/*******************************************************************************/

    bool checkIfPositionIsEmpty(int pos) {
        output_t tmp = reorderBuffer->at(pos);
        if (tmp.getTupleStartInterval() != tmp.getTupleEndInterval()
            && tmp.getTupleEndInterval() > 0
            && inTheQueue > 0)
            return true;
        else
            return false;
    }

    static bool comparedDequeOutput_t(const output_t &a, const output_t &b) {
        if(a.getTupleStartInterval() < b.getTupleStartInterval()) return true;
        return false;
    }

    static bool comparedIdDequeOutput_t(const output_t &a, const output_t &b) {
        if(a.getTupleId() < b.getTupleId()) return true;
        return false;
    }


    /**
     * if the tuple that is sent is not in-order insert it in the queue
     * @param t the tuple_t to insert in the queue
     */
    void insertInOrderInQueue(output_t *t) {
        inTheQueue++;
        tsLastInTheQueue = (*t).getTupleStartInterval();
        reorderBuffer->push_back(*t);
        if(reorderBuffer->size()>1){
            std::sort( reorderBuffer->begin() , reorderBuffer->end() , comparedDequeOutput_t );
        }
        if (ifDebug(enableDebug, "4")) {
            printf("INSERT IN QUEUE: %d - TS: %.2f ; WAIT FOR: %.2f | size queue: %lu \n", (*t).getTupleId() ,(*t).getTupleStartInterval(),currentTs,reorderBuffer->size());
        }
    }

    void insertInOrderInQueueByID(output_t *t) {
        inTheQueue++;
        idLastInTheQueue = (*t).getTupleId();
        reorderBuffer->push_back(*t);
        if(reorderBuffer->size()>1){
            std::sort( reorderBuffer->begin() , reorderBuffer->end() , comparedIdDequeOutput_t );
        }
        if (ifDebug(enableDebug, "4")) {
            printf("INSERT IN QUEUE: %d - WAIT FOR: %d | size queue: %lu \n", (*t).getTupleId() ,countOut,reorderBuffer->size());
        }
    }

    void printQueue(){
        reorderBufferType::iterator it = reorderBuffer->begin();
        for (;it!=reorderBuffer->end();++it) {  (*it).print();  }
    }

    /**
     * Check if next tuple is in the queue
     */
    void checkIfSomeoneInTheQueue() {
        if (inTheQueue <= 0) return;
        reorderBufferType::iterator it = reorderBuffer->begin();
        bool conditionEnd = false;
        while(it!=reorderBuffer->end() && conditionEnd==false) {
            if((*it).getTupleStartInterval() <= currentTs){
                tsLastInTheQueue = it->getTupleStartInterval();
                output_t *tmp = (output_t*) calloc(1,sizeof(output_t));
                *tmp = (*it);
                sendOut(tmp);
                reorderBuffer->erase(it);
                currentTs += sliding;
                inTheQueue--;
            }else{
                conditionEnd=true;
            }
        }

    }

    void checkIfSomeoneInTheQueueID() {
        if (inTheQueue <= 0) return;
        reorderBufferType::iterator it = reorderBuffer->begin();
        bool conditionEnd = false;
        while(it!=reorderBuffer->end() && conditionEnd==false) {
            if((*it).getTupleId() <= countOut){
                idLastInTheQueue = it->getTupleId();
                output_t *tmp = (output_t*) calloc(1,sizeof(output_t));
                *tmp = (*it);
                sendOut(tmp);
                reorderBuffer->erase(it);
                inTheQueue--;
            }else{
                conditionEnd=true;
            }
        }

    }


    /**
     * Print to the output AND write to the file if is specified
     * @param t the tuple to write
     */
    void sendOut(output_t *t) {
        countOut++;
        #if STATS==1
            tot_rec_tuple++;
            double latency      = FROM_TICKS_TO_USECS(getticks() - t->getEmitterLatencyTicks());
            double latencyCalc  = FROM_TICKS_TO_USECS(t->getCalculationTicks());
            avg_latency      += (1 / ((double) tot_rec_tuple)) * (latency - avg_latency);
            avg_latency_calc += (1 / ((double) tot_rec_tuple)) * (latencyCalc - avg_latency_calc);

            avg_numElemInWin_old = avg_numElemInWin;
            avg_numElemInWin    += (1 / ((double)tot_rec_tuple)) * (t->getNumElemInWin() - avg_numElemInWin);

            dev_standard         = dev_standard + (t->getNumElemInWin()-avg_numElemInWin_old)*(t->getNumElemInWin()-avg_numElemInWin);
            avg_numElemInWin_old = avg_numElemInWin;

            avg_tot_latency      += avg_latency;//(1 / ((double) tot_rec_tuple)) * (latency - avg_tot_latency);
            avg_tot_calc_latency += avg_latency_calc;//(1 / ((double) tot_rec_tuple)) * (latencyCalc - avg_tot_calc_latency);

            avg_tot_sum_elem++;
        #endif

        char buffer[1000];
        sprintf(buffer,"%3d)  OUTPUT: W[%3zd] => WIN: [%13.2f,%13.2f] {KEY: %2d, IDWin: %2d, TS: %20.2f, VAL: %13s}",
                countOut, (*t).getTupleWorkerId(), (*t).getTupleStartInterval(), (*t).getTupleEndInterval(),
                (*t).getTupleKey(), (*t).getTupleId(), (*t).getTupleTimeStamp(), (*t).toStringTupleValue().c_str());

        //PRINT OUTPUT
        if (ifDebug(enableDebug, "-")) {
            if(colorOutput==1){
              printf("%s%s%s%s\n",ANSI_COLOR_WHITE,ANSI_BG_COLOR_BLUE,buffer,ANSI_COLOR_RESET);
            }else{
              printf("%s\n",buffer);
            }
        }

        //OUTPUT FILE WRITE
        if (fileWriter) {
            if (compare == 0) {
                sprintf(buffer, "%s\n", buffer);
                fileWriter->write(buffer);
            }else {
                char buffer[1000];
                sprintf(buffer, "%3d) WIN: [%13.2f,%13.2f] => VAL: %13s\n",
                        countOut, (*t).getTupleStartInterval(), (*t).getTupleEndInterval(),
                        (*t).toStringTupleValue().c_str());
                fileWriter->write(buffer);
            }
        }

        if (getusec() - TIME_last_check >= STATS_MAXTIME) {
            printSTATS();
            TIME_last_check = getusec();
        }
        delete t;
    }

    void printSTATS(){
        double tmp = 0;
        TIME_last_diff =  getusec() - TIME_boot;
        if (TIME_AVG_latency_last>0) {
            tmp = (avg_latency-TIME_AVG_latency_last);
        }
        totPartialTuple+=tot_rec_tuple;
        if (isFirst){
            printf("%14s | %14s | #WORKER |   #RESULTS   | PARTIAL TOT. | AVG. LATENCY (ms) | AVG. CALC. LATENCY (ms) | AVG. #ELEM. WIN. \n"," INTERVAL ", " INT. (sec) ");
            printf("---------------+----------------+---------+--------------+--------------+-------------------+-------------------------+------------------  \n");
            isFirst=false;
        }
        string FORMAT = "%14s | %14.2f | %7lld | %s %10lld %s | %12lld | %17.2f | %23.2f | %16.2f \n";
        if(colorOutput==1){
          if((_expWin-tot_rec_tuple)>0){
              printf(FORMAT.c_str(),
                     fromMicroToFormattedTime(TIME_last_diff).c_str(),FROM_MICRO_TO_SEC(TIME_last_diff), highestWorkerID+1, ANSI_BG_COLOR_RED , tot_rec_tuple, ANSI_COLOR_RESET, totPartialTuple, avg_latency/1000, avg_latency_calc/1000, avg_numElemInWin);
          }else{
              printf(FORMAT.c_str(),
                     fromMicroToFormattedTime(TIME_last_diff).c_str(),FROM_MICRO_TO_SEC(TIME_last_diff), highestWorkerID+1, ANSI_BG_COLOR_GREEN , tot_rec_tuple, ANSI_COLOR_RESET, totPartialTuple,avg_latency/1000, avg_latency_calc/1000, avg_numElemInWin);
          }
        }else{
          printf(FORMAT.c_str(),
                 fromMicroToFormattedTime(TIME_last_diff).c_str(),FROM_MICRO_TO_SEC(TIME_last_diff), highestWorkerID+1, "" , tot_rec_tuple, "", totPartialTuple, avg_latency/1000, avg_latency_calc/1000, avg_numElemInWin);
        }
        highestWorkerID=0; //reset the worker id
        TIME_AVG_latency_last = avg_latency;
        tot_rec_tuple = 0;
        avg_latency = 0;
        avg_numElemInWin=0;
        avg_latency_calc = 0;
    }

public:

    Collector(double limitUP, double currentTs_val, int numWorkers, double winSize_val, double sliding_val,
              string fileName_val, int _compare, string enableDebug_val, int _colorOutput)
            : limitUP(limitUP), enableDebug(enableDebug_val), numWorkers(numWorkers), currentTs(currentTs_val),
              winSize_val(winSize_val), sliding(sliding_val), fileName(fileName_val), compare(_compare), colorOutput(_colorOutput) {
        fileWriter = (!fileName.empty()) ? new WriteOnFile(fileName) : NULL;
        if (limitUP != -1) {
            maxWinSize = ((limitUP - winSize_val) / sliding) + 1;
        }
        reorderBuffer = new reorderBufferType;
    }

    Collector(double limitUP, double currentTs_val, int numWorkers, double winSize_val, double sliding_val,
              string fileName_val, double _expWin, int _compare, string enableDebug_val, int _colorOutput)
            : limitUP(limitUP), enableDebug(enableDebug_val), numWorkers(numWorkers), currentTs(currentTs_val),
              winSize_val(winSize_val), sliding(sliding_val), fileName(fileName_val), _expWin(_expWin), compare(_compare), colorOutput(_colorOutput)  {
        fileWriter = (!fileName.empty()) ? new WriteOnFile(fileName) : NULL;
        if (limitUP != -1) {
            maxWinSize = ((limitUP - winSize_val) / sliding) + 1;
        }
        reorderBuffer = new reorderBufferType;
    }

    int svc_init(){
        #if STATS==1
            TIME_last_check = getusec();
            TIME_boot = getusec();
        #endif
        return 0;
    }

    output_t *svc(output_t *t) {

        highestWorkerID = ((*t).getTupleWorkerId()>highestWorkerID) ? (*t).getTupleWorkerId() :  highestWorkerID;

        if (ifDebug(enableDebug, "4")) {
            printf("COLLECTOR: +++ RICEVUTO %s +++ \n", (*t).toString().c_str());
        }

        //represent the limit upperbound of the file
        if (limitUP != -1 && ((*t).getTupleStartInterval() > limitUP)) {
          printf("COLLECTOR: +++ DROP TUPLE EXEED LIMIT %s +++ \n", (*t).toString().c_str());
            return GO_ON;
        }

        if (limitUP == -1) {
            if ((*t).getTupleId() != countOut) { //if out-of-order tuple
                insertInOrderInQueueByID(t);
            } else {                    //if in-order tuple
                sendOut(t);
            }
            checkIfSomeoneInTheQueueID();
        } else {
            if ((*t).getTupleStartInterval() > currentTs) {          //if out-of-order tuple
                insertInOrderInQueue(t);
            } else if ((*t).getTupleStartInterval() == currentTs) {  //if in-order tuple
                currentTs += sliding;
                sendOut(t);
            } else {
              //  printf("*** COLLECTOR DROP TUPLE: [%.2f,%.2f] \n", (*t).getTupleStartInterval(), (*t).getTupleEndInterval());
            }
            checkIfSomeoneInTheQueue();       //check if the next tuple waiting for is in the queue
        }

        #if STATS==1
        //printSTATS();
        #endif

        return GO_ON;
    }

    void eosnotify(ssize_t) {
        if (numWorkers < countEOS) { countEOS++; }
        checkIfSomeoneInTheQueue();         //check if there is something in the queue
    }

    void svc_end() {
        if (numWorkers >= countEOS) {
            printSTATS();
            printf("COLLECTOR: #WIN: %d\n", countOut);
            printf("AVG_TOT_LATENCY: %.2f\n", (avg_tot_latency/avg_tot_sum_elem)/1000);
            printf("AVG_TOT_CALC_LATENCY: %.2f\n", (avg_tot_calc_latency/avg_tot_sum_elem)/1000);
            printf("STANDARD_DEVIATION_ELEMENT_IN_WIN: %.2f\n", sqrt(dev_standard/countOut));

            if (fileWriter) { fileWriter->close(); }
        } else {
            printf("COLLECTOR WAIT FOR %d EOS\n", numWorkers - countEOS);
        }
        delete reorderBuffer;
    }

};

#endif
