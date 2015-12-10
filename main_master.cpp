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
 * /master-slave/main_master.cpp
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
#include "master_node.h"
#include "slave_node.h"
#include <semaphore.h>
#include "caf/all.hpp"
#include "caf/io/all.hpp"
using std::string;
using std::cin;
using std::cout;
using std::endl;

void behav(caf::blocking_actor * self , sem_t * done){

  auto count = 0;
  while(count < 3) {
    cout <<  count << endl;
    sleep(3);
    count ++;
  }
  sem_post(done);
}

int main() {

  cout << "This is master" << endl;
  string x ="hello world";
  MasterNode master(address("127.0.0.1",8000));
  master.SetNotifyHandle(
      [&](){ return x;}
  );
  master.Start();
  while(true) {
    //string op;
    //cin>>op;
    //auto slave_list = master.GetLive();
    //master.Dispatch(slave_list, op);

    //sleep(3);
    //for (auto p=master.subscr_list.begin();p!=master.subscr_list.end();p++)
    //  cout << p->first<<","<< p->second <<" ";
    //cout <<"*****"<<endl;
    cin >> x;
    master.Notify();
  }
 /*
  sem_t done;
  sem_init(&done, 0, 0);
  auto p = caf::spawn<caf::blocking_api>(behav, &done);
  sem_wait(&done);
  cout << "wait end;" <<endl;
  */

}
