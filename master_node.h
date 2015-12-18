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
 * /master-slave/master_node.h
 *
 *  Created on: Nov 28, 2015
 *      Author: imdb
 *		   Email: 
 * 
 * Description:
 *
 */

#ifndef MASTER_NODE_H_
#define MASTER_NODE_H_

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <utility>
#include <functional>
#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include "./based_node.h"
#include "./slave_node.h"
using std::string;
using std::map;
using std::vector;
using std::pair;
using std::function;
using std::unordered_map;


class MasterNode: public BasedNode {
 public:
  MasterNode() {}
  MasterNode(string ip, UInt16 port):BasedNode(ip, port){ }
  ~MasterNode() {}
  RetCode Start();
  RetCode Monitor();
  void SetNotifyHandle(int type, function<string()> fun) {
    notify_handle[type]=fun;
  }
  vector<Addr> Notify(int type);
  RetCode Dispatch(Addr slave, string job);
  vector<Addr> BroadDispatch(vector<Addr> slave, string job);
  vector<Addr> GetLive();
  vector<Addr> GetDead();
  void Exit() {
    auto master = caf::io::remote_actor("127.0.0.1", port);
    caf::anon_send(master, ExitAtom::value);
  }
  map<Addr,NodeInfo> slave_list;
  map<int,vector<Addr>> subscr_list;
  map<int, function<string()>> notify_handle;

 private:
  static void * MainThread(void * arg);
  static void MainBehav(caf::event_based_actor * self, MasterNode * master);
  static void MonitorBehav(caf::event_based_actor * self, MasterNode * master);

 private:
  static void NotifyBehav(caf::event_based_actor * self, MasterNode * master,
                          int type, string  data, MultiProp<string> * prop, int id);
  static void DispatchBehav(caf::event_based_actor * self,
                            Addr addr, string job, Prop<string> * prop);
  static void BDispatchBehav(caf::event_based_actor * self, vector<Addr> addr_list,
                             string job, MultiProp<string> * prop, int id);

};
#endif //  MASTER_NODE_H_ 
