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

RetCode MasterNode::Start() {
  int rc;
  pthread_t pid;
  rc = pthread_create(&pid, NULL, MainThread, (void *)this);
  return rc == 0 ? 0 : -1;
}

void *MasterNode::MainThread(void *arg) {
  MasterNode *self = (MasterNode *)arg;
  auto master = caf::spawn(MainBehav, self);
  try {
    caf::io::publish(master, self->port);
    cout << "master publish success" << endl;
  } catch (caf::bind_failure &e) {
    cout << "the specified port is already in use" << endl;
  } catch (caf::network_error &e) {
    cout << "master socket related errors occur " << endl;
  }

  caf::await_all_actors_done();
}

void MasterNode::MainBehav(caf::event_based_actor *self, MasterNode *master) {
  self->become(
      [=](RegisterAtom, string ip, UInt16 port) -> caf::message {
        NodeInfo node(ip, port);
        node.is_live = true;
        node.count = 0;
        Addr addr(ip, port);
        master->slave_list[addr] = node;
        cout << "slave <" << ip << "," << port << "> success" << endl;
        return caf::make_message(OkAtom::value);
      },
      [=](HeartbeatAtom, string ip, UInt16 port) -> caf::message {
        cout << "heartbeat from <" << ip << "," << port << ">" << endl;
        master->slave_list[Addr(ip, port)].count = 0;
        master->slave_list[Addr(ip, port)].is_live = true;
        return caf::make_message(OkAtom::value);
      },
      [=](SubscrAtom, string ip, UInt16 port, int type) -> caf::message {
        cout << "slave<" << ip << "," << port << "> subscr " << type << endl;
        bool exist = false;
        for (auto i = master->subscr_list[type].begin();
             i != master->subscr_list[type].end(); i++)
          if (*i == Addr(ip, port)) {
            exist = true;
            break;
          }
        if (!exist) master->subscr_list[type].push_back(Addr(ip, port));
        return caf::make_message(OkAtom::value);
      },
      [=](ExitAtom) {
        // caf::io::unpublish(*self, master->port);
        // caf::shutdown();
        cout << "master exit success" << endl;
        exit(0);
      },
      caf::others >> [=]() { cout << "unkown message" << endl; });
}

RetCode MasterNode::Monitor() { auto monitor = caf::spawn(MonitorBehav, this); }

void MasterNode::MonitorBehav(caf::event_based_actor *self,
                              MasterNode *master) {
  self->become([=](MonitorAtom) {
    for (auto p = master->slave_list.begin(); p != master->slave_list.end();
         p++) {
      p->second.count++;
      if (p->second.count >= kMaxTryTimes) {
        p->second.is_live = false;
        cout << "slave<" << p->second.ip << "." << p->second.port << "> dead"
             << endl;
        if (p->second.count > kMaxTryTimes * kMaxTryTimes) {
          p->second.count = kMaxTryTimes;
        }
      }
    }
    self->delayed_send(self, std::chrono::seconds(kTimeout),
                       MonitorAtom::value);
  });
  self->send(self, MonitorAtom::value);
}

vector<pair<int, RetCode>> MasterNode::Notify(int type) {
  /*
  auto target = subscr_list[type];
  string data = notify_handle[type]();
  MultiProp<string> prop(target.size());
  for (auto i = 0; i < target.size(); i++)
    auto slave = caf::spawn(NotifyBehav, this, type, data, &prop, i);
  auto join_ret = prop.Join();
  cout << "notify complete" << endl;
  vector<Addr> ret;
  for (auto i = 0; i < target.size(); i++)
    if (join_ret[i].flag != 0) ret.push_back(target[i]);
  return ret;
  */
  auto slave_list = subscr_list[type];
  string data = notify_handle[type]();
  caf::scoped_actor self;
  vector<pair<int, RetCode>> ret_list;
  for (auto id = 0; id < slave_list.size(); id++){
    int ret = 0;
    auto ip = slave_list[id].first;
    auto port = slave_list[id].second;
    try {
      auto slave = caf::io::remote_actor(ip, port);
      cout << "notify <" << ip << "," << port << ">"<< endl;
      self->sync_send(slave, UpdateAtom::value, type, data)
        .await(
            [&](OkAtom) {
                cout << "notify  success" << endl;
            },
            [&](const caf::sync_exited_msg & msg){
              ret = -1;
              cout << "notify link fail" << endl;
            },
            caf::after(std::chrono::seconds(kTimeout)) >> [&]() {
                ret = -1;
                cout << "notify timeout" << endl;
            });
      } catch (caf::network_error &e) {
        ret = -1;
        cout << "can't conncet to slave<" << ip << ","
             << port << ">" << endl;
      }
   if (ret != 0)
     ret_list.push_back(pair<int, RetCode>(id, ret));
  }
  return ret_list;
}

