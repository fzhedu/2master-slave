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

behavior MasterNode::MasterMainBehav(event_based_actor * self, MasterNode * master) {
  cout  << master->ip << ":" << master->port << endl;
  self->on_sync_failure(
    [=] {cout<<"connect fail"<<endl;}
    );
  return {
    /*
     * listen to slave's register message
     */
    [=] (RegisterAtom, string  ip, Int16 port) -> caf::message {
       NodeInfo slave;
       slave.ip = ip;
       slave.port = port;
       slave.is_live = true;
       slave.count = 0;
       master->cluster_info[address(ip, port)] = slave;
      /*
       * master return a 'ok' message as response of slave register request
       */
      cout << "Cluster Info" << endl;
      for(auto p = master->cluster_info.begin();p!=master->cluster_info.end();p++)
        p->second.print();
      return caf::make_message(OkAtom::value);
    },
    [=] (HeartbeatAtom, string  ip, Int16 port)-> caf::message  {

      cout << "heartbeat from slave<" << ip <<"," << port <<">" << endl;
      master->cluster_info[address(ip, port)].count = 0;
      return caf::make_message(OkAtom::value);
    },
    [=] (MonitorAtom) {
      for(auto p = master->cluster_info.begin();p!=master->cluster_info.end();p++) {
        p->second.count++;
        if (p->second.count >= kMaxTryTimes) {
          p->second.is_live = false;
          cout << "slave<" << p->second.ip <<"."<< p->second.port << "> dead"<< endl;

          /*
           *  HeartBeatCount may be large than int_max
           */
          if (p->second.count > kMaxTryTimes * kMaxTryTimes) {
            p->second.count = kMaxTryTimes;
          }
        } else {
          p->second.is_live = true;
        }
      }
    },
    [=](DispatchAtom, NodeInfo slave, string op) {
     try {
        auto sla = caf::io::remote_actor(slave.ip, slave.port);
        self->sync_send(sla,DispatchAtom::value,op).then(
            [=](OkAtom) {
              cout<<"dispatch to slave<"<<slave.ip<<","<<slave.port<<
                  "> op<" << op << "> success"<<endl;
            },
            caf::after(std::chrono::seconds(kTimeout)) >> [=]() {
              cout<<"dispatch time out"<<endl;
            }
        );
     } catch (caf::network_error & e) {
       cout << "dispatch fail" << endl;
     }
    },
    [=](SubscrAtom, string ip, Int16 port)->caf::message{
        master->Subscribe(address(ip,port));
        cout<<"recieve from subscr <"<<ip<<","<<port<<">"<<endl;
        return caf::make_message(OkAtom::value);
    },
    [=](NotifyAtom) {
      for (auto itr = master->subscr_list.begin();
          itr != master->subscr_list.end();itr++){
        try {
          auto subscr = caf::io::remote_actor(itr->first,itr->second);
          caf::anon_send(subscr,UpdateAtom::value);
        } catch (caf::network_error & e) {
          cout << "notify <" << itr->first <<","<<itr->second << "> fail" << endl;
        }
      }

    },
    [=](UpdateAtom)->caf::message {
      string msg = master->notify_handle();
      return caf::make_message(OkAtom::value, msg);
    },
    caf::others >> []() {
      cout << "unkonw message" << endl;
    }
  };
}

void MasterNode::MasterMonitorBehav(caf::blocking_actor * self, MasterNode * master) {
  while(true) {
    auto mas = caf::io::remote_actor(master->ip,master->port);
    caf::anon_send(mas,MonitorAtom::value);
    sleep(kTimeout);
  }
}

void * MasterNode::MasterNodeThread(void * arg) {
  MasterNode * self = (MasterNode *)arg;
  auto master = caf::spawn(MasterMainBehav, self);
  //caf::anon_send(act, RegisterAtom::value, string("127.0.0.1"),(Int16)999);
  try {
    /*
     * publish master node to internet, and provide a service
     */
    caf::io::publish(master, self->port);
    cout << "publish master success" << endl;
  } catch (caf::bind_failure & e) {
     cout << "the specified port is already in use" << endl;
  } catch (caf::network_error & e) {
    cout << "socket related errors occur " << endl;
  }
  self->Monitor();
  caf::await_all_actors_done();
}

RetCode MasterNode::Start()  {
  RetCode ret = 0;
  pthread_t pid;
  pthread_create(&pid, NULL, MasterNodeThread, this);
  return ret;
}

RetCode MasterNode::Monitor() {
  auto monitor = caf::spawn<caf::blocking_api>(MasterMonitorBehav, this);
}

vector<NodeInfo> MasterNode::GetLive() {
  vector<NodeInfo> ret;
  for(auto p = cluster_info.begin();p!=cluster_info.end();p++) {
    if (p->second.is_live)
        ret.push_back(p->second);
  }
  return ret;
}

vector<NodeInfo> MasterNode::GetDead() {
  vector<NodeInfo> ret;
  for(auto p = cluster_info.begin();p!=cluster_info.end();p++) {
    if (!p->second.is_live)
        ret.push_back(p->second);
  }
  return ret;
}

void MasterNode::Dispatch(NodeInfo slave, string op) {
  auto master = caf::io::remote_actor(ip, port);
  caf::anon_send(master, DispatchAtom::value, slave, op);
}

void MasterNode::Dispatch(vector<NodeInfo> slave_list, string op) {
  for (auto itr=slave_list.begin();itr!=slave_list.end();itr++)
    MasterNode::Dispatch(*itr, op);
}

void MasterNode::Notify(){
  try {
    auto mas = caf::io::remote_actor(ip, port);
    caf::anon_send(mas, NotifyAtom::value);
  } catch (caf::network_error & e) {
    cout << "connect to  master fail" << endl;
  }
}




