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

#ifndef WIN_WF_FUNCTION
#define WIN_WF_FUNCTION 1
/********************************[FREQ_CONVERSION]*******************************/
#define clearScreen() printf("\033[H\033[J")

 ofstream statsFile,outputFile,winEvaluation;

#define DIFF_UNIT 1

// define
#if (defined(PIANOSA) || defined(PIANOSAU))
    #define FREQ 2000
#endif

#if defined(REPARA)
    #define FREQ 2400
#endif

#if defined(REPHRASE)
    #define FREQ 512
#endif


#define FROM_TICKS_TO_USECS(ticks) (((double) ticks)/FREQ)
#define FROM_USECS_TO_TICKS(usecs) (ticks) (FREQ * usecs)

#define FROM_SEC_TO_MICRO(sec) sec*1000000
#define FROM_MICRO_TO_SEC(sec) sec/1000000

#define FROM_MILLISEC_TO_SEC(sec) sec/1000
#define FROM_SEC_TO_MILLISEC(sec) sec*1000

#define FROM_MICRO_TO_MILLISEC(sec) sec/1000
#define FROM_MILLISEC_TO_MICRO(sec) sec*1000



/*******************************[COLOR]******************************************/
#if COLORED_OUTPUT == 1

  #define ANSI_BOLD_ON "\x1b[1m"

  #define ANSI_BG_COLOR_BLACK "\x1b[40m"
  #define ANSI_BG_COLOR_RED "\x1b[41m"
  #define ANSI_BG_COLOR_GREEN "\x1b[42m"
  #define ANSI_BG_COLOR_YELLOW "\x1b[43m"
  #define ANSI_BG_COLOR_BLUE "\x1b[44m"
  #define ANSI_BG_COLOR_MAGENTA "\x1b[45m"
  #define ANSI_BG_COLOR_CYAN "\x1b[46m"
  #define ANSI_BG_COLOR_WHITE "\x1b[47m"

  #define ANSI_COLOR_BLACK "\x1b[30m"
  #define ANSI_COLOR_RED "\x1b[31m"
  #define ANSI_COLOR_GREEN "\x1b[32m"
  #define ANSI_COLOR_YELLOW "\x1b[33m"
  #define ANSI_COLOR_BLUE "\x1b[34m"
  #define ANSI_COLOR_MAGENTA "\x1b[35m"
  #define ANSI_COLOR_CYAN "\x1b[36m"
  #define ANSI_COLOR_WHITE "\x1b[37m"
  #define ANSI_COLOR_RESET "\x1b[0m"

#elif COLORED_OUTPUT == 0

  #define ANSI_BOLD_ON ""

  #define ANSI_BG_COLOR_BLACK ""
  #define ANSI_BG_COLOR_RED ""
  #define ANSI_BG_COLOR_GREEN ""
  #define ANSI_BG_COLOR_YELLOW ""
  #define ANSI_BG_COLOR_BLUE ""
  #define ANSI_BG_COLOR_MAGENTA ""
  #define ANSI_BG_COLOR_CYAN ""
  #define ANSI_BG_COLOR_WHITE ""

  #define ANSI_COLOR_BLACK ""
  #define ANSI_COLOR_RED ""
  #define ANSI_COLOR_GREEN ""
  #define ANSI_COLOR_YELLOW ""
  #define ANSI_COLOR_BLUE ""
  #define ANSI_COLOR_MAGENTA ""
  #define ANSI_COLOR_CYAN ""
  #define ANSI_COLOR_WHITE ""
  #define ANSI_COLOR_RESET ""

#endif


string decideColorPrint(int workerId) {
#if COLORED_OUTPUT == 1
  string color;
  switch (workerId % 7) {
  case 0:
    color = ANSI_COLOR_WHITE;
    break;
  case 1:
    color = ANSI_COLOR_GREEN;
    break;
  case 2:
    color = ANSI_COLOR_YELLOW;
    break;
  case 3:
    color = ANSI_COLOR_MAGENTA;
    break;
  case 4:
    color = ANSI_COLOR_RED;
    break;
  case 5:
    color = ANSI_COLOR_CYAN;
    break;
  }
  return color;

#elif COLORED_OUTPUT == 0
  return "";
#endif
}

