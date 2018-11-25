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

/**************************************************************************
 ** TEST FOR THE Win_WF_TB.hpp LIBRARY
 **************************************************************************
 * @Author Fabio L.
 * @Date November 2016
 **************************************************************************/

#include <ff/mapper.hpp>
#include <ff/node.hpp>
#include <ff/pipeline.hpp>
#include <iostream>
#include <cassert>
#include <deque>
#include <algorithm>    // std::sort
#include <vector>       // std::vector
#include <ff/farm.hpp>
#include <ff/node.hpp>
#include <ff/pipeline.hpp>
#include <fstream>
#include <functional>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdio.h>
#include <time.h>
#include <memory>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <mutex>
#include <iomanip>
#include <list>


#include "lmcurve.h"

/***************************************************************************
 * DEFINE CONFIGURATION
 ***************************************************************************
 * _DEBUG: 0 => disable debug
 *         1/2/3 => otherwise
 *
 * _STATS: 0 => disable statistic output
 *         1 => enable  statistic output
 ***************************************************************************
 * INCREMENTAL_QUERY: 0 => NORMAL QUERY
 *                    1 => INCREMENTAL QUERY
 *
 * MAINTAIN_TUPLE:    0 => DISABLE insertion tuple in the queue
 *                    1 => otherwise
 *
 * DIM_TUPLE_VALUE:   # => the length of T1 array in the tuple
 *                         (the array of the value)
 * OUT_NUM_VALUES:    # => the number of T1 output array
 ***************************************************************************/
//#define _DEBUG            0
#define STATS             1
#define STATS_MAXTIME     10000000 //10 sec

#define INCREMENTAL_QUERY 0
#define MAINTAIN_TUPLE    1
#define DIM_TUPLE_VALUE   4
#define OUT_NUM_VALUES    3 //the number of results PER block
#define OUT_NUM_BLOCKS    2 //the number of block of results with inside OUT_NUM_VALUES elements
#define COLORED_OUTPUT    1   //1: IF COLORED OUTPUT 0: no color
#define DIM               4
#define PIANOSA           1

#define FILE_STATS_NAME     "stats_agnostic_sbt"
#define FILE_OTHERS         "others_agnostic_sbt"
#define FILE_OUPUT_NAME     "output_agnostic_sbt"

using namespace ff;
using namespace std;
using namespace std::chrono;


#include "Win_WF_sw_tuple.hpp"
#include "fqueue.hpp"

using chunkT            = chunk<double, tuple_t, tuple_t[5000]>;
using queueType         = fqueue<double, tuple_t, tuple_t[5000]>;
using FUN_ITERATOR_TYPE = queueType::iterator;

#include "Win_WF_function.hpp"
#include "Win_WF_Collector.hpp"
#include "Win_WF_sw.hpp"
#include "Win_WF_agnostic_structure_sbt.hpp"
#include "Win_WF_Generator.hpp"
#include "Win_WF_userFunction.hpp"


int main(int argc, char *argv[]) {
  initBootTime(); //INIT THE TIMER



  if(argc<5){
      clearScreen();

    printf(ANSI_COLOR_RED "\n[PARAMETER ERROR]\n");
    printf("./test_wf_tb <nW> <wS> <sl> <lUP> <fileIn> <debug> <fileOut>\n");
    printf("\t <nW>        : workers number\n");
    printf("\t <wS>        : window size\n");
    printf("\t <sl>        : sliding factor\n");
    printf("\t <lUP>       : limit up (-1 = no limit up)\n");
    printf("\t <fileIn>    : file input generator (*.bin)\n");
    printf("\t <rate>      : rate tuple/sec. (ONLY FOR STATS)\n");
    printf("\t *<debug>    : composed string (es. -234)\n\t\t  -: print OUTPUT \n\t\t  0: NONE \n\t\t  1: Generator \n\t\t  2: Emitter \n\t\t  3: Worker \n\t\t  4: Win_WF_Collector \n" ANSI_COLOR_RESET);
    printf("\t *<color>    : 1: color true\n");
    printf("\t *<COMPARE_V>: 1: put in the file ONLY the comparable part\n");
    printf("\t *<fileOut>  : the name of the file where write the output\n");

    printf("\n\t * OPTIONAL PARAMETERS\n");
    return 0;
  }

  int nworkers        = atoi(argv[1]);
  double wSize        = (atof(argv[2])),
         slide        = (atof(argv[3])),
         initTupleTS  = 0,
         limitUP      = (atof(argv[4]));
  string inputFileGen = argv[5],
         enableDebug  = (argc>=7)?argv[6]:"-";
 int    colorOutput  = (argc>=8)?(atoi(argv[7])==1)?1:0:1,
        compare      = (argc>=9)?(atoi(argv[8])==1)?1:0:0;
 string fileName     = (argc>=10)?argv[9]:"";

  if(colorOutput==1){ //if the output can manage color can also manage the clearScreen
      clearScreen();
  }

  /**************************************************************************
   *  CONSTRUCT THE FARM
   **************************************************************************/
  ff_Farm<> farm([&]() {
          std::vector<std::unique_ptr<ff_node> > W;
          for(size_t i=0;i<nworkers; ++i)
              W.push_back(make_unique<Worker>(&(FUNCTION<FUN_ITERATOR_TYPE>),limitUP,wSize,slide,initTupleTS, nworkers, enableDebug));
          return W;
      } ());


  Generator *G = new Generator(inputFileGen,nworkers,wSize,slide,initTupleTS,limitUP, enableDebug);
  Emitter   *E = new Emitter(&(FUNCTION),farm.getlb(),nworkers,wSize,slide,limitUP,enableDebug);
  Collector *C = new Collector(-1,initTupleTS, nworkers, wSize, slide, fileName, 0, compare, enableDebug, colorOutput);

  farm.remove_collector();
  farm.add_emitter(*E);
  farm.wrap_around();
  farm.add_collector(*C);

  ff_Pipe<> pipe(*G, farm);
  pipe.setFixedSize(false);
  if (pipe.run_and_wait_end()<0) error("running error");

  delete G;
  delete E;
  delete C;

  return 0;
}
