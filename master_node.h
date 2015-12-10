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
class MasterNode: public BasedNode {
 public:
  MasterNode() {}
  MasterNode(address addr):BasedNode(addr){ }
  ~MasterNode() {}
  RetCode Start();
  string GetStatus() const { return "Master\n";}
  vector<NodeInfo> GetLive();
  vector<NodeInfo> GetDead();
  void Dispatch(vector<NodeInfo> slave_list, string op);
  void Dispatch(NodeInfo slave, string op);
  void Notify();
  void SetNotifyHandle(function<string()> fun) { notify_handle = fun;}
  void Subscribe(address addr) { subscr_list.push_back(addr);}
  map<address, NodeInfo> cluster_info;
  vector<address> subscr_list;
  function<string()> notify_handle;
 private:
  static void * MasterNodeThread(void * arg);
  static behavior MasterMainBehav(event_based_actor * self, MasterNode * master);
  static void MasterMonitorBehav(caf::blocking_actor * self, MasterNode * master);
  RetCode Monitor();
  RetCode Clock();

};



#endif //  MASTER_NODE_H_ 