void MasterNode::NotifyBehav(caf::event_based_actor *self, MasterNode *master,
                             int type, string data, MultiProp<string> *prop,
                             int id) {
  auto slave_addr = master->subscr_list[type][id];
  try {
    auto slave = caf::io::remote_actor(slave_addr.first, slave_addr.second);
    // caf::anon_send(slave,UpdateAtom::value, type, data);
    /*
    self->sync_send(slave,UpdateAtom::value).then(
      [=](OkAtom) {
        cout << "slave <" << slave_addr.first<<","<<slave_addr.second
            << "> notify success" << endl;
        prop->Done(id, string(""), 0);
      },

      caf::after(std::chrono::seconds(prop->timeout)) >> [=]() {

        cout << "slave <" << slave_addr.first<<","<<slave_addr.second
            << "> notify timeout" << endl;
        prop->Done(id, string(""), -1);
      }

    ); */
    cout << "notify <" << slave_addr.first << "," << slave_addr.second << ">"
         << endl;
    self->sync_send(slave, UpdateAtom::value, type, data)
        .then([=](OkAtom) {
                prop->Done(id, "", 0);
                cout << "notify  success" << endl;
              },
              caf::after(std::chrono::seconds(prop->timeout)) >>
                  [=]() {
                    prop->Done(id, "", -1);
                    cout << "notify timeout" << endl;
                  });
  } catch (caf::network_error &e) {
    prop->Done(id, "", -1);
    cout << "can't conncet to slave<" << slave_addr.first << ","
         << slave_addr.second << ">" << endl;
  }
}

RetCode MasterNode::Dispatch(Addr addr, string job) {
  /*
  Prop<string> prop;
  caf::spawn(DispatchBehav, addr, job, &prop);
  return prop.Join().flag;
  */
  caf::scoped_actor self;
  RetCode ret = 0;
  try {
    auto slave = caf::io::remote_actor(addr.first, addr.second);
    self->sync_send(slave, DispatchAtom::value, job)
        .await(
            [&](OkAtom) {
              cout << "dispatch success" << endl;
            },
            [&](const caf::sync_exited_msg & msg){
              ret = -1;
              cout << "dispatch link fail" << endl;
            },
            caf::after(std::chrono::seconds(kTimeout)) >> [&]() {
                  ret = -1;
                  cout << "dispatch timeout" << endl;
            }
        );
  } catch (caf::network_error &e) {
     ret = -1;
     cout << "dispatch network_error" << endl;
  }
  return ret;
}

void MasterNode::DispatchBehav(caf::event_based_actor *self, Addr addr,
                               string job, Prop<string> *prop) {
  try {
    auto slave = caf::io::remote_actor(addr.first, addr.second);
    self->sync_send(slave, DispatchAtom::value, job)
        .then([=](OkAtom) { prop->Done("", 0); },
              caf::after(std::chrono::seconds(prop->timeout)) >>
                  [=]() {
                    prop->Done("", -1);
                    cout << "notify timeout" << endl;
                  });
  } catch (caf::network_error &e) {
    prop->Done("", -1);
  }
}

vector<pair<int, RetCode>> MasterNode::BroadDispatch(vector<Addr> slave_list, string job) {
  /*
  vector<Addr> ret;
  MultiProp<string> prop(addr_list.size());
  for (auto i = 0; i < addr_list.size(); i++)
    caf::spawn(BDispatchBehav, addr_list, job, &prop, i);
  auto join_ret = prop.Join();
  for (auto i = 0; i < addr_list.size(); i++)
    if (join_ret[i].flag != 0) ret.push_back(addr_list[i]);
  return ret;
  */
  caf::scoped_actor self;
  vector<pair<int, RetCode>> ret_list;
  for (auto i = 0 ;i < slave_list.size(); i++) {
    RetCode ret = 0;
    auto ip = slave_list[i].first;
    auto port = slave_list[i].second;
    try {
      auto slave = caf::io::remote_actor(ip, port);
      self->sync_send(slave, DispatchAtom::value, job)
          .await(
            [&](OkAtom) {
              cout << "run" << endl;
            },
            [&](const caf::sync_exited_msg & msg){
              ret = -1;
              cout << "dispatch link fail" << endl;
            },
            caf::after(std::chrono::seconds(kTimeout)) >> [&]() {
              ret = -1;
              cout << "dispatch timeout" << endl;
             });
    } catch (caf::network_error &e) {
      ret = -1;
    }
    if (ret != 0)
      ret_list.push_back(pair<int, RetCode>(i, ret));
  }
  return ret_list;
}

void MasterNode::BDispatchBehav(caf::event_based_actor *self,
                                vector<Addr> addr_list, string job,
                                MultiProp<string> *prop, int id) {
  try {
    auto slave =
        caf::io::remote_actor(addr_list[id].first, addr_list[id].second);
    self->sync_send(slave, DispatchAtom::value, job)
        .then([=](OkAtom) {
                prop->Done(id, "", 0);
                cout << "run" << endl;
              },
              caf::after(std::chrono::seconds(prop->timeout)) >>
                  [=]() {
                    prop->Done(id, "", -1);
                    cout << "dispatch timeout" << endl;
                  });
  } catch (caf::network_error &e) {
    prop->Done(id, "", -1);
  }
}

vector<Addr> MasterNode::GetLive() {
  vector<Addr> ret;
  for (auto i = slave_list.begin(); i != slave_list.end(); i++)
    if (i->second.is_live) ret.push_back(Addr(i->second.ip, i->second.port));
  return ret;
}

vector<Addr> MasterNode::GetDead() {
  vector<Addr> ret;
  for (auto i = slave_list.begin(); i != slave_list.end(); i++)
    if (!i->second.is_live) ret.push_back(Addr(i->second.ip, i->second.port));
  return ret;
}
