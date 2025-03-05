#include "3rd/json.hpp"
#include <iostream>
#include <libserial/SerialPort.h>
#include <libserial/SerialStream.h>
#include <chrono>
#include <thread>
#include <zmq.hpp>
#include <mutex>
#include <thread>
#include "gray.h"
#include <set>
using json = nlohmann::json;


namespace Config
{
const bool ApplyGrayCode=false;
const std::map<int, int> BitMapping=
    {
        {0,0},
        {1,1},
        {2,2},
        {3,3},
        {4,4},
        {5,5},
        {6,9},// ommit 6,7,8
        {7,10},
        {8,11},
        {9,12},
        {10,13},
        {11,14},
        {12,15},
        {13,16},
        {14,17}, //omit 18,19,20
        {15,21},
        {16,22},
        {17,23},
        {18,24},
        {19,25},
        {20,26},
        {21,27},

};
}


const std::string SERIAL_PORT = "/dev/ttyUSB0";
const auto SERIAL_BAUDRATE = LibSerial::BaudRate::BAUD_115200;

struct DataSendToUc{
    uint32_t ledState;
    std::array<uint8_t,3> colorActive;
    std::array<uint8_t,3> colorInactive;
    uint8_t brightness;
};

uint32_t ConvertColorToUc(const std::array<uint8_t,3>& color){
    return uint32_t(0xff000000) | (color[0] << 16) | (color[1] << 8) | color[2];
}


// Create a mapping from the 32 leds to the 32 leds on the board, the inactive leds are mapped to -1
std::vector<int> CreateMappingWithInactiveLeds(std::set<int> inactiveLeds)
{
  int deactivatedCount =0;
  std::vector<int> data;
  for (int i=0; i < 32; i ++)
  {
    if (inactiveLeds.find(i) != inactiveLeds.end())
    {
      deactivatedCount++;
    }
    data.push_back(i+deactivatedCount);
  }
  return data;
}

uint32_t ApplyMapping(uint32_t ledState)
{
  uint32_t result = 0;
  for (const auto & [i, mappedIndex] : Config::BitMapping)
  {
    if (mappedIndex>=0 && mappedIndex<32)
    {
      bool orginalBitState = (ledState & (1 << i)) != 0;
      result |= (orginalBitState << mappedIndex);
    }
  }
  return result;
}

void sendToUc(LibSerial::SerialStream& serialStream, const DataSendToUc& data){
    json j;
    j["ledState"] = data.ledState;
    j["active"] = ConvertColorToUc(data.colorActive);
    j["inactive"] = ConvertColorToUc(data.colorInactive);
    j["brightness"] = data.brightness;
    std::string s = j.dump();
    std::cout << s << std::endl;
    serialStream << s << std::endl;
    serialStream.FlushIOBuffers();
}

struct StampedTime{
  double timeStamp;
  std::chrono::steady_clock::time_point time;
};


void ZMQThread(StampedTime &timeStamp, std::mutex &lock){
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, ZMQ_SUB);
    subscriber.connect("tcp://localhost:5556");
    subscriber.setsockopt(ZMQ_CONFLATE, 1); // Keep only the latest task
    subscriber.setsockopt( ZMQ_SUBSCRIBE, "", 0 );

    while (subscriber)
    {
        zmq::message_t update;
        subscriber.recv( &update );
        std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
        std::string msg = update.to_string();
        std::cout << "Received: " << msg << std::endl;
        const auto data = json::parse(msg);
        if (data.contains("time")){
            std::lock_guard<std::mutex> lck(lock);
            timeStamp.timeStamp = static_cast<double>(data["time"])/1e9;
            std::swap(timeStamp.time,time);
        }

    }
}

void sendTimestamp(float ts, LibSerial::SerialStream& serialStream)
{
    // offset time to current time
    // lets send number of 1/100 seconds
    const uint32_t timeToUc = static_cast<uint32_t>(floor(ts/0.1));
    // send to UC
    DataSendToUc data;
    data.brightness = 52;
    data.colorActive = {0,0,0};
    data.colorInactive = {255,0,0};
    uint32_t coded = Config::ApplyGrayCode ? toGrayCode(timeToUc) : timeToUc ;
    uint32_t codedMapped = ApplyMapping(coded);
    data.ledState = codedMapped;

    sendToUc(serialStream, data);
}
int main(int argc, char** argv)
{
    std::mutex  lock;
    StampedTime timeStamp;

    // open serial port
    LibSerial::SerialStream serialStream;
    serialStream.Open(SERIAL_PORT);
    serialStream.SetBaudRate(SERIAL_BAUDRATE);
    if (argc>=2)
    {
        const std::string command (argv[1]);
        if (command=="--testTimestamp" && argc >=3)
        {
            const std::string arg(argv[2]);
            float testTimestamp=std::atof(arg.c_str());
            std::cout << " test mode, will display tiemstamp of " << testTimestamp << std::endl;
            sendTimestamp(testTimestamp, serialStream);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            serialStream.Close();
            return 0;
        }
        if (command=="--inactiveLeds")
        {
          DataSendToUc data;
          data.brightness = 52;
          data.colorActive = {0,0,0};
          data.colorInactive = {255,0,0};
          uint32_t coded = std::numeric_limits<uint32_t>::max();
          uint32_t codedMapped = ApplyMapping(coded);
          data.ledState = codedMapped;
          sendToUc(serialStream, data);
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          serialStream.Close();
          return 0;
        }
    }


    // open zmq listener
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, ZMQ_SUB);
    subscriber.connect("tcp://localhost:5556");
    subscriber.setsockopt(ZMQ_CONFLATE, 1); // Keep only the latest task
    subscriber.setsockopt( ZMQ_SUBSCRIBE, "", 0 );

    auto zmqThread = std::thread([&](){
        ZMQThread(timeStamp, lock);
    });
    while (subscriber)
    {
       StampedTime stampedTimeCp;
       {
           std::lock_guard<std::mutex> lck(lock);
           stampedTimeCp = timeStamp;
       }

       sendTimestamp(stampedTimeCp.timeStamp, serialStream);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
