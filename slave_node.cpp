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
 * /master-slave/slave_node.cpp
 *
 *  Created on: Nov 28, 2015
 *      Author: imdb
 *		   Email: 
 * 
 * Description:
 *
 */

#ifndef SLAVE_NODE_CPP_
#define SLAVE_NODE_CPP_

#include <iostream>
#include <string>
#include <chrono>
#include <unistd.h>
#include <pthread.h>
#include "caf/all.hpp"
#include "caf/io/all.hpp"
#include "./based_node.h"
#include "./slave_node.h"
#include "./master_node.h"

using std::string;
using std::cin;
using std::cout;
using std::endl;
using caf::event_based_actor;
using caf::behavior;



RetCode SlaveNode::Start(){

  int rc;
  pthread_t pid;
  rc = pthread_create(&pid, NULL, MainThread, (void *)this);
  return rc == 0 ? 0 : -1;
}

void * SlaveNode::MainThread(void * arg){
  SlaveNode * self = (SlaveNode*)arg;
  auto slave = caf::spawn(MainBehav, self);
  try {
    caf::io::publish(slave, self->port);
    cout << "slave publish success" << endl;
  } catch (caf::bind_failure & e){
    cout << "the specified port is already in use" << endl;
  } catch( caf::network_error & e) {
    cout << "socket related errors occur " << endl;
  }
  caf::await_all_actors_done();
}

void SlaveNode::MainBehav(caf::event_based_actor * self, SlaveNode * slave){
  self->become(
      [=](DispatchAtom) {},
      [=](UpdateAtom, int type, string & data) {
        cout << "update" << type <<" success" << endl;
        slave->update_handle[type]( data);
      },
      caf::others >> [=]() {cout<<"unkown message";}
  );
}

RetCode SlaveNode::Register() {
  Prop<string> prop;
  auto reg = caf::spawn<caf::blocking_api>(RegBehav,this, &prop);
  return prop.Join().flag;
}

void SlaveNode::RegBehav(caf::blocking_actor * self, SlaveNode * slave,
                         Prop<string> * prop) {
  try {
    auto master = caf::io::remote_actor(slave->ip_master, slave->port_master);
    self->on_sync_failure([=](){
      cout << "slave register fail " << endl;
    });
    self->sync_send(master, RegisterAtom::value, slave->ip, slave->port).await(
            [=](OkAtom) {
              prop->Done("", 0);
              cout <<"register success" <<endl;
             },
            caf::after(std::chrono::seconds(prop->timeout)) >> [=]() {
              prop->Done(string(""), -1);
              cout << "slave register timeout" << endl;
            }
        );
  } catch(caf::network_error & e) {
    cout << "cannot't connect to <"<<slave->ip_master<<","
        <<slave->port_master<<">"<<endl;
    prop->Done(string(""), -1);
  }
}

RetCode SlaveNode::Heartbeat() {
  auto heatbeat = caf::spawn(HeartbeatBehav, this);
}

void SlaveNode::HeartbeatBehav(caf::event_based_actor * self,SlaveNode * slave) {
  self->become(
      [=](HeartbeatAtom) {
        try {
          auto master = caf::io::remote_actor(slave->ip_master, slave->port_master);
          self->sync_send(master, HeartbeatAtom::value, slave->ip, slave->port).then(
              [=](OkAtom) { },
              caf::after(std::chrono::seconds(kTimeout)) >> [=]() {
                cout << "heartbeat timeout" << endl;
              }
          );
        } catch (caf::network_error & e) {
          cout <<"cannot't connect when heartbeat" << endl;
        }
        self->delayed_send(self,std::chrono::seconds(kTimeout),HeartbeatAtom::value);
      }
  );
  self->send(self, HeartbeatAtom::value);
}

RetCode SlaveNode::Subscribe(int type) {
  Prop<string> prop;
  auto subscr = caf::spawn(SubscrBehav, this, type, &prop);
  return prop.Join().flag;
}

void SlaveNode::SubscrBehav(caf::event_based_actor * self, SlaveNode * slave,
                            int type, Prop<string>* prop) {
  try {
    auto master = caf::io::remote_actor(slave->ip_master,slave->port_master);
    self->sync_send(master,SubscrAtom::value,slave->ip,slave->port,type).
        then(
        [=](OkAtom) {
          prop->Done(string(""), 0);
          cout << "subscr success" << endl;
        },
        caf::after(std::chrono::seconds(prop->timeout)) >> [=]() {
          prop->Done(string(""), -1);
          cout << "subscr fail" << endl;
        }
        );
  } catch(caf::network_error & e) {
    prop->Done(string(""), -1);
  }
}



#endif //  SLAVE_NODE_CPP_ 
