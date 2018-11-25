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

#if INCREMENTAL_QUERY == 1

static output_t* FUNCTION(queueType *queue, double start, double end, output_t* currentOutput) {
  if(&(currentOutput->valueNumElement[0]) == NULL || currentOutput->valueNumElement[0] == 0){
    currentOutput->value[0] = (double*)calloc(1,sizeof(double));
    currentOutput->valueNumElement[0] = 1;
  }
  if(currentOutput==NULL){
    return 0;
  }else{
    currentOutput->value[0][0] ++;
    return currentOutput;
  }
}

#else

/**
    Fitting function
  Levenberg-Marquardt
  fitting will be performed using a parabola
*/

  static double parabola( double t, const double *p )
  {
  	return p[0] + p[1]*t + p[2]*t*t;
  }

template <typename X>
output_t *FUNCTION(X itStart, X itEnd, output_t *ret) {
        //printf("USER FUNCTION: [%.2f; %.2f]\n",(*itStart).getTupleTimeStamp(),(*itEnd).getTupleTimeStamp());

        // set some fields of the output result
        lm_control_struct control = lm_control_double;

        X ORIGINAL_itStart  = itStart,
          ORIGINAL_itEnd    = itEnd;
        X tmpIt = itStart;

        int npoints=0;
        int mpoints=3,tmpMpoints=3;
        int j=0;

        int size = distance(itStart,itEnd);
        //ret->setNumElemInWin(size);

        double *x_bid   = (double*)calloc(size,sizeof(double));
        double *y_bid   = (double*)calloc(size,sizeof(double));
        double *x_ask   = (double*)calloc(size,sizeof(double));
        double *y_ask   = (double*)calloc(size,sizeof(double));
        ret->value      = (double**)calloc(2,sizeof(double));
        ret->value[0]   = (double*)calloc(3,sizeof(double));
        ret->value[1]   = (double*)calloc(3,sizeof(double));
        double *par_ask = (ret->value[0]);
        double *par_bid = (ret->value[1]);
        /*
        for(int i=0;i<3;i++){
            par_ask[i] = 0;
            par_bid[i] = 0;
        }
        */


        tuple_t curr_tuple;
        double *curr_values   = NULL;
        double curr_bid_price = 0,
              curr_bid_size  = 0,
              curr_ask_price = 0,
              curr_ask_size  = 0,
              curr_ts        = 0,
              curr_key       = 0,
              curr_id        = 0;


//BID
        int i=0;
        // building the vectors for bid quotes

        while ( itStart != itEnd ) {
            tmpIt = itStart;
            curr_values        = (*itStart).getTupleValue();
            curr_bid_price     = curr_values[0];
            curr_bid_size      = curr_values[1];
            curr_ask_price     = curr_values[2];
            curr_ask_size      = curr_values[3];
            curr_ts            = curr_tuple.getTupleTimeStamp();
            curr_key           = curr_tuple.getTupleKey();
            curr_id            = curr_tuple.getTupleId();

            if(curr_bid_size>0) {
                x_bid[npoints]=curr_ts; // ins_pointer is the older element
                y_bid[npoints]=curr_bid_price;
                j=(i+1);
                // check if subsequent point have the same x-value
                ++tmpIt;
                tuple_t curr_tuple_j = (*tmpIt);//queue->at(j);
                double *curr_values_j = curr_tuple_j.getTupleValue();
                while(j<size && curr_ts==curr_tuple_j.getTupleTimeStamp() && curr_values_j[1]>0) {
                    y_bid[npoints]+=curr_values_j[1];
                    j++;
                }

                y_bid[npoints]/=(j-i);
                // go ahead
                i=j; // j point to the next value with different timestamp
                npoints++; // next point to be derived
            }
            ++i;
            ++itStart;
        }

        // now we can perform the fitting
        lm_status_struct status;
        // just a guess
        if(par_bid[0]==0) par_bid[0]=y_bid[0];

        if(npoints>0) {
            tmpMpoints = (npoints < mpoints) ? npoints : mpoints;
            lmcurve(tmpMpoints, par_bid, npoints, x_bid, y_bid, parabola, &control, &status);
        }

//ASK

        // building the vectors for ask quotes
        npoints=0;
        itStart = ORIGINAL_itStart,
        itEnd   = ORIGINAL_itEnd;
        tmpIt   = itStart;

        i=0, j=0;
        while ( itStart != itEnd ) {
              tmpIt = itStart;
              curr_values        = (*itStart).getTupleValue();
              curr_bid_price     = curr_values[0];
              curr_bid_size      = curr_values[1];
              curr_ask_price     = curr_values[2];
              curr_ask_size      = curr_values[3];
              curr_ts            = curr_tuple.getTupleTimeStamp();
              curr_key           = curr_tuple.getTupleKey();
              curr_id            = curr_tuple.getTupleId();

              if(curr_ask_size>0) {
                  x_ask[npoints]=curr_ts; // ins_pointer is the older element
                  y_ask[npoints]=curr_ask_price;
                  j=(i+1);
                  // check if subsequent point have the same x-value
                  ++tmpIt;
                  tuple_t curr_tuple_j = (*tmpIt);//queue->at(j);
                  double *curr_values_j = curr_tuple_j.getTupleValue();
                  while(j<size && curr_ts==curr_tuple_j.getTupleTimeStamp() && curr_values_j[1]>0) {
                      y_ask[npoints]+=curr_values_j[1];
                      j++;
                  }
                  y_ask[npoints]/=(j-i);
                  // go ahead
                  i=j; // j point to the next value with different timestamp
                  npoints++; // next point to be derived
              }
              ++i;
              ++itStart;
          }

          // just a guess
          if(par_ask[0]==0) par_ask[0]=y_ask[0];

          if(npoints>0) {
              tmpMpoints = (npoints < mpoints) ? npoints : mpoints;
              lmcurve(tmpMpoints, par_ask, npoints, x_ask, y_ask, parabola, &control, &status);
          }

          //lmcurve(3, par_ask, npoints, x_ask, y_ask, parabola, &control, &status);

          //PRINT RESULT
          //printf("-- npoints: %d | ASK: {%.2f, %.2f, %.2f}\n",npoints,par_ask[0],par_ask[1],par_ask[2]);

        //usleep(FROM_MILLISEC_TO_MICRO(100)); //SLEEP

         free(x_bid);
         free(y_bid);
         free(x_ask);
         free(y_ask);

        return ret;

  }


//SOMMA
template <typename X>
output_t *FUNCTION_somma(X itStart, X itEnd, output_t *ret) {
    double tmpRes = 0;
    int count = 0;
    int c = 0;
    double val = 0, *v;

    while ( itStart != itEnd ) {
        val = 0;
        v = (*itStart).getTupleValue();
        if(v!=NULL) {
            //printf("\t\t>> %d) %s  \n", c, (*itStart).toString().c_str());
            for (int i = 0; i < DIM_TUPLE_VALUE; i++) {
                val += v[i];
            }
            tmpRes += val;
        }
        c++;

        ++itStart;
    }


    while ( itStart != itEnd ) {
        val = 0;
        v = (*itEnd).getTupleValue();
        if(v!=NULL) {
            //printf("\t\t>> %d) %s  \n", c, (*itStart).toString().c_str());
            for (int i = 0; i < DIM_TUPLE_VALUE; i++) {
                val += v[i];
            }
            tmpRes += val;
        }
        c++;

        --itEnd;
    }



    ret->value[0][0] = tmpRes;
    //usleep(FROM_SEC_TO_MILLISEC(500));
    return ret;
}

#endif