string decideBGColorPrint(int workerId) {
#if COLORED_OUTPUT == 1
  string color;
  switch (workerId % 7) {
  case 0:
    color = ANSI_BG_COLOR_WHITE;
    break;
  case 1:
    color = ANSI_BG_COLOR_GREEN;
    break;
  case 2:
    color = ANSI_BG_COLOR_YELLOW;
    break;
  case 3:
    color = ANSI_BG_COLOR_MAGENTA;
    break;
  case 4:
    color = ANSI_BG_COLOR_RED;
    break;
  case 5:
    color = ANSI_BG_COLOR_CYAN;
    break;
  }
  return color;
#elif COLORED_OUTPUT == 0
  return "";
#endif
}

/********************************************************************************
 * AUXILIARY FUNCTION
 ********************************************************************************/

string fromMicroToFormattedTime(double timestamp){
    #define MILLE 1000.0
    long milliseconds   = fmod((long) (timestamp / MILLE),MILLE);
    long seconds        = fmod((((long) (timestamp / MILLE) - milliseconds)/MILLE),60) ;
    long minutes        = fmod((((((long) (timestamp / MILLE) - milliseconds)/MILLE) - seconds)/60),60);
    long hours          = ((((((long) (timestamp / MILLE) - milliseconds)/MILLE) - seconds)/60) - minutes)/60;
    char ret[30];
    sprintf(ret,"%0.2lu:%0.2lu:%0.2lu.%0.4lu",hours,minutes,seconds,milliseconds);
    return ret;
    //return std::to_string(hours)+":"+std::to_string(minutes)+":"+std::to_string(seconds)+"."+std::to_string(milliseconds);
}


string convertDoubleToString(double t){
    std::ostringstream strs;
    strs << t;
    std::string str = strs.str();
    return str;
}

string convertIntToString(long long int t){
    std::ostringstream strs;
    strs << t;
    std::string str = strs.str();
    return str;
}

void outputError(string s) {
    cerr << s << endl;
    throw runtime_error(s);
}


double getCurren_ms() {
  return system_clock::now().time_since_epoch() / milliseconds(1);
}

void printTuplesInWindow(deque<tuple_t> elem, int wId, double ts,
                         double startTs, double endTs) {
  string color = decideColorPrint(wId);
  if (elem.size() == 0)
    return;
  printf("%s* COMPUTE State on W[%zd] => State:[%.2f,%.2f]: %s\n",
         color.c_str(), wId, startTs, endTs, ANSI_COLOR_RESET);
  for (unsigned i = 0; i < elem.size(); i++) {
    tuple_t *it = &(elem.at(i));
    string valueStr;
     double *v = (*it).getTupleValue();
    for(int i=0;i<DIM_TUPLE_VALUE;i++){
        valueStr += std::to_string(v[i]) + ", ";
      }
    printf("%s\tW[%zd] -> {ts: %.2f, id: %d, key: %d, val: %s} %s\n",
           color.c_str(), wId, (*it).getTupleTimeStamp(), (*it).getTupleId(), (*it).getTupleKey(), valueStr.c_str(),
           ANSI_COLOR_RESET);
  }
}

void printTuplesInWindow2(deque<tuple_t> elem) {
  string color = decideColorPrint(0);
  printf("%s***************** State *****************%s\n", color.c_str(), ANSI_COLOR_RESET);
  for (unsigned i = 0; i < elem.size(); i++) {
    tuple_t *it = &(elem.at(i));
    string valueStr;
    double *v = (*it).getTupleValue();
    for(int i=0;i<DIM_TUPLE_VALUE;i++){
        valueStr += std::to_string(v[i]) + ", ";
      }
    printf("%s {ts: %.2f, id: %d, key: %d, val: %s} %s\n", color.c_str(), (*it).getTupleTimeStamp(), (*it).getTupleId(), (*it).getTupleKey(),valueStr.c_str(), ANSI_COLOR_RESET);
  }
  printf("%s**********************************%s\n", color.c_str(), ANSI_COLOR_RESET);
}

