#include <random>
#include <stdio.h>

//#define DEBUGCC
//#define UTILITY_TRACE
using namespace std;

typedef int int32_t;
struct CPacket {};

class CCC {
public:
  CCC();
  virtual ~CCC();

private:
  CCC(const CCC &);
  CCC &operator=(const CCC &) { return *this; }

public:
  // Functionality:
  //    Callback function to be called (only) at the start of a UDT connection.
  //    note that this is different from CCC(), which is always called.
  // Parameters:
  //    None.
  // Returned value:
  //    None.

  virtual void init() {}

  // Functionality:
  //    Callback function to be called when a UDT connection is closed.
  // Parameters:
  //    None.
  // Returned value:
  //    None.

  virtual void close() {}

  // Functionality:
  //    Callback function to be called when an ACK packet is received.
  // Parameters:
  //    0) [in] ackno: the data sequence number acknowledged by this ACK.
  // Returned value:
  //    None.

  virtual void onACK(int32_t) {}

  // Functionality:
  //    Callback function to be called when a loss report is received.
  // Parameters:
  //    0) [in] losslist: list of sequence number of packets, in the format
  //    describled in packet.cpp.
  //    1) [in] size: length of the loss list.
  // Returned value:
  //    None.

  virtual void onLoss(const int32_t *, int) {}

  // Functionality:
  //    Callback function to be called when a timeout event occurs.
  // Parameters:
  //    None.
  // Returned value:
  //    None.

  virtual void onTimeout() {}

  // Functionality:
  //    Callback function to be called when a data is sent.
  // Parameters:
  //    0) [in] seqno: the data sequence number.
  //    1) [in] size: the payload size.
  // Returned value:
  //    None.

  virtual void onPktSent(const CPacket *) {}

  // Functionality:
  //    Callback function to be called when a data is received.
  // Parameters:
  //    0) [in] seqno: the data sequence number.
  //    1) [in] size: the payload size.
  // Returned value:
  //    None.

  virtual void onPktReceived(const CPacket *) {}

  // Functionality:
  //    Callback function to Process a user defined packet.
  // Parameters:
  //    0) [in] pkt: the user defined packet.
  // Returned value:
  //    None.

  virtual void processCustomMsg(const CPacket *) {}

protected:
  // Functionality:
  //    Set periodical acknowldging and the ACK period.
  // Parameters:
  //    0) [in] msINT: the period to send an ACK.
  // Returned value:
  //    None.

  void setACKTimer(int msINT);

  // Functionality:
  //    Set packet-based acknowldging and the number of packets to send an ACK.
  // Parameters:
  //    0) [in] pktINT: the number of packets to send an ACK.
  // Returned value:
  //    None.

  void setACKInterval(int pktINT);

  // Functionality:
  //    Set RTO value.
  // Parameters:
  //    0) [in] msRTO: RTO in macroseconds.
  // Returned value:
  //    None.

  void setRTO(int usRTO);

  // Functionality:
  //    Send a user defined control packet.
  // Parameters:
  //    0) [in] pkt: user defined packet.
  // Returned value:
  //    None.

  void sendCustomMsg(CPacket &pkt) const;

  // Functionality:
  //    Set user defined parameters.
  // Parameters:
  //    0) [in] param: the paramters in one buffer.
  //    1) [in] size: the size of the buffer.
  // Returned value:
  //    None.

  void setUserParam(const char *param, int size);

private:
  void setMSS(int mss);
  void setMaxCWndSize(int cwnd);
  void setBandwidth(int bw);
  void setSndCurrSeqNo(int seqno);
  void setRcvRate(int rcvrate);
  void setRTT(int rtt);

protected:
  const int32_t &m_iSYNInterval; // UDT constant parameter, SYN

  double m_dPktSndPeriod; // Packet sending period, in microseconds
  double m_dCWndSize;     // Congestion window size, in packets

  int m_iBandwidth;      // estimated bandwidth, packets per second
  double m_dMaxCWndSize; // maximum cwnd size, in packets

