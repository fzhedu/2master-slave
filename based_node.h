/*
 * Copyright [2012-2015] DaSE@ECNU
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * /master-slave/based_node.h
 *
 *  Created on: Nov 28, 2015
 *      Author: imdb
 *		   Email: 
 * 
 * Description:
 *
 */

#ifndef BASED_NODE_H_
#define BASED_NODE_H_

#include <iostream>
#include <string>
#include <utility>
#include <functional>
#include <semaphore.h>
#include "caf/all.hpp"
#include "caf/io/all.hpp"

using std::string;
using std::cout;
using std::cin;
using std::endl;
using std::pair;
using std::function;
using OkAtom = caf::atom_constant<caf::atom("ok")>;
using FailAtom = caf::atom_constant<caf::atom("fail")>;
using RequestRegAtom = caf::atom_constant<caf::atom("req_reg")>;
using RegisterAtom = caf::atom_constant<caf::atom("register")>;
using HeartbeatAtom = caf::atom_constant<caf::atom("heartbeat")>;
using MonitorAtom = caf::atom_constant<caf::atom("monitor")>;
using DispatchAtom = caf::atom_constant<caf::atom("dispatch")>;
using ExecuteAtom = caf::atom_constant<caf::atom("execute")>;
using SubscrAtom = caf::atom_constant<caf::atom("subscr")>;
using UpdateAtom = caf::atom_constant<caf::atom("update")>;
using NotifyAtom = caf::atom_constant<caf::atom("notify")>;

typedef int RetCode;
typedef unsigned short Int16;
typedef pair<string, Int16> address;

const auto kTimeout = 3;
const auto kMaxTryTimes = 10;
/*
 * This is a block API, for all node to terminate;
 */

class BasedNode {
 public:
  BasedNode() {}
  BasedNode(address self):ip(self.first),port(self.second) { }
  ~BasedNode() {}
  virtual string GetStatus() const = 0; // just for debug info
  string ip;
  Int16 port;
};

class NodeInfo {
 public:
  string ip;
  Int16 port;
  bool is_live = true;
  unsigned count = 0;
  NodeInfo() {}
  NodeInfo(string _ip, Int16 _port):ip(_ip),port(_port) {}
  void print(){
    cout<<"ip:"<<ip<<" port:"<<port<<" IsLive:"<<is_live
        <<" HeartBeatCount:"<<count<<endl;
  }
};

template<typename T>
class Prop{
 public:
  Prop() { sem_init(&done, 0, 0);}
  ~Prop() { sem_destroy(&done);}
  pair<T,RetCode> Join() {
    sem_wait(&done);
    return pair<T,RetCode>(value,flag);
  }
  void Done(const string & _context, bool _flag ){
    value = T(_context);
    flag = _flag;
    sem_post(&done);
  }
  void SetTimeout(int _timeout) {timeout = _timeout;}
  void SetFailureHandle(function<void()> handle){
    failure_handle = handle;
  }
 T value;
 RetCode flag = 0;
 int timeout = 3;
 function<void()> failure_handle = [=](){ cout << "fail" << endl;};
 private:
  sem_t done;
};

#endif //  BASED_NODE_H_ 
