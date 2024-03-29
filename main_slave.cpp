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
 * /master-slave/main_slave.cpp
 *
 *  Created on: Nov 30, 2015
 *      Author: imdb
 *		   Email: 
 * 
 * Description:
 *
 */

#include <iostream>
#include <string>
#include <unistd.h>
#include "slave_node.h"
using std::string;
using std::cin;
using std::cout;
using std::endl;

/*
int main(){

  int port;
  cout << "input port" << endl;
  cin>>port;
  string x = "hello world";
  Subscr subscr(address("127.0.0.1", port),address("127.0.0.1",8000));
  subscr.SetUpdateHandle(
      [&](string value) { x = value;}
  );
  subscr.Start();
  while(true) {
    string cmd;
    cin >> cmd;
    if (cmd == "show")
      cout << x << endl;
  }
}
*/

void behavA(caf::event_based_actor * self, caf::actor * B){
  for(int i = 0; i<10;i++){
    self->sync_send(*B, i).then(
        [=](int ret) { cout<<"ans:"<<ret<<endl;}
    );
  }
}
void behavC(caf::event_based_actor * self){
  self->become(
      [=](int i)->caf::message {
        sleep(1);
        return caf::make_message(i*i);
      }
  );
}
void behavB(caf::event_based_actor * self) {
  self->become(
      [=](int i) {
        auto C = caf::spawn(behavC);
        self->forward_to(C);
      }
  );
}

caf::actor A;
caf::actor B;

int main(){
/*
  B = caf::spawn(behavB);
  A = caf::spawn(behavA,&B);

  caf::await_all_actors_done();
  */
  int type1 = 1;
  int type2 = 2;
  string x = "hello world";
  SerTest y;
  UInt16 port;
  cout << "input port" <<endl;
  cin >> port;
  SlaveNode slave("127.0.0.1",port,"127.0.0.1",8000);
  slave.Start();
  slave.Register();
  //slave.Heartbeat();
  slave.Subscribe(type1);
  slave.Subscribe(type2);
  slave.SetUpdateHandle(
      type1,
      [&](string _value) { x = _value;}
  );
  slave.SetUpdateHandle(
      type2,
      [&](string _value) { y = Derialize<SerTest>(_value);}
  );
  slave.SetJobHandle(
      [&](string job) { cout << "job:" << job << endl;}
  );
  while(true){
    string cmp;
    cin>>cmp;
    if (cmp == "showx")
      cout << x << endl;
    else if (cmp == "showy")
      y.print();
    else if (cmp == "exit")
      slave.Exit();
  }
}



