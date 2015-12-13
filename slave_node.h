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
 * /master-slave/slave_node.h
 *
 *  Created on: Nov 28, 2015
 *      Author: imdb
 *		   Email: 
 * 
 * Description:
 *
 */

#ifndef SLAVE_NODE_H_
#define SLAVE_NODE_H_

#include <iostream>
#include <string>
#include <functional>
#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include "./based_node.h"
#include "./slave_node.h"
using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::function;
using caf::event_based_actor;
using caf::behavior;
using std::map;



class SlaveNode:public BasedNode {
 public:
  SlaveNode() {}
  SlaveNode(string _ip, UInt16 _port, string _ip_master, UInt16 _port_master):
    ip(_ip),port(_port),ip_master(_ip_master),port_master(_port_master) {
    cout<<"ip"<<ip<<endl;
    cout<<"port"<<port<<endl;
    cout<<"ip_master"<<ip_master<<endl;
    cout<<"port_master"<<port_master<<endl;
  }
  ~SlaveNode() {}
  RetCode Start();
  RetCode Register();
  RetCode Heartbeat();
  RetCode Subscribe(int type);
  void SetUpdateHandle(int type, function<void(string)> fun) {
    update_handle[type] = fun;
  }
  string ip;
  UInt16 port;
  string ip_master;
  UInt16 port_master;
  map<int, function<void(string)>> update_handle;


 private:
 RetCode Update(int type);
 RetCode Execute(string op);

 private:
 static void MainBehav(caf::event_based_actor * self, SlaveNode * slave);
 static void * MainThread(void * arg);
 static void RegBehav(caf::blocking_actor * self,SlaveNode * slave,
                      Prop<string> * prop);
 static void HeartbeatBehav(caf::event_based_actor * self, SlaveNode * slave);
 static void SubscrBehav(caf::event_based_actor * self, SlaveNode * slave,
                         int type, Prop<string>* prop);
};


#endif //  SLAVE_NODE_H_ 
