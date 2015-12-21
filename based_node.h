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
#include <sstream>
#include <string>
#include <utility>
#include <functional>
#include <semaphore.h>
#include <sstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/utility.hpp>
#include "caf/all.hpp"
#include "caf/io/all.hpp"

using caf::spawn_mode;
using std::string;
using std::cout;
using std::cin;
using std::endl;
using std::pair;
using std::function;
using std::vector;
using std::stringstream;
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
using ExitAtom = caf::atom_constant<caf::atom("exit")>;

typedef int RetCode;
typedef unsigned short UInt16;
typedef pair<string, UInt16> Addr;
typedef pair<Addr,RetCode> NodeRetCode;
const auto kTimeout = 3;
const auto kMaxTryTimes = 10;
/*
 * This is a block API, for all node to terminate;
 */



class NodeInfo {
 public:
  string ip;
  UInt16 port;
  bool is_live;
  unsigned count;
  NodeInfo():is_live(true),count(0) {}
  NodeInfo(string _ip, UInt16 _port):ip(_ip),port(_port),
      is_live(true),count(0) {
  }
  void print(){
    cout<<"ip:"<<ip<<" port:"<<port<<" IsLive:"<<is_live
        <<" Count:"<<count<<endl;
  }
};

class BasedNode {
 public:
  BasedNode() {}
  BasedNode(string _ip, UInt16 _port):ip(_ip),port(_port) { }
  ~BasedNode() {}
  string ip;
  UInt16 port;
};

template<typename T>
class CallRet{
 public:
  CallRet(){}
  CallRet(T _value, RetCode _flag):value(_value),flag(_flag) {}
  T value;
  RetCode flag = 0;
};

template<typename T>
class Prop{
 public:
 Prop() { sem_init(&done, 0, 0);}
 ~Prop() { sem_destroy(&done);}
 CallRet<T> Join() {
   sem_wait(&done);
   return CallRet<T>(value,flag);
 }
 void Done(const string & _context, RetCode _flag ){
   value = T(_context);
   flag = _flag;
   sem_post(&done);
 }
 void SetTimeout(int _timeout) {timeout = _timeout;}
 T value;
 RetCode flag = 0;
 int timeout = 3;

 private:
  sem_t done;
};

template<typename T>
class MultiProp{
 public:
  MultiProp (int c) {
    count = c;
    value = new T[count]();
    flag = new RetCode[count]();
    for (auto i=0;i<count;i++)
      flag[count] = -1;
    sem_init(&done, 0, 0);
  }
  ~MultiProp () {
    delete [] value;
    delete [] flag;
  }
  void Done(int id, const string & _value, bool _flag){
    value[id] = T(_value);
    flag[id] = _flag;
    sem_post(&done);
  }
  vector<CallRet<T>> Join(){
   for (auto i=0;i<count;i++)
     sem_wait(&done);
   cout << "done:" <<done.__align<<endl;
   vector<CallRet<T>> ret;
   for (auto i=0;i<count;i++)
     ret.push_back(CallRet<T>(value[i], flag[i]));
   return ret;
  }

 T * value = nullptr;
 RetCode * flag = nullptr;
 int timeout = 3;
 int count = 0;
 private:
  sem_t done;
};

class SerTest{
 public:
  int a;
  string b;
  double c;
  SerTest(){}
  SerTest(int _a, string _b, double _c):
    a(_a), b(_b), c(_c) { }
  void print() {
    cout << "a:" << a << endl;
    cout << "b:" << b << endl;
    cout << "c:" << c << endl;
  }
 private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & a & b & c;
  }

};

template<typename T>
string Serialize(T obj) {
  stringstream ss;
  boost::archive::text_oarchive oa(ss);
  oa << obj;
  return ss.str();
}

template<typename T>
T Derialize(string obj) {
  T ret;
  stringstream ss(obj);
  boost::archive::text_iarchive ia(ss);
  ia >> ret;
  return ret;
}




#endif //  BASED_NODE_H_ 
