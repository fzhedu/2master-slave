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
  /**
   * @param master's ip
   * @param master's port
   */
  MasterNode(string ip, UInt16 port):BasedNode(ip, port){ }
  ~MasterNode() {}
  /**
   * Launch master main thread，and publish it to Internet at given <ip,port>
   */
  RetCode Start();
  /**
   * Launch monitor thread, and it will check register node at a frequency.
   */
  RetCode Monitor();
  /**
   * Set a callback function handle by lambda expression.
   * It will be call when "notify" is called.
   * The lambda expression return a string, which is the
   * new value to notify subscribers.
   */
  void SetNotifyHandle(int type, function<string()> fun) {
    notify_handle[type]=fun;
  }
  /**
   * Notify all subscribers to update data.
   * Multiple theme subscribe is implemented.
   * "type" is the theme of subscribe, to be defined in a header file by user.
   * Return value is vector of error notify, so if notify success, the
   * return vector will be empty.
   * elements of vector is a pair<index of master.subscr_list[type], RetCode of error>.
   */
  vector<pair<int, RetCode>> Notify(int type);
  /**
   * Dispatch a string value job to a slave at address "addr".
   * Execute is right when RetCode is 0
   */
  RetCode Dispatch(Addr slave, string job);
  /**
   * Dispatch a string value job to a "slave_list" at address list
   * The return vector is same as the notify
   */
  vector<pair<int, RetCode>> BroadDispatch(vector<Addr> slave_list, string job);
  /**
   * Get all live node
   */
  vector<Addr> GetLive();
  /**
   * Get all dead node
   */
  vector<Addr> GetDead();

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
