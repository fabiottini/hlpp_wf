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

#include <deque>
#include <string>

using namespace std;

template <class T>
class input_struct {
  private:
    atomic<int> *numWorkers;
  public:
    void print();
    string toString();

    input_struct *clone();
    input_struct *createSpeciaTuple(double currentTs, int workerId); //just for agnostic case

    T   getTupleTimeStamp();
    int getTupleKey();
    T   *getTupleValue();
    int getTupleId();

    int incWorkers();
    int decWorkers();
    int setWorkers(int val);
    int getWorkersNumber();
    // decrement the counter and check if is the last return true
    bool decWorkersCheck();

};


/****************************************[OUTPUT]********************************/
template <class T>
class output_struct {
public:
  void print();
  string toString();
};




/****************************************[window descriptor]********************************/
template <class T1>
struct WindowDescriptor{
  public:
    WindowDescriptor(){};
    long int startPos = 0, endPos = 0, currIdWin = 0;
    T1   startDesc = 0, endDesc = 0;
    bool isClosed = false;
};


/****************************************[State]********************************/

template <class T1, class T2, class T3>
class State{
  private:
    long int fromIdToStartPos(long int idW);
    long int fromIdToEndPos(long int idW);
    T2 fromIdToStartValue(long int idW);
    T2 fromIdToEndValue(long int idW);
  public:
    int key;
    long workerId;
    deque<input_struct<T1>> *queue;
    deque<WindowDescriptor<T1>> *winDesc;

    State(){};
    State(int workerId, int key, T1 initWin, T1 winLength, T2 sliding);

    int getKey();
    T1 getInitInterval();
    T1 getEndInterval();
    T2 getSliding();
    deque<input_struct<T1>> **getTupledeque();

    bool update(input_struct<T2> *tuple);
    WindowDescriptor<T1>* retrieveWinId(long int idWindow);
    void removeOldWin();
    //long int getClosedWindow(); //return the actual closed window each time is invoked return the first pending window
    long int getPendentWin();
    T3* getFunctionRet(long int idWindow);
    T3* getIncResultQuery(long int idWindow);
    T3* getResult(long int idWindow);//given the id of the window apply the F on it and return the value

    long int evictionUpdate(T2 limit);
    long int eviction(T2 limit);

    long int getValuePosition(T2 value);

    //for the timebased
    void updateStart(T1 newStart);
    void updateEnd(T1 newEnd);

    void print();              //Print the representation of the State
    string toString();           //A string representation of the State
   
};

/***************************[TASK IN THE AGNOSTIC CASE]**************************/
template <class T1, class T2, class T3>
class Task{
  public:
    Task(){};
    Task(State<T1,T2,T3> **statePointer, long int winId); 
    string toString();
    State<T1,T2,T3>*        getWindow();
    deque<input_struct<T2>>* getQueue();
    long int        getStartPosition();
    long int        getEndPosition();
    long int        getWinId();
    T2          getCurrentValue();
    T2          getStartWinValue();
    T2          getEndWinValue();
};
