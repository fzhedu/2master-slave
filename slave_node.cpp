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

void SlaveNode::SlaveRegBehav(caf::blocking_actor * self,SlaveNode * slave){
  auto master = caf::io::remote_actor(slave->master_ip, slave->master_port);
  self->sync_send(master,RegisterAtom::value,slave->ip,slave->port).
      await(
      [&](OkAtom) {
        slave->is_register = true;
        cout << "register ok" << endl;
      },
      caf::after(std::chrono::seconds(kMaxTryTimes)) >> [&]() {
        cout << "register timeout" << endl;
      }
  );
}

void SlaveNode::SlaveHeartbeatBehav(caf::blocking_actor * self, SlaveNode * slave) {
  while(true) {
    auto master = caf::io::remote_actor(slave->master_ip,slave->master_port);
    auto success = false;
    self->sync_send(master, HeartbeatAtom::value, slave->ip, slave->port).
        await(
        [&](OkAtom) {
          cout << "beats ok" << endl;
          success = true;
        },
        caf::after(std::chrono::seconds(kTimeout)) >> [&]() {
          cout << "heartbeat timeout" << endl;
        }
     );
    if(success)
      sleep(kTimeout);
  }
}

caf::behavior SlaveNode::SlaveMainBehav(caf::event_based_actor * self, SlaveNode* slave) {
  cout << "slave<" << slave->ip << ":" << slave->port << "> start" << endl;
  /*
   * register to master
   */

  return {
   [=](DispatchAtom, string op) {
    cout << "execute " << op << endl;
    return caf::make_message(OkAtom::value);
   }
  };
}

void * SlaveNode::SlaveNodeThread(void * arg) {
  SlaveNode * self = (SlaveNode*)arg;
  auto slave = caf::spawn(SlaveMainBehav, self);
  try {
    caf::io::publish(slave, self->port,self->ip.c_str());
  } catch (caf::bind_failure & e){
    cout << "the specified port is already in use" << endl;
  } catch( caf::network_error & e) {
    cout << "socket related errors occur " << endl;
  }
  self->Register();
  self->Heartbeat();
  caf::await_all_actors_done();
}

RetCode SlaveNode::Start() {
  RetCode ret = 0;
  pthread_t pid;
  pthread_create(&pid, NULL, SlaveNodeThread, (void *)this);
  return ret;
}

RetCode SlaveNode::SlaveNode::Register() {
  auto count = 0;
  while(!is_register && count < kMaxTryTimes) {
    auto reg = caf::spawn<caf::blocking_api>(SlaveRegBehav, this);
    sleep(kTimeout+1);
  }
  return is_register == true ? 0 : 1;
}

RetCode SlaveNode::SlaveNode::Heartbeat() {
  auto heatbeat = caf::spawn<caf::blocking_api>(SlaveHeartbeatBehav, this);
  return 0;
}



void Subscr::Start(){
  pthread_t pid;
  pthread_create(&pid, NULL, SubscrThread, (void *)this);
}

void * Subscr::SubscrThread(void * arg){
  Subscr * subscr = (Subscr *)arg;
  auto self = caf::spawn(Subscr::SubscrMainBehav, subscr);
  caf::io::publish(self, subscr->port);
  caf::await_all_actors_done();
}

caf::behavior Subscr::SubscrMainBehav(caf::event_based_actor * self, Subscr * subscr) {
  auto ret = subscr->Subscribe();
  if (ret==0)
    return {
      [=](UpdateAtom){ subscr->Update();}
    };
  else
    return {};
}

void Subscr::Update(){
  Prop<string> prop;
  caf::spawn<caf::blocking_api>(Subscr::UpdateBehav, this, &prop);
  auto ret = prop.Join();
  update_handle(prop.value);
}

void Subscr::UpdateBehav(caf::blocking_actor * self, Subscr * subscr,
                         Prop<string> * prop) {
  try {
    auto master = caf::io::remote_actor(subscr->master_ip,subscr->master_port);
    self->on_sync_failure(prop->failure_handle);
    self->sync_send(master, UpdateAtom::value).await(
        [=](OkAtom, string context){
          prop->Done(context, 0);
          cout << "update success" << endl;
        },
        caf::after(std::chrono::seconds(prop->timeout)) >> [=]() {
          prop->Done(string(""), -1);
          cout << "update timeout" << endl;
        }
    );
  } catch(caf::network_error & e) {
    cout << "subscr link to master fail when update" << endl;
  }
}

RetCode Subscr::Subscribe(){
  Prop<string> prop;

  caf::spawn<caf::blocking_api>(Subscr::SubscribeBehav, this, & prop);
  return prop.Join().second;
}

void Subscr::SubscribeBehav(caf::blocking_actor * self, Subscr * subscr,
                           Prop<string> * prop) {
  try {
    auto master = caf::io::remote_actor(subscr->master_ip, subscr->master_port);
    self->sync_send(master, SubscrAtom::value, subscr->ip, subscr->port).
        await(
            [=](OkAtom ){
              cout << "subscribe success" << endl;
              prop->Done(string(""), 0);
            },
            caf::after(std::chrono::seconds(prop->timeout)) >> [=]() {
              cout << "subscribe timeout" << endl;
              prop->Done(string(""), -1);
            }
        );
  } catch (caf::network_error & e) {
    cout << "subscr link to master fail where subscribe" << endl;\
    prop->Done(string(""), -1);
  }
}
#endif //  SLAVE_NODE_CPP_ 