  int m_iMSS;          // Maximum Packet Size, including all packet headers
  int m_iSndCurrSeqNo; // current maximum seq no sent out
  int m_iRcvRate; // packet arrive rate at receiver side, packets per second
  int m_iRTT;     // current estimated RTT, microsecond

  char *m_pcParam; // user defined parameter
  int m_iPSize;    // size of m_pcParam

private:
  int m_iACKPeriod;   // Periodical timer to send an ACK, in milliseconds
  int m_iACKInterval; // How many packets to send one ACK, in packets

  bool m_bUserDefinedRTO; // if the RTO value is defined by users
  int m_iRTO;             // RTO value, microseconds
};

#define MAXCOUNT 1000
#define GRANULARITY 0.01
#define MUTATION_TH 300
#define JUMP_RANGE 0.05
#define NUMBER_OF_PROBE 4
#define MAX_COUNTINOUS_GUESS 5
#define MAX_COUNTINOUS_SEND 1
#define MAX_MONITOR_NUMBER 100

class BBCC : public CCC {
public:
  double utility_array[MAXCOUNT];
  double rate_array[MAXCOUNT];
  int array_pointer;
  int target_monitor;
  int starting_phase;
  int make_guess;
  int guess_result;
  int moving_phase;
  int moving_phase_initial;
  int find_max;
  int random_guess;
  int random_guess_result;
  int mutation_counter;
  double rate_new;
  int change_intense;
  double change_amount;
  int change_direction;
  double rate_bucket[NUMBER_OF_PROBE];
  int monitor_bucket[NUMBER_OF_PROBE];
  double utility_bucket[NUMBER_OF_PROBE];
  int recorded_number;
  double current_rate;
  double previous_rate;
  double current_utility;
  double previous_utility;
  int guess_time;
  int continous_guess_count;
  int continous_send;
  int continous_send_count;
  int recording_guess_result;
  int baseline;
  double utility_baseline;
  double start_rate_array[MAX_MONITOR_NUMBER];
  int start_previous_monitor;
  double start_previous_utility;
  double previous_rtt;

public:
  BBCC()

  {
    m_dPktSndPeriod = 10000;
    m_dCWndSize = 100000.0;
    setRTO(100000000);
    starting_phase = 1;
    target_monitor = 0;
    make_guess = 0;
    guess_result = 0;
    moving_phase = 0;
    moving_phase_initial = 0;
    change_direction = 0;
    change_intense = 1;
    guess_time = 0;
    continous_guess_count = 0;
    continous_send = 0;
    continous_send_count = 0;
    previous_utility = 0;
    previous_rate = 1;
    current_rate = 1;
    recording_guess_result = 0;
    recorded_number = 0;
    baseline = 0;
    utility_baseline = 0;

    for (int i = 0; i < MAX_MONITOR_NUMBER; i++)
      start_rate_array[i] = 0;

    start_previous_monitor = -1;
    start_previous_utility = -10000;
    previous_rtt = 0;
  }

public:
  virtual void onLoss(const int32_t *, const int &) {}
  virtual void onTimeout() {}
  /*	int findmax(double arr[]){
  int tmp=0;
  for(int i=0;i<1000;i++){
  if(arr[i]>arr[tmp]&&stale[i]<20)
  {
  tmp=i;

  }
  }
  return tmp;
  }*/