void printTuplesOutWindow(deque<output_t> elem) {
  string color = decideColorPrint(0);

  for (unsigned i = 0; i < elem.size() - 1; i++) {
    output_t *it = &(elem.at(i));
    printf("%s", color.c_str());
    (*it).print();
    printf("%s", ANSI_COLOR_RESET);
  }
}

void printTuplesOutWindow2(output_t* elem, int numElem) {
  string color = decideColorPrint(0);
  output_t* ptrElem = elem;

  printf("#elem: %d \n",numElem);

  for(int i=0; i<=numElem; i++){
    ptrElem->print();
    ptrElem++;
  }
}

/**
 * Check if string s2 is in string s1
 * @param  s1 string
 * @param  s2 string
 * @return true if found;  false otherwise
 */
bool ifDebug(string s1, string s2) {
  if (s1.find("0") != std::string::npos) return false; //if found 0 no output
  return (s1.find(s2) != std::string::npos)? true : false;
}


/****************************************[FILE_MANAGER]********************************/
class WriteOnFile {
private:
  ofstream myfile;
  string fileName;

public:
  WriteOnFile() {}
  WriteOnFile(string fileName) : fileName(fileName) { fOpen(fileName); }

  bool fOpen(string fileName){
    if (!fileName.empty()) {
      myfile.open(fileName);
      if(myfile.fail()){
        printf("\n[ERROR] FAIL to open file: %s\n\n",fileName.c_str() );
        return false;
      }
      return true;
    }
    return false;
  }

  //write to file
  bool write(string data) {
    myfile << data;
    if(myfile.fail()){
      printf("\n[ERROR] FAIL WRITE to file: %s\n\n",fileName.c_str() );
      return false;
    }
    myfile.flush();
    return true;
  }

  bool close() {
    myfile.close();
    if(myfile.fail()){
      printf("\n[ERROR] CLOSE FAIL - file: %s\n\n",fileName.c_str() );
      return false;
    }
    return true;
  }
};



/*!
 *  \class DatasetReaderBin
 *
 *  \brief Class to Parse the Dataset Binary File
 *
 *  This class is used by the Generator to read the dataset of tuples from
 *  a binary file.
 *
 *  This class is defined in \ref Pane_Farming/src/generator.cpp
 */
class DatasetReaderBin {
private:
	int fd;
	size_t num_tuples; // total number fo tuples in the dataset
	size_t next_id; // next tuple to read
	double *map; // mapped file as an array of doubles (each tuple is DIM+2 contiguous doubles)
	struct stat stbuf;
	size_t file_size; // size in bytes of the mapped file

