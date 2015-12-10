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
class SlaveNode: public BasedNode {
 public:

  SlaveNode(){}
  SlaveNode(address self) : BasedNode(self) { }
  SlaveNode(address self, address master)
  : BasedNode(self),master_ip(master.first),master_port(master.second) { }
  ~SlaveNode() { caf::shutdown();}
  RetCode Start() ;
  string GetStatus() const { return "Master\n";}
  string master_ip;
  Int16 master_port;
  bool is_register = false;

 private:
  static void SlaveRegBehav(caf::blocking_actor * self,SlaveNode * slave);
  static void SlaveHeartbeatBehav(caf::blocking_actor * self, SlaveNode * slave);
  static caf::behavior SlaveMainBehav(caf::event_based_actor * self, SlaveNode* slave);
  static void * SlaveNodeThread(void * arg);
  RetCode Register();
  RetCode Heartbeat();
};

class Subscr{
 public:

  Subscr(){}
  Subscr(address self,address master):ip(self.first),port(self.second),
      master_ip(master.first),master_port(master.second) { }
  void Start();
  RetCode Subscribe();
  void SetUpdateHandle(function<void(string)> fun) { update_handle = fun;}
  string ip;
  Int16 port;
  string master_ip;
  Int16 master_port;
  function<void(string)> update_handle;
 private:
  void Update();
  static void * SubscrThread(void * arg);
  static caf::behavior SubscrMainBehav(caf::event_based_actor * self, Subscr * subscr);
  static void UpdateBehav(caf::blocking_actor * self, Subscr * subscr,
                          Prop<string> * prop);
  static void SubscribeBehav(caf::blocking_actor * self, Subscr * subscr,
                            Prop<string> * prop);
};

#endif //  SLAVE_NODE_H_ 