  virtual void onMonitorStart(int current_monitor) {

    if (starting_phase) {
      start_rate_array[current_monitor] = previous_rate * 2;
#ifdef DEBUGCC
      cerr << "double rate to " << start_rate_array[current_monitor] << endl;
#endif
      previous_rate = start_rate_array[current_monitor];
      setRate(start_rate_array[current_monitor]);
      return;
    }
    if (make_guess == 1)
	{
      // "make guess!" << continous_guess_count << endl;
      if (guess_time == 0 && continous_guess_count == MAX_COUNTINOUS_GUESS)
	  {
        // "skip guess" << endl;
        continous_guess_count = 0;
      }
      if (guess_time == 0)
	  {
        recording_guess_result = 1;
        continous_guess_count++;

        int rand_dir;

        for (int i = 0; i < NUMBER_OF_PROBE; i++)
		{
          rand_dir = (rand() % 2 * 2 - 1);
          rate_bucket[i] = current_rate + rand_dir * continous_guess_count * GRANULARITY * current_rate;
          rate_bucket[++i] = current_rate - rand_dir * continous_guess_count * GRANULARITY * current_rate;
          // cerr << "guess rate" << rate_bucket[i - 1] << " " << rate_bucket[i]
        }
        for (int i = 0; i < NUMBER_OF_PROBE; i++)
		{
          monitor_bucket[i] = (current_monitor + i) % MAX_MONITOR_NUMBER;
          // "guess monitor" << monitor_bucket[i] << endl;
        }
      }

      setRate(rate_bucket[guess_time]);
      cerr << "setrate as " << rate_bucket[guess_time] << endl;
      guess_time++;

      // TODO:Here the sender stopped at a particular rate
      if (guess_time == NUMBER_OF_PROBE)
	  {
        cerr << "Guess exit!" << endl;
        make_guess = 0;
        guess_time = 0;
      }
    }
    if (continous_send == 1)
	{
      // "CONTINOUS send"
      if (continous_send_count == 1)
	  {
        setRate(current_rate);
      }

      if (continous_send_count < MAX_COUNTINOUS_SEND)
	  {
        continous_send_count++;
        // cerr << "continous send" << endl;
      }
	  else
	  {
        // "clear continous send" << endl;
        continous_send = 0;
        continous_send_count = 0;
        continous_guess_count = 0;
        make_guess = 1;
      }
    }
  }

  // time = seconds
  //

