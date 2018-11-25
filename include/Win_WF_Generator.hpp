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

#define FROM_MICRO_TO_SEC(sec) sec/1000000


/***********************************************************************
 *  GENERATOR MY
 ***********************************************************************/
class EasyGenerator: public ff_node_t<tuple_t, tuple_t> {
private:
  double    wSize,slide;                    //State size & sliding factor
  double    val = 0, oldVal = 0, tmp = 0;   //support var
  int       idKey=0;
  tuple_t*  ret;
  int       limit;                          //tuple generator limit
  int       SEED;                           //parameter to let the replication of the experiment
  int       pardegree;
  string      enableDebug;

public:
  EasyGenerator(int limit, int pardegree, double wSize, double slide, int SEED, string enableDebug)
    :limit(limit), pardegree(pardegree), wSize(wSize), slide(slide), SEED(SEED), enableDebug(enableDebug){
      srand(SEED);
    }

  tuple_t* svc(tuple_t *t){
    double *value = (double*) calloc(DIM_TUPLE_VALUE,sizeof(double));
    for(int i=0; i < limit; ++i){
      idKey = 1;
      val = (((i+1)*1000))+1;
      for(int j=0;j<DIM_TUPLE_VALUE;j++){
        value[j]   = val;
      }

      //ret           = new tuple_t(val,i+1,idKey,value,pardegree);
      ff_send_out(ret);
      if (ifDebug(enableDebug, "1")) { (*ret).print(); }

    }
    return EOS;
  }
};


/***********************************************************************
 *  MENCAGLI GENERATOR
 ***********************************************************************
 * read from a *.bin file the tuples and send it to the next stage
 ***********************************************************************/
 class Generator : public ff_node_t<tuple_t, tuple_t> {
 private:
   double wSize, slide; // State size & sliding factor
   double val = 0, oldVal = 0, tmp = 0, initTupleTS = 0, limitUP=0;
   int pardegree = 0, idKey = 0;
   tuple_t *ret;
   string enableDebug, filename;

 public:
   Generator(string fIn, int pardegree, double wSize, double slide,  double initTupleTS, double _limitUP, string enableDebug)
       : filename(fIn), pardegree(pardegree), wSize(wSize), slide(slide),  initTupleTS(initTupleTS), enableDebug(enableDebug), limitUP(_limitUP) {
         initGeneratorTime(); //INIT THE GENERATOR TIMER
        }



   tuple_t *svc(tuple_t *t) {

      //printf("filename: %s  \n",filename.c_str());
      char *cstr = new char[filename.length() + 1];
      strcpy(cstr, filename.c_str());
      DatasetReaderBin dataset(cstr);
      if (ifDebug(enableDebug, "1")) { 	printf("Dataset acquired successfully\n" ); }
      // send all the tuples in the dataset following their generator timestamps
      double *tuple;
      tuple_t *tmpTuple;
      int sizeTuple=DIM+1;
      ticks start_ticks = getticks();
      size_t tuples_to_receive = 0; // no. of tuples to read from the dataset
      tuples_to_receive = dataset.getNoTuples();
     // volatile ticks gen_starting_ticks = getticks();
      double previouseTime = getusec();
      double stop=false;

/*
if(t!=NULL && t!=nullptr && (*t).isStopTuple()){
        stop = true;
        ff_send_out(EOS);
      }
 */

       auto TIME_START=getticks();

       double *value = NULL;
       for(size_t i=0; i<tuples_to_receive && stop==false; i++) {

          TOTAL_TUPLE++;// MAINTAINT THE TOTALE TUPLE

          // get next tuple (already serialized)
          tuple      = dataset.nextTuple();

          //alloc and fill-in data into tuple
           value = (double*) calloc(DIM_TUPLE_VALUE,sizeof(double));
          if (DIM_TUPLE_VALUE>DIM){
              long long int curr = 1;
              for (int i = 0; i < DIM; i++) {
                  double val = tuple[i];
                  for (int j = 0; j < DIM_TUPLE_VALUE; j++) {
                      value[j] = val;
                  }
              }
          }else {
              for (int j = 0; j < DIM_TUPLE_VALUE; j++) {
                  value[j] = tuple[j];
              }
          }

          if(FROM_MICRO_TO_MILLISEC(tuple[sizeTuple])>limitUP){
            //printf("GENERATOR STOPS: %.2f\n",tuple[sizeTuple]);
            stop=true;
          }else{
            //sizeTuple  = DIM+1;//sizeof(tuple)/sizeof(tuple[0])+DIM;
            tmpTuple   = new tuple_t(tuple[sizeTuple],(int)i,1,value,pardegree);
              //tuple_t(val,i+1,idKey,i+1,pardegree);
            //if (ifDebug(enableDebug, "1")) { tmpTuple->print(); }

            // wait according to the generator timestamps of the tuple
            volatile ticks end_ticks = start_ticks+FROM_USECS_TO_TICKS(tuple[sizeTuple]); // DIM+1 is the last field
            volatile ticks cycles    = getticks();

            while(cycles < end_ticks)
              cycles = getticks();

            ff_send_out(tmpTuple);

            if(isDeltaTime(&previouseTime,FROM_SEC_TO_MICRO(3))){
              writeStats("> GENERATOR: " +toString(getGeneratorMid(),2) +" tuple/sec | DIFF prev: " + toString(getGeneratorMidDiff(),2) + " tuple/sec" );
            }
          }
      }

      printf("GENERATOR TIME FOR: %f | GENERATOR STOPS tuple ts: %.2f \n",(FROM_TICKS_TO_USECS(getticks()-TIME_START)/1000000),tuple[sizeTuple]);

      if(value!=NULL) free(value);
       delete [] cstr;
      //double gen_time_us = FROM_TICKS_TO_USECS(getticks() - gen_starting_ticks);
      //printf("FINE GENERATOR %s \n",(*tmpTuple).toString().c_str());

       //close and memorize the generator work time
      writeOther("> GENERATOR ENDS IN: " + fromMicroToFormattedTime(endGenerator()) + " | GENERATE: " + toString(TOTAL_TUPLE,0) + " tuples" );

      return EOS;
   }


 };