	// method to count the number of tuples in the file
	inline size_t countNoTuples() {
    	if((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
    		perror("Error fstat on the dataset binary file");
    		exit(-1);
		}
		size_t file_size = stbuf.st_size;
		return file_size / (sizeof(double) * (DIM+2));
	}

public:
	// constructor
	DatasetReaderBin(char *filename) {
		fd = open(filename, O_RDWR);
		if(fd == -1) {
			perror("Error opening the dataset binary file");
    		exit(-1);
		}
		num_tuples = countNoTuples();
		next_id = 0;
		// map the file in memory
		file_size = sizeof(double) * (DIM+2) * num_tuples;
		map = (double *) mmap(0, file_size, PROT_READ, MAP_SHARED, fd, 0);
		if(map == MAP_FAILED) {
			close(fd);
			perror("Error mmapping the file");
			exit(-1);
    	}
	}

	// destructor
	~DatasetReaderBin() {
		if(munmap(map, file_size) == -1) {
			perror("Error un-mmapping the dataset binary file");
    	}
    	close(fd);
	}

	// method to get the number of tuples
	inline size_t getNoTuples() {
		return num_tuples;
	}

	// method to get the next tuple
	inline double* nextTuple() {
		if(next_id < num_tuples) {
			double *p;
			p = &(map[next_id * (DIM+2)]);
			next_id++;
			return p;
		}
		else return nullptr;
	}
};


/**
 * RANDOM DOUBLE function for the generator part
 * @param  fMin minimum value
 * @param  fMax maximal value
 * @return      double value in the range
 */
double* fRand(double fMin, double fMax)
{
    double f = (double)rand() / RAND_MAX;
    f = fMin + f * (fMax - fMin);
    f = roundf(f * 100) / 100;
    return new double(f);
}
/**
 * RANDOM INTEGER function for the generator part
 * @param  fMin minimum value
 * @param  fMax maximal value
 * @return      integer value in the range
 */
int* randKey(int fMin, int fMax)
{
    int f = (int)rand() / RAND_MAX;
    f = fMin + f * (fMax - fMin);
    return new int(f);
}

double endInterval(double ts, double wLength){
  if(ts < wLength ) return wLength;
  double tmp = (double) (ts+wLength);
  double ret = tmp-fmod(tmp,wLength);
  return ret;
}

double startInterval(double ts, double wLength){
  double ret = (double) endInterval(ts, wLength)-wLength;
  if(ret>=0) return ret;
  else return 0;
}



/********************************************************************************
 support for window evaluation
 ********************************************************************************/

double myRound(double d, int pp) // pow() doesn't work with unsigned, so made this switch.
{
	return int(d * pow(10.0, pp) + .5) /  pow(10.0, pp);
}

string toString(double num, int precision){
  std::stringstream str;
  string s;
  str << to_string(num);
  s = str.str();
  int posDot = s.find(".");
  return s.substr(0,posDot+1+precision);
}

/**
 * EVALUATE THE # of win that the algorithm HAVE TO return
 */
int expectWinNumber(double maxTime, double winSize, double slide){
  if(maxTime <= 0 || winSize <= 0 || slide < 0) return -1;
  return (int)ceil( (maxTime-winSize) / slide ) +1;
}

double bootTime         = 0,
       generatorTime    = 0,
       generatorMed     = 0,
       generatorPrev    = 0,
       generatorMedDiff = 0;

long long int TOTAL_TUPLE = 0;

void initBootTime(){
  bootTime = getusec();
}

double endBoot(){
  bootTime = getusec()-bootTime;
  return bootTime;
}

double getBootTime(){
  return getusec()-bootTime;
}



void initGeneratorTime(){
  generatorTime = getusec();
}

double endGenerator(){
  generatorTime = getusec()-generatorTime;
  return generatorTime;
}

double getGeneratorTime(){
  return getusec()-generatorTime;
}

double getGeneratorMid(){
  double timeInSec = FROM_MICRO_TO_SEC(getGeneratorTime());
  generatorMed     = myRound(TOTAL_TUPLE/timeInSec,2);
  return generatorMed;
}

double getGeneratorMidDiff(){
  generatorMedDiff = generatorPrev - generatorMed;
  generatorPrev    = generatorMed;
  return myRound(generatorMedDiff,2);
}



bool isDeltaTime(double *prev, double delta){
  bool ret = ((getusec()-*prev) >= delta)?true:false;
  if(ret == true){
    *prev = getusec();
  }
  return ret;
}





double evaluateTuplePerDelta(long long int tuple, double delta){
  if(delta <= 0) return -1;
  return myRound(tuple/delta ,2);
}



/********************************************************************************
 WRITE TO FILE
 ********************************************************************************/
void writeStats(string msg){
  #if defined(_STATS) && defined(FILE_STATS_NAME)
  if(!statsFile.is_open()){
    statsFile.open (FILE_STATS_NAME,ios::out);
  }
  statsFile << msg.c_str() << endl;
  #endif
}

void closeStats(){
   #if defined(_STATS) && defined(FILE_STATS_NAME)
   statsFile.close();
   #endif
}


void writeOuput(string msg){
  #if defined(FILE_OUPUT_NAME)
  if(!outputFile.is_open()){
    outputFile.open (FILE_OUPUT_NAME,ios::out);
  }
  outputFile << msg.c_str() << endl;
  #endif
}

void closeOutput(){
   #if defined(FILE_OUPUT_NAME)
   outputFile.close();
   #endif
}



void writeOther(string msg){
  #if defined(FILE_OTHERS)
  if(!winEvaluation.is_open()){
    winEvaluation.open (FILE_OTHERS,ios::out);
  }
  winEvaluation << msg.c_str() << endl;
  #endif
}

void closeWriteOther(){
   #if defined(FILE_OTHERS)
   winEvaluation.close();
   #endif
}




#endif
