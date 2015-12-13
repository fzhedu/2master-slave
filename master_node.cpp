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
 * /master-slave/master_node.cpp
 *
 *  Created on: Nov 28, 2015
 *      Author: imdb
 *		   Email: 
 * 
 * Description:
 *
 */

#include <iostream>
#include <string>
#include <unistd.h>
#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include "./based_node.h"
#include "./master_node.h"
#include "./slave_node.h"
using std::map;
using std::cout;
using std::endl;
using caf::event_based_actor;
using caf::behavior;
using caf::on;
using caf::spawn;
using caf::io::publish;
using caf::actor;

static caf::event_based_actor main_loop;


RetCode MasterNode::Start(){
  int rc;
  pthread_t pid;
  rc = pthread_create(&pid, NULL, MainThread, (void *)this);
  return rc == 0 ? 0 : -1;
}

void * MasterNode::MainThread(void * arg) {
  MasterNode * self = (MasterNode*)arg;
  auto slave = caf::spawn(MainBehav, self);
  try {
  caf::io::publish(slave, self->port);
  } catch (caf::bind_failure & e){
    cout << "the specified port is already in use" << endl;
  } catch( caf::network_error & e) {
    cout << "socket related errors occur " << endl;
  }
  cout << "master start" << endl;
  caf::await_all_actors_done();
}

void MasterNode::MainBehav(caf::event_based_actor * self, MasterNode * master) {
  self->become(
      [=](RegisterAtom, string ip, UInt16 port)->caf::message {
        NodeInfo node(ip, port);
        Addr addr(ip, port);
        master->slave_list[addr] = node;
        cout<<"slave <" << ip<<"," << port <<"> success"<<endl;
        return caf::make_message(OkAtom::value);
      },
     [=](HeartbeatAtom, string ip, UInt16 port)->caf::message {
        cout << "heartbeat from <"<<ip<<","<<port<<">"<<endl;
        master->slave_list[Addr(ip, port)].count = 0;
        master->slave_list[Addr(ip, port)].is_live = true;
        return caf::make_message(OkAtom::value);
      },
      [=](SubscrAtom, string ip, UInt16 port, int type)->caf::message {
        cout << "slave<"<<ip<<","<<port<<"> subscr " << type <<endl;
        master->subscr_list[type].push_back(Addr(ip, port));
        return caf::make_message(OkAtom::value);
      },
      caf::others >> [=]() {cout << "unkown message" << endl;}
  );
}

RetCode MasterNode::Monitor(){
  auto monitor = caf::spawn(MonitorBehav, this);
}

void MasterNode::MonitorBehav(caf::event_based_actor * self, MasterNode * master) {
  self->become(
      [=](MonitorAtom) {
        for (auto p=master->slave_list.begin();p!=master->slave_list.end();p++){
          p->second.count++;
          if (p->second.count >= kMaxTryTimes){
            p->second.is_live = false;
            cout << "slave<" << p->second.ip <<"."<< p->second.port << "> dead"<< endl;
            if (p->second.count > kMaxTryTimes * kMaxTryTimes) {
              p->second.count = kMaxTryTimes;
            }
          }
        }
        self->delayed_send(self,std::chrono::seconds(kTimeout), MonitorAtom::value);
      }
  );
  self->send(self,MonitorAtom::value);
}

vector<Addr> MasterNode::Notify(int type){
  auto target = subscr_list[type];
  string data = notify_handle[type]();
  cout << "data:"<<data<<" slave:"<<target.size()<<endl;
  MultiProp<string> prop(target.size());
  for (auto i=0;i<target.size();i++)
    auto slave = caf::spawn(NotifyBehav, this, type, data, &prop, i);
  auto join_ret = prop.Join();
  vector<Addr> ret;
  for (auto i=0;i<target.size();i++)
    if (join_ret[i].flag != 0)
      ret.push_back(target[i]);
  return ret;
}

void MasterNode::NotifyBehav(caf::event_based_actor * self, MasterNode * master,
                      int type, string  data, MultiProp<string> * prop, int id){
  auto slave_addr = master->subscr_list[type][id];
  try {
    auto slave = caf::io::remote_actor(slave_addr.first, slave_addr.second);
    self->sync_send(slave,UpdateAtom::value, type, data).then(
      [=](OkAtom) {
        prop->Done(type, string(""), 0);
        cout << "slave <" << slave_addr.first<<","<<slave_addr.second
            << "> notify success" << endl;
      },
      caf::after(std::chrono::seconds(prop->timeout)) >> [=]() {
        prop->Done(type, string(""), -1);
        cout << "slave <" << slave_addr.first<<","<<slave_addr.second
            << "> notify timeout" << endl;
      }
    );
  } catch(caf::network_error & e) {
    prop->Done(id, string(""), -1);
    cout << "can't conncet to slave<" << slave_addr.first <<","
        <<slave_addr.second<<">"<<endl;
  }
}

