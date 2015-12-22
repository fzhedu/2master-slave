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
using caf::aout;


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
    cout << "slave socket related errors occur " << endl;

  }
  caf::await_all_actors_done();
}

void SlaveNode::MainBehav(caf::event_based_actor * self, SlaveNode * slave){
  self->become(
      /**
       * Apply master's dispatched "job" and create a job thread
       */
      [=](DispatchAtom, string job)->caf::message {
        auto jobact = caf::spawn(JobBehav,slave, job);
        //slave->job_handle(job);
        return caf::make_message(OkAtom::value);
      },
      /**
       * Apply master update request,
       * and call "update_handle" to update slave
       */
      [=](UpdateAtom, int type, string data)->caf::message  {
        cout << "update  success" << endl;
        slave->update_handle[type]( data);
        return caf::make_message(OkAtom::value);
      },
      [=](ExitAtom) {
        //caf::shutdown();
        cout << "slave exit success" << endl;
        exit(0);
      },
      caf::others >> []() {cout<<"unkown message"<<endl;}
  );
}

RetCode SlaveNode::Register() {
  RetCode ret = 0;
  caf::scoped_actor self;
  try {
    auto master = caf::io::remote_actor(ip_master, port_master);
    self->sync_send(master, RegisterAtom::value, ip, port).await(
            [&](OkAtom) {
               cout <<"registe r success" <<endl;
             },
             [&](const caf::sync_exited_msg & msg){
               ret = -1;
               cout << "register link fail" << endl;
             },
            caf::after(std::chrono::seconds(kTimeout)) >> [&]() {
               ret = -1;
               cout << "slave register timeout" << endl;
            }
        );
  } catch(caf::network_error & e) {
    ret = -1;
    cout << "cannot't connect to <"<<ip_master<<","<< port_master<<">"<<endl;
  }
  return ret;
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
/**
 *  Slave subscribe theme of "type".
 *  Slave must set update handle for theme of "type" previously.
 */
RetCode SlaveNode::Subscribe(int type) {
  /*
  Prop<string> prop;
  auto subscr = caf::spawn(SubscrBehav, this, type, &prop);
  return prop.Join().flag;
  */
  caf::scoped_actor self;
  RetCode ret = 0;
  try {
    auto master = caf::io::remote_actor(ip_master, port_master);
    self->sync_send(master,SubscrAtom::value, ip, port,type).
        await(
          [&](OkAtom) {
            //prop->Done(string(""), 0);
            ret = -1;
            cout << "subscr success" << endl;
          },
          [&](const caf::sync_exited_msg & msg){
            ret = -1;
            cout << "register link fail" << endl;
          },
          caf::after(std::chrono::seconds(kTimeout)) >> [&]() {
            //prop->Done(string(""), -1);
            ret = -1;
            cout << "subscr fail" << endl;
          }
        );
  } catch(caf::network_error & e) {
    //prop->Done(string(""), -1);
    ret = -1;
    cout << "subscr network error" << endl;
  }
  return ret;
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

void SlaveNode::JobBehav(caf::event_based_actor * self, SlaveNode * slave, string job) {
  slave->job_handle(job);
}

#endif //  SLAVE_NODE_CPP_ 
