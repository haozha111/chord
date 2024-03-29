//
//  chordMessager.cpp
//  chordMessager
//
//  Created by Haoliang on 11/24/15.
//  Copyright © 2015 Haoliang. All rights reserved.
//

#include "chordMessager.h"

namespace chordMessager {

  int chordMessager::startListenLoop() {
    DEBUG_PRINT("Start listen loop");
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    isListenning = true;
    int newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
    if (newsockfd < 0) {
      throw ERRORS::chordMessagerSocketAcceptFail();
    }
    return newsockfd;
  }

  void chordMessager::setSocketFileDescriptor(int _sockfd) {
    sockfd = _sockfd;
  }

  void chordMessager::sendData(std::ifstream& is, int data_size) {
    DEBUG_PRINT("Sending data");
    //read file to buffer
    char* data_buffer = new char[data_size];
    bzero((char*)data_buffer, data_size);
    is.read(data_buffer, data_size);
    ssize_t n = write(sockfd, data_buffer, data_size);
    while (n < 0) {
      std::this_thread::sleep_for(std::chrono::seconds(2));
      n = write(sockfd, data_buffer, data_size);
    }
    //release data buffer
    delete [] data_buffer;
  }

  void chordMessager::receiveData(std::ofstream& os, int data_size) {
    DEBUG_PRINT("Receiving data");
    char* data_buffer = new char[data_size];
    ssize_t n = read(sockfd, data_buffer, data_size);
    while (n < 0) {
      std::this_thread::sleep_for(std::chrono::seconds(2));
      bzero(data_buffer, data_size);
      n = read(sockfd, data_buffer, data_size);
    }
    //write data to ofstream
    os.write((const char*)data_buffer, data_size);
    delete [] data_buffer;
  }

  void chordMessager::deserializeMessage(std::string msg, chordMessageBase*& inmsg) {
    DEBUG_PRINT("Deserializing message");
    if (msg.substr(0, 5) != "type:") {
      inmsg = nullptr;
      return;
    }
    std::unordered_map<std::string, std::string> mp;
    std::istringstream iss(msg);
    std::string str;
    while (getline(iss, str, '|')) {
      if (!str.empty()) {
        size_t pos = str.find(':');
        std::string key = str.substr(0, pos);
        std::string value = str.substr(pos + 1);
        mp[key] = value;
      }
    }
    int type = atoi(mp["type"].c_str());
    if (type == chordMessageType::messageAck) {
      inmsg = new chordMessageAck(node_t(), node_t());
    }else if (type == chordMessageType::messageDetectNode) {
      inmsg = new chordMessageDetectNode(node_t(), node_t(), node_t(mp["hostname"], atoll(mp["identifier"].c_str()), atoi(mp["portno_disp"].c_str()), atoi(mp["portno_rpc"].c_str()), mp["machine_name"]));
    }else if (type == chordMessageType::messageDetectNodeResponse) {
      inmsg = new chordMessageDetectNodeResponse(node_t(), node_t(), node_t(mp["hostname"], atoll(mp["identifier"].c_str()), atoi(mp["portno_disp"].c_str()), atoi(mp["portno_rpc"].c_str()), mp["machine_name"]));
    }else if (type == chordMessageType::messageDataInfo) {
      inmsg = new chordMessageDataInfo(node_t(), node_t(), mp["key"], atoll(mp["int_param"].c_str()));
    }else if (type == chordMessageType::messageDataRequest) {
      inmsg = new chordMessageDataRequest(node_t(), node_t(), mp["key"]);
    }else {
      throw ERRORS::chordMessagerUnknownMessageType();
    }
  }
}
