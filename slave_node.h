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
  /**
   * @param slave self's ip
   * @param slave self's port
   * @param slave self's ip_master
   * @param slave self's port_master
   */
  SlaveNode(string _ip, UInt16 _port, string _ip_master, UInt16 _port_master):
    ip(_ip),port(_port),ip_master(_ip_master),port_master(_port_master) {
  }
  ~SlaveNode() {}
  /**
   *  Launch slave main thread，and publish it to Internet at given <ip,port>
   */
  RetCode Start();
  /**
   *  Slave register to master(at given <ip, port>).
   *  The return will be 0 if success register, or negative.
   */
  RetCode Register();
  /**
   *  Launch a thread to send heart beat message to master at proper time
   */
  RetCode Heartbeat();

  /**
   * Add the Slave to master subscribe list at "type" theme
   */
  RetCode Subscribe(int type);
  /**
   * Set slave's update callback function handle "fun".
   * If slave receive master's update message, it will be called.
   * The "fun" has a string param provide by system, it can used to
   * update local data.
   */
  void SetUpdateHandle(int type, function<void(string)> fun) {
    update_handle[type] = fun;
  }
  /**
   * Set slave's job callback function  handle "fun".
   * When slave receive master's job request,
   * Slave call "fun" to do the job.
   * Job string can be transformed to object by "Derialize"
   */
  void SetJobHandle(function<void(string)> fun) {
    job_handle = fun;
  }

  string ip;
  UInt16 port;
  string ip_master;
  UInt16 port_master;
  map<int, function<void(string)>> update_handle;
  function<void(string)> job_handle;

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
 static void JobBehav(caf::event_based_actor * self, SlaveNode * slave, string job);
 static void EmptyBehav(caf::blocking_actor * self) { }

};


#endif //  SLAVE_NODE_H_ 
