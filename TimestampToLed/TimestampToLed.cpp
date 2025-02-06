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
using json = nlohmann::json;


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
void sendToUc(LibSerial::SerialStream& serialStream, const DataSendToUc& data){
    json j;
    j["ledState"] = data.ledState;
    j["active"] = ConvertColorToUc(data.colorActive);
    j["inactive"] = ConvertColorToUc(data.colorInactive);
    j["brightness"] = data.brightness;
    std::string s = j.dump();
    serialStream << s << std::endl;
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
int main()
{
    std::mutex  lock;
    StampedTime timeStamp;

    // open serial port
    LibSerial::SerialStream serialStream;
    serialStream.Open(SERIAL_PORT);
    serialStream.SetBaudRate(SERIAL_BAUDRATE);


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
       // offset time to current time
       auto now = std::chrono::steady_clock::now();
       std::chrono::duration<double>  diff = now - stampedTimeCp.time; //seconds

       double resolvedTime = stampedTimeCp.timeStamp + diff.count();
       std::cout << "Resolved Time: " << resolvedTime << std::endl;
       const uint32_t timeToUc = static_cast<uint32_t>(floor(resolvedTime/0.1));

        // send to UC
        DataSendToUc data;
        data.brightness = 5;
        data.colorActive = {0,255,0};
        data.colorInactive = {255,0,0};
        data.ledState = toGrayCode(timeToUc);

        sendToUc(serialStream, data);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