  virtual void onMonitorEnds(int total, int loss, double time, int current, int endMonitor, double rtt)
  {
    double utility;
    double t = total;
    double l = loss;
    int random_direciton;
    if (l < 0)
      l = 0;

    if (rtt == 0) {
      // "RTT cannot be 0!!!"
    }

	if (previous_rtt == 0)
      previous_rtt = rtt;

    utility = ((t - l) / time * (1 - 1 / (1 + exp(-100 * (l / t - 0.05)))) - 1 * l / time);

    previous_rtt = rtt;
    if (endMonitor == 0 && starting_phase)
      utility /= 2;

    if (starting_phase)
	{
      if (endMonitor - 1 > start_previous_monitor)
	  {
        if (start_previous_monitor == -1)
		{
          starting_phase = 0;
          make_guess = 1;
          setRate(start_rate_array[0]);
          current_rate = start_rate_array[0];
          return;
        } else {
          // exit because of loss in monitor, fallback due to loss
          starting_phase = 0;
          make_guess = 1;
          setRate(start_rate_array[start_previous_monitor]);
          current_rate = start_rate_array[start_previous_monitor];
          return;
        }
      }

      if (start_previous_utility < utility)
	  {
        // "moving forward"
        // do nothing
        start_previous_utility = utility;
        start_previous_monitor = endMonitor;
        return;
      }
	  else
	  {
        starting_phase = 0;
        make_guess = 1;
        setRate(start_rate_array[start_previous_monitor]);
        current_rate = start_rate_array[start_previous_monitor];
        previous_rate = current_rate;
        return;
      }
    }

    if (recording_guess_result)
	{
      for (int i = 0; i < NUMBER_OF_PROBE; i++)
	  {
        if (endMonitor == monitor_bucket[i])
		{
          recorded_number++;
          utility_bucket[i] = utility;
        }
      }

      // TODO to let the sender go back to the current sending rate, one way is
      // to
      // let the decision maker stop for another monitor period,which might not
      // be a good option, let's try this first
      if (recorded_number == NUMBER_OF_PROBE)
	  {
        recorded_number = 0;
        double decision = 0;
        for (int i = 0; i < NUMBER_OF_PROBE; i++)
		{
          if (((utility_bucket[i] > utility_bucket[i + 1]) && (rate_bucket[i] > rate_bucket[i + 1])) ||
              ((utility_bucket[i] < utility_bucket[i + 1]) && (rate_bucket[i] < rate_bucket[i + 1])))
            decision += 1;
          else
            decision -= 1;
          i++;
        }

        if (decision == 0) {
          make_guess = 1;
          recording_guess_result = 0;
        } else {
          change_direction = decision > 0 ? 1 : -1;
          recording_guess_result = 0;
          target_monitor = (current + 1) % MAX_MONITOR_NUMBER;
          moving_phase_initial = 1;
          change_intense = 1;
          change_amount = (continous_guess_count / 2 + 1) * change_intense *
                          change_direction * GRANULARITY * current_rate;
          previous_utility = 0;
          continous_guess_count--;
          continous_guess_count = 0;
          if (continous_guess_count < 0)
            continous_guess_count = 0;
          previous_rate = current_rate;
          current_rate = current_rate + change_amount;
          target_monitor = (current + 1) % MAX_MONITOR_NUMBER;
          setRate(current_rate);
        }
      }
    }

    if (moving_phase_initial && endMonitor == target_monitor) {
      if (current_rate > (t * 12 / time / 1000 + 30) && current_rate > 200)
	  {
        current_rate = t * 12 / time / 1000;
        make_guess = 1;
        moving_phase = 0;
        moving_phase_initial = 0;
        change_direction = 0;
        change_intense = 1;
        guess_time = 0;
        continous_guess_count = 0;
        continous_send = 0;
        continous_send_count = 0;
        recording_guess_result = 0;
        recorded_number = 0;
        setRate(current_rate);
#ifdef DEBUGCC
        cerr << "system udp call speed limiting, resyncing rate" << endl;
#endif
        return;
      }

#ifdef DEBUGCC
      cerr << "first time moving" << endl;
#endif
      target_monitor = (current + 1) % MAX_MONITOR_NUMBER;
      previous_rate = current_rate;
      previous_utility = utility;
      change_intense += 1;
      change_amount =
          change_intense * GRANULARITY * current_rate * change_direction;
      current_rate = current_rate + change_amount;
      setRate(current_rate);
      moving_phase_initial = 0;
      moving_phase = 1;
    }

    if (moving_phase && endMonitor == target_monitor) {

      if (current_rate > (t * 12 / time / 1000 + 30) && current_rate > 200) {
        current_rate = t * 12 / time / 1000;

        make_guess = 1;
        moving_phase = 0;
        moving_phase_initial = 0;
        change_direction = 0;
        change_intense = 1;
        guess_time = 0;
        continous_guess_count = 0;
        continous_send = 0;
        continous_send_count = 0;
        recording_guess_result = 0;
        recorded_number = 0;
        setRate(current_rate);
#ifdef DEBUGCC
        cerr << "system udp call speed limiting, resyncing rate" << endl;
#endif
        return;
      }

#ifdef DEBUGCC
      cerr << "moving faster" << endl;
#endif
      current_utility = utility;
      if (current_utility > previous_utility) {
        target_monitor = (current + 1) % MAX_MONITOR_NUMBER;
        change_intense += 1;
        previous_utility = current_utility;
        previous_rate = current_rate;
        change_amount =
            change_intense * GRANULARITY * current_rate * change_direction;
        current_rate = current_rate + change_amount;
        setRate(current_rate);

      } else {
        moving_phase = 0;
        make_guess = 1;
        // change_intense+=1;
        previous_utility = current_utility;
        previous_rate = current_rate;
        change_amount =
            change_intense * GRANULARITY * current_rate * change_direction;
        current_rate = current_rate - change_amount;
        setRate(current_rate);
      }
    }
  }

  virtual void onACK(const int &ack) {}

  void setRate(double mbps) { m_dPktSndPeriod = (m_iMSS * 8.0) / mbps; }
};
